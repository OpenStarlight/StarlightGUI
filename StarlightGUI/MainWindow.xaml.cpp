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
#include "UpdateDialog.xaml.h"

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

        int32_t width = ReadConfig("window_width", 1200);
        int32_t height = ReadConfig("window_height", 800);
        AppWindow().Resize(SizeInt32{ width, height });

        // 外观
        LoadBackdrop();
        LoadBackground();
        LoadNavigation();

        g_mainWindowInstance = this;

        Activated([this](auto&&, auto&&) -> IAsyncAction {
            if (!loaded) {
                RootNavigation().IsEnabled(false);
                loaded = true;
                // 加载模块
                slg::CreateInfoBarAndDisplay(L"信息", L"正在加载模块，这可能需要一点时间...", InfoBarSeverity::Informational, g_mainWindowInstance);
                co_await LoadModules();

                // 进入主页
                LOG_INFO(L"MainWindow", L"Navigates to StarlightGUI::HomePage because we are initializing MainWindow for the first time.");
                MainFrame().Navigate(xaml_typename<StarlightGUI::HomePage>());
                RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));

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
            LOGGER_CLOSE();
            });

        LOG_INFO(L"MainWindow", L"MainWindow initialized.");
    }

    MainWindow::~MainWindow()
    {
        for (auto& window : m_openWindows) {
            if (window) {
                window.Close();
            }
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
        std::string option = "*";

        if (background_type == "Mica") {
            CustomMicaBackdrop micaBackdrop = CustomMicaBackdrop();

            this->SystemBackdrop(micaBackdrop);

            option = mica_type;
            if (option == "Base") {
                micaBackdrop.Kind(MicaKind::Base);
            }
            else {
                micaBackdrop.Kind(MicaKind::BaseAlt);
            }
        }
        else if (background_type == "Acrylic") {
            CustomAcrylicBackdrop acrylicBackdrop = CustomAcrylicBackdrop();

            this->SystemBackdrop(acrylicBackdrop);

            option = acrylic_type;
            if (option == "Base") {
                acrylicBackdrop.Kind(DesktopAcrylicKind::Base);
            }
            else if (option == "Thin") {
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

        LOG_INFO(L"MainWindow", L"Loaded backdrop async with options: [%s, %s]", std::wstring(background_type.begin(), background_type.end()).c_str(), std::wstring(option.begin(), option.end()).c_str());
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

                    brush.Stretch(image_stretch == "None" ? Stretch::None : image_stretch == "Uniform" ? Stretch::Uniform : image_stretch == "Fill" ? Stretch::Fill : Stretch::UniformToFill);
                    brush.Opacity(image_opacity / 100.0);

                    MainWindowGrid().Background(brush);

                    LOG_INFO(L"MainWindow", L"Loaded background async with options: [%s, %d, %s]", to_hstring(background_image).c_str(), image_opacity, to_hstring(image_stretch).c_str());
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
        if (navigation_style == "Left") {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::Left);
        }
        else if (navigation_style == "Top") {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::Top);
        }
        else
        {
            RootNavigation().PaneDisplayMode(NavigationViewPaneDisplayMode::LeftCompact);
        }

        LOG_INFO(L"MainWindow", L"Loaded navigation async with options: [%s]", to_hstring(navigation_style).c_str());
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
                    slg::CreateInfoBarAndDisplay(L"警告", L"检查更新失败！", InfoBarSeverity::Warning, g_mainWindowInstance);
                }
                else if (latestBuildNumber == currentBuildNumber) {
                    LOG_INFO(L"Updater", L"Latest = current, we are on the latest version.");
                    slg::CreateInfoBarAndDisplay(L"信息", L"你正在使用最新版本的 Starlight GUI！", InfoBarSeverity::Informational, g_mainWindowInstance);
                }
                else if (latestBuildNumber > currentBuildNumber) {
                    LOG_INFO(L"Updater", L"Latest > current, new version avaliable. Calling up update dialog.");
                    slg::CreateInfoBarAndDisplay(L"信息", L"检测到新版本的 Starlight GUI！", InfoBarSeverity::Informational, g_mainWindowInstance);
                    auto dialog = winrt::make<winrt::StarlightGUI::implementation::UpdateDialog>();
                    dialog.IsUpdate(true);
                    dialog.LatestVersion(json.GetNamedString(L"version"));
                    dialog.XamlRoot(MainWindowGrid().XamlRoot());

                    auto result = co_await dialog.ShowAsync();

                    if (result == ContentDialogResult::Primary) {
                        Uri target(json.GetNamedString(L"download_link"));
                        auto result = co_await Launcher::LaunchUriAsync(target);

                        if (result) {
                            slg::CreateInfoBarAndDisplay(L"成功", L"已在浏览器打开网页！", InfoBarSeverity::Success, g_mainWindowInstance);
                        }
                        else {
                            slg::CreateInfoBarAndDisplay(L"失败", L"无法打开网页！", InfoBarSeverity::Error, g_mainWindowInstance);
                        }
                    }
                }
                else if (latestBuildNumber < currentBuildNumber) {
                    LOG_INFO(L"Updater", L"Latest < current, maybe we are on a dev environment.", kernelPath.c_str());
                    slg::CreateInfoBarAndDisplay(L"信息", L"你正在使用 Starlight GUI 的开发版本！", InfoBarSeverity::Informational, g_mainWindowInstance);
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

                auto kernelFile = co_await StorageFile::GetFileFromPathAsync(GetInstalledLocationPath() + L"\\Assets\\kernel.sys");
                auto astralFile = co_await StorageFile::GetFileFromPathAsync(GetInstalledLocationPath() + L"\\Assets\\AstralX.sys");
                auto wtmFile = co_await StorageFile::GetFileFromPathAsync(GetInstalledLocationPath() + L"\\WindowTopMost.dll");
                auto iamKeyHackerFile = co_await StorageFile::GetFileFromPathAsync(GetInstalledLocationPath() + L"\\IAMKeyHacker.dll");

                if (kernelFile) {
                    kernelPath = kernelFile.Path();

                    LOG_INFO(L"Driver", L"kernel.sys path [%s], loading...", kernelPath.c_str());
                    DriverUtils::LoadKernelDriver(kernelPath.c_str(), unused);
                    LOG_INFO(L"Driver", L"kernel.sys initialization result: %s, GetLastError() = %d", unused.c_str(), GetLastError());
                }

                if (astralFile) {
                    astralPath = astralFile.Path();

                    LOG_INFO(L"Driver", L"AstralX.sys path [%s], loading...", astralPath.c_str());
                    DriverUtils::LoadDriver(astralPath.c_str(), L"AstralX", unused);
                    LOG_INFO(L"Driver", L"AstralX.sys initialization result: %s, GetLastError() = %d", unused.c_str(), GetLastError());
                }

                if (wtmFile) {
                    wtmPath = wtmFile.Path();

                    LOG_INFO(L"Driver", L"WindowTopMost.dll path [%s].", wtmPath.c_str());
                }

                if (iamKeyHackerFile) {
                    iamKeyHackerPath = iamKeyHackerFile.Path();

                    LOG_INFO(L"Driver", L"IAMKeyHacker.dll path [%s].", iamKeyHackerPath.c_str());
                }

                LOG_INFO(L"Driver", L"Loaded successfully.", kernelPath.c_str());

                co_await wil::resume_foreground(DispatcherQueue());
                slg::CreateInfoBarAndDisplay(L"成功", L"模块加载成功！", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            catch (const hresult_error& e) {
                LOG_ERROR(L"Driver", L"Failed to load modules! winrt::hresult_error: %s (%d)", e.message().c_str(), e.code().value);
                slg::CreateInfoBarAndDisplay(L"警告", L"一个或多个模块文件未找到或无法加载，部分功能可能不可用！", InfoBarSeverity::Warning, g_mainWindowInstance);
            }
        }
    }

    HWND MainWindow::GetWindowHandle()
    {
        return globalHWND;
    }

    LRESULT CALLBACK MainWindow::MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {

        switch (uMsg)
        {
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            pMinMaxInfo->ptMinTrackSize.x = 800;
            pMinMaxInfo->ptMinTrackSize.y = 600;
            return 0;
        }

        case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hWnd, &MainWindowProc, uIdSubclass);
            break;
        }
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}