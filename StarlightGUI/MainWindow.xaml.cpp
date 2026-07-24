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
#include <winrt/Microsoft.UI.Xaml.Navigation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/WinUI3Package.h>
#include <wil/cppwinrt_helpers.h>
#include <microsoft.ui.xaml.window.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include "resource.h"
#include "UpdateDialog.xaml.h"
#include "FilePage.xaml.h"
#include "Utils/CppUtils.h"
#include "Utils/Config.h"
#include "Utils/KernelBase.h"
#include "Utils/Utils.h"

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
        g_mainWindowInstance = this;
        slg::ApplyConfiguredTheme();
        SetupLocalization();

        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND windowHandle{ 0 };
        windowNative->get_WindowHandle(&windowHandle);
        globalWindowHandle = windowHandle;

        LOG_INFO(L"MainWindow", L"Initializing AppWindow interface...");
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
        AppWindow().TitleBar().PreferredHeightOption(winrt::Microsoft::UI::Windowing::TitleBarHeightOption::Tall);
        AppWindow().SetIcon(GetInstalledLocationPath() + L"\\Assets\\Starlight.ico");
        SetWindowSubclass(windowHandle, &MainWindowProc, 1, (DWORD_PTR)this);

		// 允许拖放和复制数据到窗口
        CHANGEFILTERSTRUCT cfs{};
        cfs.cbSize = sizeof(cfs);
        ChangeWindowMessageFilterEx(windowHandle, WM_DROPFILES, MSGFLT_ALLOW, &cfs);
        ChangeWindowMessageFilterEx(windowHandle, WM_COPYDATA, MSGFLT_ALLOW, &cfs);
        ChangeWindowMessageFilterEx(windowHandle, 0x0049, MSGFLT_ALLOW, &cfs);
        DragAcceptFiles(windowHandle, TRUE);

        // 恢复窗口大小
        int32_t width = ReadConfig("window_width", 1200);
        int32_t height = ReadConfig("window_height", 800);
        AppWindow().Resize(SizeInt32{ width, height });

        // 外观
        LoadBackdrop();
        LoadBackground();
        LoadNavigation();

        // 显示托盘图标
        if (trayBackgroundRun) {
            InitializeTrayIcon();
        }

        Activated([this](auto&&, auto&&) {
            if (!loaded) {
                loaded = true;

                // 进入主页
                LOG_INFO(L"MainWindow", L"Navigates to StarlightGUI::HomePage because we are initializing MainWindow for the first time.");
                MainFrame().Navigate(xaml_typename<StarlightGUI::HomePage>());
                RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));

                // 检查更新
                m_updateAction = CheckUpdate();
                LOG_INFO(L"MainWindow", L"Completed all loading-stage tasks.");
            }
            });


        Closed([this](auto&&, auto&&) {
            LOG_INFO(L"MainWindow", L"Closed event received. Flushing logger before process shutdown.");

            int32_t width = this->AppWindow().Size().Width;
            int32_t height = this->AppWindow().Size().Height;

            SaveConfig("window_width", width);
            SaveConfig("window_height", height);

            LOG_INFO(L"MainWindow", L"Saved window size.");
            if (autoStopDriver) {
                if (DriverUtils::StopKernelDriver()) {
                    LOG_INFO(L"Driver", L"%s", L"Sirius service stopped automatically.");
                }
                else {
                    LOG_WARNING(L"Driver", L"%s", L"Failed to stop Sirius service automatically.");
                }
            }
            RemoveTrayIcon();
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
        trayBackgroundRun = enabled;
        if (enabled) {
            InitializeTrayIcon();
        }
        else {
            RemoveTrayIcon();
            m_allowClose = false;
        }
    }

    void MainWindow::InitializeTrayIcon()
    {
        if (m_trayIconAdded || !globalWindowHandle) return;

        m_notifyIconData = {};
        m_notifyIconData.cbSize = sizeof(NOTIFYICONDATAW);
        m_notifyIconData.hWnd = globalWindowHandle;
        m_notifyIconData.uID = TRAY_ID;
        m_notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        m_notifyIconData.uCallbackMessage = WM_TRAYICON;
        m_notifyIconData.hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED);

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
        ShowWindow(globalWindowHandle, SW_HIDE);
    }

    void MainWindow::RestoreWindowFromTray()
    {
        ShowWindow(globalWindowHandle, SW_SHOW);
        ShowWindow(globalWindowHandle, SW_RESTORE);
        Activate();
        SetForegroundWindow(globalWindowHandle);
    }

    void MainWindow::ShowTrayMenu()
    {
        HMENU menuHandle = CreatePopupMenu();
        if (!menuHandle) return;

        AppendMenuW(menuHandle, MF_STRING, TRAY_CMD_RESTORE, t(L"MainWindow.Tray.Open").c_str());
        AppendMenuW(menuHandle, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menuHandle, MF_STRING, TRAY_CMD_EXIT, t(L"Common.Exit").c_str());

        POINT point{};
        GetCursorPos(&point);
        SetForegroundWindow(globalWindowHandle);

        auto command = TrackPopupMenu(menuHandle, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD, point.x, point.y, 0, globalWindowHandle, nullptr);

        DestroyMenu(menuHandle);

        if (command == TRAY_CMD_RESTORE) {
            RestoreWindowFromTray();
            return;
        }

        if (command == TRAY_CMD_EXIT) {
            m_allowClose = true;
            PostMessageW(globalWindowHandle, WM_CLOSE, 0, 0);
        }
    }

    void MainWindow::RootNavigation_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs args)
    {
		Navigation::FrameNavigationOptions options{};
        options.TransitionInfoOverride(args.RecommendedNavigationTransitionInfo());
        if (args.IsSettingsInvoked())
        {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::SettingsPage>(), nullptr, options);
            return;
        }

        auto invokedItem = unbox_value<winrt::hstring>(args.InvokedItemContainer().Tag());

        if (invokedItem == L"Home")
        {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::HomePage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
        }
        else if (invokedItem == L"Task") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::TaskPage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(1));
        }
        else if (invokedItem == L"KernelModule") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::KernelModulePage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(2));
        }
        else if (invokedItem == L"File") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::FilePage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(3));
        }
        else if (invokedItem == L"Window") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::WindowPage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(4));
        }
        else if (invokedItem == L"Utility") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::UtilityPage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(5));
        }
        else if (invokedItem == L"Monitor") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::MonitorPage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(6));
        }
        else if (invokedItem == L"Disasm") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::DisasmPage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(7));
        }
        else if (invokedItem == L"Help") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::HelpPage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().FooterMenuItems().GetAt(0));
        }
        else if (invokedItem == L"Settings") {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::SettingsPage>(), nullptr, options);
            RootNavigation().SelectedItem(RootNavigation().FooterMenuItems().GetAt(1));
        }
    }

    void MainWindow::AppTitleBar_PaneToggleRequested(Microsoft::UI::Xaml::Controls::TitleBar, winrt::Windows::Foundation::IInspectable const&)
    {
        if (RootNavigation().PaneDisplayMode() == NavigationViewPaneDisplayMode::Top) {
            RootNavigation().IsPaneOpen(false);
            return;
        }

        RootNavigation().IsPaneOpen(!RootNavigation().IsPaneOpen());
    }

    slg::coroutine MainWindow::LoadBackdrop()
    {
        int option = -1;

        if (backgroundType == 1) {
            static MicaBackdrop micaBackdrop = MicaBackdrop();

            this->SystemBackdrop(micaBackdrop);

            option = micaType;
            if (option == 0) {
                micaBackdrop.Kind(MicaKind::Base);
            }
            else {
                micaBackdrop.Kind(MicaKind::BaseAlt);
            }

            MainWindowGrid().Background(nullptr);
        }
        else if (backgroundType == 2) {
            static CustomAcrylicBackdrop acrylicBackdrop = CustomAcrylicBackdrop();

            this->SystemBackdrop(acrylicBackdrop);

            acrylicBackdrop.RequestedTheme(slg::GetConfiguredElementTheme());
            option = acrylicType;
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
            if (backgroundImage.empty()) {
                MainWindowGrid().Background(SolidColorBrush(slg::GetConfiguredElementTheme() == ElementTheme::Dark ? Color{ 255,32,32,32 } : Color{ 255,243,243,243 }));
            }
        }

        LOG_INFO(L"MainWindow", L"Loaded backdrop async with options: [%d, %d]", backgroundType, option);
        co_return;
    }

    slg::coroutine MainWindow::LoadBackground()
    {
        if (backgroundImage.empty()) {
            // 先清空然后重新加载一次背景色，防止纯色背景被覆盖
            MainWindowGrid().Background(nullptr);
            LoadBackdrop();
            co_return;
        }

        HANDLE fileHandle = CreateFileA(backgroundImage.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(fileHandle);

            try {
                StorageFile file = co_await StorageFile::GetFileFromPathAsync(to_hstring(backgroundImage));

                if (file && file.IsAvailable() && (file.FileType() == L".png" || file.FileType() == L".jpg" || file.FileType() == L".bmp" || file.FileType() == L".jpeg")) {
                    ImageBrush brush;
                    BitmapImage bitmapImage;
                    auto stream = co_await file.OpenReadAsync();
                    bitmapImage.SetSource(stream);
                    brush.ImageSource(bitmapImage);

                    brush.Stretch(imageStretch == 0 ? Stretch::None : imageStretch == 2 ? Stretch::Uniform : imageStretch == 1 ? Stretch::Fill : Stretch::UniformToFill);
                    brush.Opacity(imageOpacity / 100.0);

                    MainWindowGrid().Background(brush);

                    LOG_INFO(L"MainWindow", L"Loaded background async with options: [%s, %d, %d]", to_hstring(backgroundImage).c_str(), imageOpacity, imageStretch);
                }
            }
            catch (hresult_error) {
                // 先清空然后重新加载一次背景色，防止纯色背景被覆盖
                MainWindowGrid().Background(nullptr);
                LoadBackdrop();
                LOG_ERROR(L"MainWindow", L"Unable to load window backgroud! Applying transparent brush instead.");
            }
        }
        else {
            // 保存一次空的，后面不再检查
            backgroundImage.clear();
            SaveConfig("background_image", "");
            // 先清空然后重新加载一次背景色，防止纯色背景被覆盖
            MainWindowGrid().Background(nullptr);
            LoadBackdrop();
            LOG_ERROR(L"MainWindow", L"Background file does not exist. Applying transparent brush instead.");
        }
        co_return;
    }

    slg::coroutine MainWindow::LoadNavigation()
    {
        AppTitleBar().IsPaneToggleButtonVisible(true);

        if (navigationStyle == 1) {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::Left);
        }
        else if (navigationStyle == 2) {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::Top);
            RootNavigation().IsPaneOpen(false);
        }
        else
        {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::LeftCompact);
        }

        LOG_INFO(L"MainWindow", L"Loaded navigation async with options: [%d]", navigationStyle);
        co_return;
    }

    IAsyncAction MainWindow::CheckUpdate()
    {
        try {
            if (!ReadConfig("check_update", true)) co_return;

            auto lifetime = get_strong();
            auto dispatcher = DispatcherQueue();

            int currentBuildNumber = unbox_value<int>(Application::Current().Resources().TryLookup(box_value(L"BuildNumber")));
            int latestBuildNumber = 0;

            co_await winrt::resume_background();

            HttpClient client;
            Uri uri(L"https://pastebin.com/raw/kz5qViYF");

            // 防止获取旧数据
            client.DefaultRequestHeaders().Append(L"Cache-Control", L"no-cache");

            LOG_INFO(L"Updater", L"Sending update check request...");

            auto result = co_await client.GetStringAsync(uri);
            LOG_INFO(L"Updater", L"Update response received! Payload length: %zu.", result.size());

            auto json = Windows::Data::Json::JsonObject::Parse(result);
            latestBuildNumber = (int)json.GetNamedNumber(L"build_number");
            LOG_INFO(L"Updater", L"Update response parsed successfully!");

            co_await wil::resume_foreground(dispatcher);

            LOG_INFO(L"Updater", L"Current: %d, Latest: %d", currentBuildNumber, latestBuildNumber);

            if (ReadConfig("last_announcement_date", 0) < GetDateAsInt()) {
                hstring announcement;

                if (json.HasKey(L"announcement")) {
                    announcement = json.GetNamedString(L"announcement");
                }
                else {
                    std::wstring legacyAnnouncement;

                    for (int i = 1; i <= 3; ++i) {
                        std::wstring key = L"an_line" + std::to_wstring(i);

                        if (!json.HasKey(hstring{ key })) continue;

                        std::wstring line{ json.GetNamedString(hstring{ key }) };
                        if (line.empty()) continue;

                        if (!legacyAnnouncement.empty()) legacyAnnouncement += L"\n";
                        legacyAnnouncement += line;
                    }

                    announcement = hstring{ legacyAnnouncement };
                }

                auto dialog = winrt::make<winrt::StarlightGUI::implementation::UpdateDialog>();
                dialog.IsUpdate(false);
                dialog.LatestVersion(json.GetNamedString(L"an_update_time"));
                dialog.Announcement(announcement);
                dialog.XamlRoot(MainWindowGrid().XamlRoot());
                co_await dialog.ShowAsync();
            }

            if (latestBuildNumber == 0) {
                LOG_WARNING(L"Updater", L"Latest = 0, check failed.");
                slg::CreateInfoBarAndDisplay(t(L"Common.Warning"), t(L"MainWindow.Updater.Failed"), InfoBarSeverity::Warning, g_mainWindowInstance);
            }
            else if (latestBuildNumber == currentBuildNumber) {
                LOG_INFO(L"Updater", L"Latest = current, we are on the latest version.");
                slg::CreateInfoBarAndDisplay(t(L"Common.Info"), t(L"MainWindow.Updater.Latest"), InfoBarSeverity::Informational, g_mainWindowInstance);
            }
            else if (latestBuildNumber > currentBuildNumber) {
                LOG_INFO(L"Updater", L"Latest > current, new version avaliable. Calling up update dialog.");
                slg::CreateInfoBarAndDisplay(t(L"Common.Info"), t(L"MainWindow.Updater.New"), InfoBarSeverity::Informational, g_mainWindowInstance);
                auto dialog = winrt::make<winrt::StarlightGUI::implementation::UpdateDialog>();
                dialog.IsUpdate(true);
                dialog.LatestVersion(json.GetNamedString(L"version"));
                dialog.XamlRoot(MainWindowGrid().XamlRoot());

                auto dialogResult = co_await dialog.ShowAsync();

                if (dialogResult == ContentDialogResult::Primary) {
                    Uri target(json.GetNamedString(L"download_link"));
                    auto launchResult = co_await Launcher::LaunchUriAsync(target);

                    if (launchResult) {
                        slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.OpenInBrowser.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
                    }
                    else {
                        slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.OpenInBrowser.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
                    }
                }
            }
            else if (latestBuildNumber < currentBuildNumber) {
                LOG_INFO(L"Updater", L"Latest < current, maybe we are on a dev environment.");
                slg::CreateInfoBarAndDisplay(t(L"Common.Info"), t(L"MainWindow.Updater.Dev"), InfoBarSeverity::Informational, g_mainWindowInstance);
            }
        }
        catch (const hresult_error& e) {
            LOG_ERROR(L"Updater", L"Failed to check update! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
        }
        catch (const std::exception& e) {
            LOG_ERROR(L"Updater", L"Failed to check update! std::exception: %hs", e.what());
        }
        catch (...) {
            LOG_ERROR(L"Updater", L"Failed to check update! Unknown exception.");
        }

        co_return;
    }

    HWND MainWindow::GetWindowHandle()
    {
        return globalWindowHandle;
    }

    LRESULT CALLBACK MainWindow::MainWindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR referenceData)
    {
        auto instance = (MainWindow*)referenceData;

        switch (message)
        {
        case WM_CLOSE:
        {
            if (instance && trayBackgroundRun && !instance->m_allowClose) {
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
            auto dropHandle = (HDROP)wParam;
            if (!dropHandle) return 0;

            UINT fileCount = DragQueryFileW(dropHandle, 0xFFFFFFFF, nullptr, 0);
            std::vector<std::wstring> paths;
            paths.reserve(fileCount);

            for (UINT i = 0; i < fileCount; ++i) {
                UINT len = DragQueryFileW(dropHandle, i, nullptr, 0);
                if (len == 0) continue;
                std::wstring path(len + 1, L'\0');
                DragQueryFileW(dropHandle, i, path.data(), len + 1);
                path.resize(len);
                paths.push_back(path);
            }

            DragFinish(dropHandle);

            if (!paths.empty() && g_filePageInstance) {
                g_filePageInstance->HandleExternalDropFiles(paths);
            }

            return 0;
        }

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;
            minMaxInfo->ptMinTrackSize.x = 800;
            minMaxInfo->ptMinTrackSize.y = 600;
            return 0;
        };

        case WM_NCDESTROY:
        {
            if (instance) {
                instance->RemoveTrayIcon();
            }
            RemoveWindowSubclass(windowHandle, &MainWindowProc, subclassId);
            LOGGER_SHUTDOWN();
            break;
        }
        }
        return DefSubclassProc(windowHandle, message, wParam, lParam);
    }

    void MainWindow::SetupLocalization() {
        NavHomeUid().Content(tbox(L"Nav.Home"));
        NavTaskUid().Content(tbox(L"Nav.Task"));
        NavKernelModuleUid().Content(tbox(L"Nav.KernelModule"));
        NavFileUid().Content(tbox(L"Nav.File"));
        NavWindowUid().Content(tbox(L"Nav.Window"));
        NavUtilityUid().Content(tbox(L"Nav.Utility"));
        NavMonitorUid().Content(tbox(L"Nav.Monitor"));
        NavDisasmUid().Content(tbox(L"Nav.Disasm"));
        NavSettingsUid().Content(tbox(L"Nav.Settings"));
        NavHelpUid().Content(tbox(L"Nav.Help"));
    }
}
