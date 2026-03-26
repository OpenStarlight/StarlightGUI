#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <winrt/Microsoft.UI.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <commctrl.h>
#include <shellapi.h>
#include "resource.h"
#include "UpdateDialog.xaml.h"
#include "FilePage.xaml.h"

using namespace winrt;
using namespace WinUI3Package;
using namespace Windows::UI;
using namespace Windows::Graphics;
using namespace Windows::Web::Http;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::ApplicationModel;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Composition::SystemBackdrops;

namespace winrt::StarlightGUI::implementation
{
    MainWindow* g_mainWindowInstance = nullptr;

    MainWindow::MainWindow()
    {
        InitializeComponent();

        NavHomeUid().Content(tbox(L"Nav_Home.Content"));
        NavTaskUid().Content(tbox(L"Nav_Task.Content"));
        NavKernelModuleUid().Content(tbox(L"Nav_KernelModule.Content"));
        NavFileUid().Content(tbox(L"Nav_File.Content"));
        NavWindowUid().Content(tbox(L"Nav_Window.Content"));
        NavUtilityUid().Content(tbox(L"Nav_Utility.Content"));
        NavMonitorUid().Content(tbox(L"Nav_Monitor.Content"));
        NavDisasmUid().Content(tbox(L"Nav_Disasm.Content"));
        NavHelpUid().Content(tbox(L"Nav_Help.Content"));

        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND hWnd{ 0 };
        windowNative->get_WindowHandle(&hWnd);
        globalHWND = hWnd;

        LOG_INFO(L"MainWindow", L"Initializing AppWindow interface...");
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
        AppWindow().TitleBar().PreferredHeightOption(winrt::Microsoft::UI::Windowing::TitleBarHeightOption::Tall);
        AppWindow().SetIcon(GetInstalledLocationPath() + L"\\Assets\\Starlight.ico");
        SetWindowSubclass(hWnd, &MainWindowProc, 1, reinterpret_cast<DWORD_PTR>(this));

		// 允许拖放和复制数据到窗口
        CHANGEFILTERSTRUCT cfs{};
        cfs.cbSize = sizeof(cfs);
        ChangeWindowMessageFilterEx(hWnd, WM_DROPFILES, MSGFLT_ALLOW, &cfs);
        ChangeWindowMessageFilterEx(hWnd, WM_COPYDATA, MSGFLT_ALLOW, &cfs);
        ChangeWindowMessageFilterEx(hWnd, 0x0049 /* WM_COPYGLOBALDATA */, MSGFLT_ALLOW, &cfs);
        DragAcceptFiles(hWnd, TRUE);

        // 恢复窗口大小
        int32_t width = ReadConfig("window_width", 1200);
        int32_t height = ReadConfig("window_height", 800);
        AppWindow().Resize(SizeInt32{ width, height });

        // 外观
        LoadBackdrop();
        LoadBackground();
        LoadNavigation();

        g_mainWindowInstance = this;

        // 显示托盘图标
        if (tray_background_run) {
            InitializeTrayIcon();
        }

        Activated([this](auto&&, auto&&) -> IAsyncAction {
            if (!loaded) {
                RootNavigation().IsEnabled(false);
                loaded = true;
                // 加载模块
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Info").c_str(), GetLocalizedString(L"MainWindow_LoadingModules").c_str(), InfoBarSeverity::Informational, g_mainWindowInstance);
                co_await LoadModules();

                // 进入主页
                if (navigate_task_request) {
                    NavigateToTaskPage();
                    navigate_task_request = false;
                }
                else {
                    LOG_INFO(L"MainWindow", L"Navigates to StarlightGUI::HomePage because we are initializing MainWindow for the first time.");
                    MainFrame().Navigate(xaml_typename<StarlightGUI::HomePage>());
                    RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
                }

                RootNavigation().IsEnabled(true);

                // 检查更新
                CheckUpdate();
                LOG_INFO(L"MainWindow", L"Completed all loading-stage tasks.");
            }
            });


        Closed([this](auto&&, auto&&) {
            int32_t width = this->AppWindow().Size().Width;
            int32_t height = this->AppWindow().Size().Height;

            SaveConfig("window_width", width);
            SaveConfig("window_height", height);

            LOG_INFO(L"MainWindow", L"Saved window size.");
            RemoveTrayIcon();
            LOGGER_CLOSE();
            });

        LOG_INFO(L"MainWindow", L"MainWindow initialized.");
    }

