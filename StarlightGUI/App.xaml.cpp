#include "pch.h"
#include "Utils/Config.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include <shellapi.h>
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

        UnhandledException([](winrt::Windows::Foundation::IInspectable const&,
            winrt::Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e)
            {
                LOG_ERROR(L"App", L"===== Unhandled exception detected! =====");
                LOG_ERROR(L"App", L"Type: 'winrt::hresult_error'");
                LOG_ERROR(L"App", L"Code: %d", e.Exception().value);
                LOG_ERROR(L"App", L"Message: %s", e.Message().c_str());
                LOG_ERROR(L"App", L"=========================================");
                e.Handled(true);
            });
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        ApplyBuildEditionResources();

        bool trustedInstallerRelaunch = HasSwitch(L"--trustedinstaller-relaunch");

        InitializeConfig();

        // Set UI language before any XAML page is created.
        // "system" means follow OS language — don't override MUI.
        if (language != "system") {
            std::wstring lang(language.begin(), language.end());
            lang += L'\0'; // double-null terminated multi-string
            ULONG numLangs = 0;
            SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, lang.c_str(), &numLangs);
        }

        InitializeLogger();

        if (elevated_run) {
            if (trustedInstallerRelaunch) {
                LOG_INFO(L"", L"Running as TrustedInstaller!");
            }
            else {
                std::wstring relaunchArgs = L"--trustedinstaller-relaunch";
                if (CreateProcessElevated(GetExecutablePath(), true, relaunchArgs)) {
                    Exit();
                    return;
                }
                else {
                    LOG_ERROR(L"", L"Failed to run as TrustedInstaller! See log for more information.");
                }
            }
        }

        if (!InitializeDriverBeforeWindow()) {
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

    bool App::InitializeDriverBeforeWindow()
    {
        try {
            LOG_INFO(L"Driver", L"Loading driver before window creation...");

            auto installedPath = GetInstalledLocationPath();
            siriusPath = installedPath + L"\\Assets\\Sirius.sys";
            wtmPath = installedPath + L"\\WindowTopMost.dll";
            iamKeyHackerPath = installedPath + L"\\IAMKeyHacker.dll";

            if (DriverUtils::LoadKernelDriver(siriusPath.c_str())) {
                LOG_INFO(L"Driver", L"Sirius.sys initialized successfully.");
                return true;
            }

            DWORD error = GetLastError();
            hstring message;
            if (error == 2 || error == 98) {
                message = t(L"MainWindow.Driver.FailedHelp1");
            }
            else if (error == 193) {
                message = t(L"MainWindow.Driver.FailedHelp2");
            }
            else {
                message = t(L"MainWindow.Driver.Failed");
            }

            LOG_ERROR(L"Driver", L"Sirius.sys initialization failed, GetLastError() = %d", error);
            MessageBoxW(nullptr, message.c_str(), t(L"Common.Error").c_str(), MB_OK | MB_ICONERROR);
            return false;
        }
        catch (const hresult_error& e) {
            LOG_ERROR(L"Driver", L"Failed to load driver! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
            MessageBoxW(nullptr, t(L"MainWindow.Driver.Failed").c_str(), t(L"Common.Error").c_str(), MB_OK | MB_ICONERROR);
            return false;
        }
    }
}
