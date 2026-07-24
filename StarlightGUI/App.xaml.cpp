#include "pch.h"
#include "Utils/Config.h"
#include "Utils/Diagnostics.h"
#include "Utils/CppUtils.h"
#include "Utils/Elevator.h"
#include "Utils/KernelBase.h"
#include "Utils/Utils.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include <shellapi.h>
#include <string>
#include <vector>

using namespace winrt;
using namespace Microsoft::UI::Xaml;


namespace winrt::StarlightGUI::implementation
{
    static std::vector<std::wstring> GetCommandLineArgs()
    {
        int argc = 0;
        auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv) return {};

        std::vector<std::wstring> result;
        result.reserve(argc);
        for (int i = 0; i < argc; ++i) result.emplace_back(argv[i]);
        LocalFree(argv);
        return result;
    }

    static bool HasSwitch(const wchar_t* key)
    {
        auto args = GetCommandLineArgs();
        auto prefix = std::wstring(key) + L"=";
        for (size_t i = 1; i < args.size(); ++i) {
            if (_wcsicmp(args[i].c_str(), key) == 0) return true;
            if (args[i].size() > prefix.size() && _wcsnicmp(args[i].c_str(), prefix.c_str(), prefix.size()) == 0) return true;
        }
        return false;
    }

    static void ApplyBuildEditionResources()
    {
        Application::Current().Resources().Insert(box_value(hstring(L"Version")), box_value(hstring(STARLIGHT_VERSION_BASE STARLIGHT_VERSION_SUFFIX)));
    }

    App::App()
    {
        slg::InitializeCrashHandler();

        UnhandledException([](winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e)
            {
                LOG_ERROR(L"App", L"===== Unhandled exception detected! =====");
                LOG_ERROR(L"App", L"Type: 'winrt::hresult_error'");
                LOG_ERROR(L"App", L"Code: 0x%08X", (uint32_t)e.Exception().value);
                LOG_ERROR(L"App", L"Message: %s", e.Message().c_str());
                LOG_ERROR(L"App", L"=========================================");
                slg::CreateCrashDump();
                e.Handled(true);
            });
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        ApplyBuildEditionResources();

        bool trustedInstallerRelaunch = HasSwitch(L"--trustedinstaller-relaunch");

        InitializeConfig();

        // 在任何 XAML 实例创建前设置语言
        if (language != "system") {
            std::wstring lang(language.begin(), language.end());
            lang += L'\0';
            ULONG numLangs = 0;
            SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, lang.c_str(), &numLangs);
        }

        InitializeLogger();

        if (elevatedRun) {
            if (trustedInstallerRelaunch) {
                LOG_INFO(L"", L"Running as TrustedInstaller!");
            }
            else {
                std::wstring relaunchArgs = L"--trustedinstaller-relaunch";
                if (CreateProcessElevated(GetExecutablePath(), true, relaunchArgs)) {
                    LOG_INFO(L"", L"TrustedInstaller relaunch succeeded. Exiting bootstrap process.");
                    LOGGER_SHUTDOWN();
                    Exit();
                    return;
                }
                else {
                    LOG_ERROR(L"", L"Failed to run as TrustedInstaller! See log for more information.");
                }
            }
        }

        if (!InitializeDriver()) {
            LOGGER_SHUTDOWN();
            Exit();
            return;
        }

        window = make<MainWindow>();
        window.Activate();
    }

    void App::InitializeLogger() {
        LOGGER_INIT();
        LOG_INFO(L"", L"Launching Starlight GUI...");
    }

    bool App::InitializeDriver()
    {
        try {
            LOG_INFO(L"Sirius", L"Initializing driver...");

            auto installedPath = GetInstalledLocationPath();
            siriusPath = installedPath + L"\\Assets\\Sirius.sys";
            wtmPath = installedPath + L"\\WindowTopMost.dll";
            iamKeyHackerPath = installedPath + L"\\IAMKeyHacker.dll";

            if (DriverUtils::LoadKernelDriver(siriusPath.c_str())) {
                LOG_INFO(L"Sirius", L"Driver initialized successfully!");
                return true;
            }

            DWORD error = GetLastError();

            if (error == 2 || error == 3) {
                LOG_WARNING(L"Sirius", L"Service exists, but the request returned with error 2/3, indicating that file does not exist. We will delete the service and retry.");
                if (DriverUtils::LoadKernelDriver(siriusPath.c_str())) {
                    LOG_INFO(L"Sirius", L"Driver initialized successfully!");
                    return true;
                }
            }

            hstring message;
            if (error == 98) {
                message = t(L"MainWindow.Driver.FailedHelp1");
            }
            else if (error == 193) {
                message = t(L"MainWindow.Driver.FailedHelp2");
            }
            else {
                message = t(L"MainWindow.Driver.Failed");
            }

            LOG_ERROR(L"Sirius", L"Driver initialization failed! GetLastError() = %d", error);
            MessageBoxW(nullptr, message.c_str(), t(L"Common.Error").c_str(), MB_OK | MB_ICONERROR);
            return false;
        }
        catch (const hresult_error& e) {
            LOG_ERROR(L"Sirius", L"Driver initialization failed! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
            MessageBoxW(nullptr, t(L"MainWindow.Driver.Failed").c_str(), t(L"Common.Error").c_str(), MB_OK | MB_ICONERROR);
            return false;
        }
    }
}