    MainWindow::~MainWindow()
    {
        RemoveTrayIcon();

        for (auto& window : m_openWindows) {
            if (window) {
                window.Close();
            }
        }
    }

    void MainWindow::SetTrayBackgroundRun(bool enabled)
    {
        tray_background_run = enabled;
        if (enabled) {
            InitializeTrayIcon();
        }
        else {
            RemoveTrayIcon();
            m_allowClose = false;
        }
    }

    void MainWindow::NavigateToTaskPage()
    {
        MainFrame().Navigate(xaml_typename<StarlightGUI::TaskPage>());
        RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(1));
    }

    void MainWindow::InitializeTrayIcon()
    {
        if (m_trayIconAdded || !globalHWND) return;

        m_notifyIconData = {};
        m_notifyIconData.cbSize = sizeof(NOTIFYICONDATAW);
        m_notifyIconData.hWnd = globalHWND;
        m_notifyIconData.uID = TRAY_ID;
        m_notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        m_notifyIconData.uCallbackMessage = WM_TRAYICON;
        m_notifyIconData.hIcon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));

        if (!m_notifyIconData.hIcon) {
            m_notifyIconData.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        }

        wcscpy_s(m_notifyIconData.szTip, L"Starlight GUI");

        if (Shell_NotifyIconW(NIM_ADD, &m_notifyIconData)) {
            m_notifyIconData.uVersion = NOTIFYICON_VERSION;
            Shell_NotifyIconW(NIM_SETVERSION, &m_notifyIconData);
            m_trayIconAdded = true;
            LOG_INFO(L"MainWindow", L"Initialized tray icon.");
        }
    }

    void MainWindow::RemoveTrayIcon()
    {
        if (!m_trayIconAdded) return;
        Shell_NotifyIconW(NIM_DELETE, &m_notifyIconData);
        m_trayIconAdded = false;
    }

    void MainWindow::HideWindowToTray()
    {
        if (!m_trayIconAdded) {
            InitializeTrayIcon();
        }
        ShowWindow(globalHWND, SW_HIDE);
    }

    void MainWindow::RestoreWindowFromTray()
    {
        ShowWindow(globalHWND, SW_SHOW);
        ShowWindow(globalHWND, SW_RESTORE);
        Activate();
        SetForegroundWindow(globalHWND);
    }

    void MainWindow::ShowTrayMenu()
    {
        HMENU hMenu = CreatePopupMenu();
        if (!hMenu) return;

        AppendMenuW(hMenu, MF_STRING, TRAY_CMD_RESTORE, GetLocalizedString(L"MainWindow_OpenMainWindow").c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, TRAY_CMD_EXIT, GetLocalizedString(L"MainWindow_Exit").c_str());

        POINT point{};
        GetCursorPos(&point);
        SetForegroundWindow(globalHWND);

        auto command = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD, point.x, point.y, 0, globalHWND, nullptr);

        DestroyMenu(hMenu);

        if (command == TRAY_CMD_RESTORE) {
            RestoreWindowFromTray();
            return;
        }

        if (command == TRAY_CMD_EXIT) {
            m_allowClose = true;
            PostMessageW(globalHWND, WM_CLOSE, 0, 0);
        }
    }

    void MainWindow::RootNavigation_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs args)
    {
        if (args.IsSettingsInvoked())
        {
            MainFrame().Navigate(xaml_typename<StarlightGUI::SettingsPage>());
            return;
        }

        auto invokedItem = unbox_value<winrt::hstring>(args.InvokedItemContainer().Tag());

        if (invokedItem == L"Home")
        {
            MainFrame().Navigate(xaml_typename<StarlightGUI::HomePage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
        }
        else if (invokedItem == L"Task") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::TaskPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(1));
        }
        else if (invokedItem == L"KernelModule") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::KernelModulePage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(2));
        }
        else if (invokedItem == L"File") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::FilePage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(3));
        }
        else if (invokedItem == L"Window") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::WindowPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(4));
        }
        else if (invokedItem == L"Utility") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::UtilityPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(5));
        }
        else if (invokedItem == L"Monitor") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::MonitorPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(6));
        }
        else if (invokedItem == L"Disasm") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::DisasmPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(7));
        }
        else if (invokedItem == L"Deuterium") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::DeuteriumPage>());
            RootNavigation().SelectedItem(RootNavigation().FooterMenuItems().GetAt(0));
        }
        else if (invokedItem == L"Help") {
            MainFrame().Navigate(xaml_typename<StarlightGUI::HelpPage>());
            RootNavigation().SelectedItem(RootNavigation().FooterMenuItems().GetAt(1));
        }
    }

    slg::coroutine MainWindow::LoadBackdrop()
    {
        int option = -1;

        if (background_type == 1) {
            CustomMicaBackdrop micaBackdrop = CustomMicaBackdrop();

            this->SystemBackdrop(micaBackdrop);

            option = mica_type;
            if (option == 0) {
                micaBackdrop.Kind(MicaKind::Base);
            }
            else {
                micaBackdrop.Kind(MicaKind::BaseAlt);
            }
        }
        else if (background_type == 2) {
            CustomAcrylicBackdrop acrylicBackdrop = CustomAcrylicBackdrop();

            this->SystemBackdrop(acrylicBackdrop);

            option = acrylic_type;
            if (option == 1) {
                acrylicBackdrop.Kind(DesktopAcrylicKind::Base);
            }
            else if (option == 2) {
                acrylicBackdrop.Kind(DesktopAcrylicKind::Thin);
            }
            else {
                acrylicBackdrop.Kind(DesktopAcrylicKind::Default);
            }
        }
        else
        {
            this->SystemBackdrop(nullptr);
        }

        LOG_INFO(L"MainWindow", L"Loaded backdrop async with options: [%d, %d]", background_type, option);
        co_return;
    }

    slg::coroutine MainWindow::LoadBackground()
    {
        if (background_image.empty()) {
            SolidColorBrush brush;
            brush.Color(Colors::Transparent());

            MainWindowGrid().Background(brush);
            co_return;
        }

        HANDLE hFile = CreateFileA(background_image.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);

            try {
                StorageFile file = co_await StorageFile::GetFileFromPathAsync(to_hstring(background_image));

                if (file && file.IsAvailable() && (file.FileType() == L".png" || file.FileType() == L".jpg" || file.FileType() == L".bmp" || file.FileType() == L".jpeg")) {
                    ImageBrush brush;
                    BitmapImage bitmapImage;
                    auto stream = co_await file.OpenReadAsync();
                    bitmapImage.SetSource(stream);
                    brush.ImageSource(bitmapImage);

                    brush.Stretch(image_stretch == 0 ? Stretch::None : image_stretch == 2 ? Stretch::Uniform : image_stretch == 1 ? Stretch::Fill : Stretch::UniformToFill);
                    brush.Opacity(image_opacity / 100.0);

                    MainWindowGrid().Background(brush);

                    LOG_INFO(L"MainWindow", L"Loaded background async with options: [%s, %d, %d]", to_hstring(background_image).c_str(), image_opacity, image_stretch);
                }
            }
            catch (hresult_error) {
                SolidColorBrush brush;
                brush.Color(Colors::Transparent());

                MainWindowGrid().Background(brush);
                LOG_ERROR(L"MainWindow", L"Unable to load window backgroud! Applying transparent brush instead.");
            }
        }
        else {
            SaveConfig("background_image", ""); // 保存一次空的，后面不再检查
            SolidColorBrush brush;
            brush.Color(Colors::Transparent());

            MainWindowGrid().Background(brush);
            LOG_ERROR(L"MainWindow", L"Background file does not exist. Applying transparent brush instead.");
        }
        co_return;
    }

    slg::coroutine MainWindow::LoadNavigation()
    {
        if (navigation_style == 1) {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::Left);
        }
        else if (navigation_style == 2) {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::Top);
        }
        else
        {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::LeftCompact);
        }

        LOG_INFO(L"MainWindow", L"Loaded navigation async with options: [%d]", navigation_style);
        co_return;
    }

    IAsyncAction MainWindow::CheckUpdate()
    {
        try {
            auto weak_this = get_weak();

            int currentBuildNumber = unbox_value<int>(Application::Current().Resources().TryLookup(box_value(L"BuildNumber")));
            int latestBuildNumber = 0;

            if (auto strong_this = weak_this.get()) {
                co_await winrt::resume_background();

                HttpClient client;
                Uri uri(L"https://pastebin.com/raw/kz5qViYF");

                // 防止获取旧数据
                client.DefaultRequestHeaders().Append(L"Cache-Control", L"no-cache");
                client.DefaultRequestHeaders().Append(L"If-None-Match", L"");

                LOG_INFO(L"Updater", L"Sending update check request...");
                auto result = co_await client.GetStringAsync(uri);

                auto json = Windows::Data::Json::JsonObject::Parse(result);
                latestBuildNumber = json.GetNamedNumber(L"build_number");

                co_await wil::resume_foreground(DispatcherQueue());

                LOG_INFO(L"Updater", L"Current: %d, Latest: %d", currentBuildNumber, latestBuildNumber);

                if (ReadConfig("last_announcement_date", 0) < GetDateAsInt()) {
                    auto dialog = winrt::make<winrt::StarlightGUI::implementation::UpdateDialog>();
                    dialog.IsUpdate(false);
                    dialog.LatestVersion(json.GetNamedString(L"an_update_time"));
                    dialog.SetAnLine(1, json.GetNamedString(L"an_line1"));
                    dialog.SetAnLine(2, json.GetNamedString(L"an_line2"));
                    dialog.SetAnLine(3, json.GetNamedString(L"an_line3"));
                    dialog.XamlRoot(MainWindowGrid().XamlRoot());
                    co_await dialog.ShowAsync();
                }

                if (!ReadConfig("check_update", true)) co_return;

                if (latestBuildNumber == 0) {
                    LOG_WARNING(L"Updater", L"Latest = 0, check failed.");
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"MainWindow_CheckUpdateFailed").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
                }
                else if (latestBuildNumber == currentBuildNumber) {
                    LOG_INFO(L"Updater", L"Latest = current, we are on the latest version.");
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Info").c_str(), GetLocalizedString(L"MainWindow_LatestVersion").c_str(), InfoBarSeverity::Informational, g_mainWindowInstance);
                }
                else if (latestBuildNumber > currentBuildNumber) {
                    LOG_INFO(L"Updater", L"Latest > current, new version avaliable. Calling up update dialog.");
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Info").c_str(), GetLocalizedString(L"MainWindow_NewVersionAvailable").c_str(), InfoBarSeverity::Informational, g_mainWindowInstance);
                    auto dialog = winrt::make<winrt::StarlightGUI::implementation::UpdateDialog>();
                    dialog.IsUpdate(true);
                    dialog.LatestVersion(json.GetNamedString(L"version"));
                    dialog.XamlRoot(MainWindowGrid().XamlRoot());

                    auto result = co_await dialog.ShowAsync();

                    if (result == ContentDialogResult::Primary) {
                        Uri target(json.GetNamedString(L"download_link"));
                        auto result = co_await Launcher::LaunchUriAsync(target);

                        if (result) {
                            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"MainWindow_OpenedInBrowser").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                        }
                        else {
                            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"MainWindow_OpenBrowserFailed").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                        }
                    }
                }
                else if (latestBuildNumber < currentBuildNumber) {
                    LOG_INFO(L"Updater", L"Latest < current, maybe we are on a dev environment.", kernelPath.c_str());
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Info").c_str(), GetLocalizedString(L"MainWindow_DevVersion").c_str(), InfoBarSeverity::Informational, g_mainWindowInstance);
                }
            }
        } 
        catch (const hresult_error& e) {
            LOG_ERROR(L"Updater", L"Failed to check update! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
        }

        co_return;
    }

    IAsyncAction MainWindow::LoadModules() {
        if (kernelPath.empty() || astralPath.empty() || wtmPath.empty() || iamKeyHackerPath.empty()) {
            try {
                co_await winrt::resume_background();

                LOG_INFO(L"Driver", L"Loading necessary modules...");

                int failedCount = 0;
                auto kernelFile = co_await StorageFile::GetFileFromPathAsync(GetInstalledLocationPath() + L"\\Assets\\kernel.sys");
                auto astralFile = co_await StorageFile::GetFileFromPathAsync(GetInstalledLocationPath() + L"\\Assets\\AstralX.sys");
                auto wtmFile = co_await StorageFile::GetFileFromPathAsync(GetInstalledLocationPath() + L"\\WindowTopMost.dll");
                auto iamKeyHackerFile = co_await StorageFile::GetFileFromPathAsync(GetInstalledLocationPath() + L"\\IAMKeyHacker.dll");

                kernelPath = kernelFile.Path();
                astralPath = astralFile.Path();
                wtmPath = wtmFile.Path();
                iamKeyHackerPath = iamKeyHackerFile.Path();

				// 快速对于常见报错码进行判断，减少用户困惑

                if (DriverUtils::LoadKernelDriver(kernelPath.c_str(), unused)) {
                    LOG_INFO(L"Driver", L"kernel.sys initializated successfully.");
                }
                else {
                    failedCount++;
                    LOG_INFO(L"Driver", L"kernel.sys initialization result: %s, GetLastError() = %d", unused.c_str(), GetLastError());

                    if (GetLastError() == 2 || GetLastError() == 98) {
                        co_await wil::resume_foreground(DispatcherQueue());
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"MainWindow_FailedHelp1").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                        co_await winrt::resume_background();
                    }
                    else if (GetLastError() == 193) {
                        co_await wil::resume_foreground(DispatcherQueue());
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"MainWindow_FailedHelp2").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                        co_await winrt::resume_background();
                    }
                }


                if (DriverUtils::LoadDriver(astralPath.c_str(), L"AstralX", unused)) {
                    LOG_INFO(L"Driver", L"AstralX.sys initializated successfully.");
                }
                else {
                    failedCount++;
                    LOG_INFO(L"Driver", L"AstralX.sys initialization result: %s, GetLastError() = %d", unused.c_str(), GetLastError());

                    if (GetLastError() == 2 || GetLastError() == 98) {
                        co_await wil::resume_foreground(DispatcherQueue());
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"MainWindow_FailedHelp1").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                    }
                    else if (GetLastError() == 193) {
                        co_await wil::resume_foreground(DispatcherQueue());
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"MainWindow_FailedHelp2").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                    }
                }

                co_await wil::resume_foreground(DispatcherQueue());

                if (failedCount > 0) {
                    LOG_ERROR(L"Driver", L"%d module(s) failed to load.", failedCount);
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"MainWindow_ModuleLoadFailed").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                }
                else {
                    LOG_INFO(L"Driver", L"Modules loaded successfully.", kernelPath.c_str());
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"MainWindow_ModuleLoadSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                }
            }
            catch (const hresult_error& e) {
                LOG_ERROR(L"Driver", L"Failed to load modules! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"MainWindow_ModuleLoadFailed").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            }
        }
    }

    HWND MainWindow::GetWindowHandle()
    {
        return globalHWND;
    }

    LRESULT CALLBACK MainWindow::MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        auto instance = reinterpret_cast<MainWindow*>(dwRefData);

        switch (uMsg)
        {
        case WM_CLOSE:
        {
            if (instance && tray_background_run && !instance->m_allowClose) {
                instance->HideWindowToTray();
                if (instance->m_trayIconAdded) return 0;
            }
            break;
        }

        case WM_TRAYICON:
        {
            if (!instance) return 0;

            auto trayMsg = LOWORD(lParam);

            if (trayMsg == WM_LBUTTONUP || trayMsg == WM_LBUTTONDBLCLK || trayMsg == NIN_SELECT || trayMsg == NIN_KEYSELECT) {
                instance->RestoreWindowFromTray();
                return 0;
            }

            if (trayMsg == WM_RBUTTONUP || trayMsg == WM_CONTEXTMENU) {
                instance->ShowTrayMenu();
                return 0;
            }

            break;
        }

        case WM_DROPFILES:
        {
            auto hDrop = reinterpret_cast<HDROP>(wParam);
            if (!hDrop) return 0;

            UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            std::vector<std::wstring> paths;
            paths.reserve(fileCount);

            for (UINT i = 0; i < fileCount; ++i) {
                UINT len = DragQueryFileW(hDrop, i, nullptr, 0);
                if (len == 0) continue;
                std::wstring path(len + 1, L'\0');
                DragQueryFileW(hDrop, i, path.data(), len + 1);
                path.resize(len);
                paths.push_back(path);
            }

            DragFinish(hDrop);

            if (!paths.empty() && g_filePageInstance) {
                g_filePageInstance->HandleExternalDropFiles(paths);
            }

            return 0;
        }

        case WM_COPYDATA:
        {
            if (!instance) return 0;

            auto copyData = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
            if (!copyData || !copyData->lpData) return 0;

            auto command = std::wstring(reinterpret_cast<const wchar_t*>(copyData->lpData));
            if (copyData->dwData == COPYDATA_NAVIGATE_TASK && command == L"navigate_task") {
                navigate_task_request = true;
                instance->RestoreWindowFromTray();
                instance->NavigateToTaskPage();
                return 1;
            }

            return 0;
        }

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            pMinMaxInfo->ptMinTrackSize.x = 800;
            pMinMaxInfo->ptMinTrackSize.y = 600;
            return 0;
        }

        case WM_NCDESTROY:
        {
            if (instance) {
                instance->RemoveTrayIcon();
            }
            RemoveWindowSubclass(hWnd, &MainWindowProc, uIdSubclass);
            break;
        }
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}



