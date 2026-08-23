#include "pch.h"
#include "InfoWindow.xaml.h"
#if __has_include("InfoWindow.g.cpp")
#include "InfoWindow.g.cpp"
#endif

#include <winrt/Microsoft.UI.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Navigation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/WinUI3Package.h>
#include <microsoft.ui.xaml.window.h>
#include <commctrl.h>
#include <algorithm>
#include <utility>
#include <MainWindow.xaml.h>
#include <Utils/ProcessInfo.h>
#include "Utils/CppUtils.h"
#include "Utils/Config.h"
#include "Utils/Utils.h"

using namespace winrt;
using namespace WinUI3Package;
using namespace Windows::UI;
using namespace Windows::Storage;
using namespace Windows::Graphics;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Composition::SystemBackdrops;

namespace winrt::StarlightGUI::implementation
{
    InfoWindow::InfoWindow() : InfoWindow(nullptr) {}

    InfoWindow::InfoWindow(winrt::StarlightGUI::ProcessInfo const& process) : m_process(process) {
        InitializeComponent();
        SetupLocalization();

        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND windowHandle{ 0 };
        windowNative->get_WindowHandle(&windowHandle);
        m_windowHandle = windowHandle;

        SetWindowPos(windowHandle, g_mainWindowInstance->GetWindowHandle(), 0, 0, 1200, 800, SWP_NOMOVE);

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
        AppWindow().TitleBar().PreferredHeightOption(winrt::Microsoft::UI::Windowing::TitleBarHeightOption::Tall);
        AppWindow().SetIcon(GetInstalledLocationPath() + L"\\Assets\\Starlight.ico");
        SetWindowSubclass(windowHandle, &InfoWindowProc, 1, (DWORD_PTR)this);

        int32_t width = ReadConfig("window_width", 1200);
        int32_t height = ReadConfig("window_height", 800);
        AppWindow().Resize(SizeInt32{ width, height });

        g_mainWindowInstance->m_openWindows.push_back(*this);

        // 外观
        slg::ApplyConfiguredTheme();
        LoadBackground();
        LoadNavigation();

        MainFrame().Navigate(xaml_typename<StarlightGUI::Process_ThreadPage>(), m_process);
        RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
        AppTitleBar().Title(m_process ? m_process.Name() : L"");
        AppTitleBar().Subtitle(m_process ? L"(" + to_hstring(m_process.Id()) + L")" : L"");
        Title(m_process ? m_process.Name() : L"");

        auto iconSource = Microsoft::UI::Xaml::Controls::ImageIconSource();
        if (m_process) iconSource.ImageSource(m_process.Icon());
        AppTitleBar().IconSource(iconSource);

        Closed([this](auto&& sender, const winrt::Microsoft::UI::Xaml::WindowEventArgs& args) {
            auto& windows = g_mainWindowInstance->m_openWindows;
            std::erase_if(windows, [this](auto const& window) {
                return window && winrt::get_self<InfoWindow>(window) == this;
                });
            });
    }

    void InfoWindow::RootNavigation_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView sender, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs args)
    {
        Navigation::FrameNavigationOptions options{};
        options.TransitionInfoOverride(args.RecommendedNavigationTransitionInfo());

        auto invokedItem = unbox_value<winrt::hstring>(args.InvokedItemContainer().Tag());

        if (invokedItem == L"Thread")
        {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::Process_ThreadPage>(), m_process, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
        }
        else if (invokedItem == L"Handle")
        {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::Process_HandlePage>(), m_process, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(1));
        }
        else if (invokedItem == L"Module")
        {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::Process_ModulePage>(), m_process, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(2));
        }
        else if (invokedItem == L"KCT")
        {
            MainFrame().NavigateToType(xaml_typename<StarlightGUI::Process_KCTPage>(), m_process, options);
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(3));
        }
    }

    void InfoWindow::AppTitleBar_PaneToggleRequested(Microsoft::UI::Xaml::Controls::TitleBar, winrt::Windows::Foundation::IInspectable const&)
    {
        if (RootNavigation().PaneDisplayMode() == NavigationViewPaneDisplayMode::Top) {
            RootNavigation().IsPaneOpen(false);
            return;
        }

        RootNavigation().IsPaneOpen(!RootNavigation().IsPaneOpen());
    }

    slg::coroutine InfoWindow::LoadBackdrop()
    {
        int option = -1;

        if (backgroundType == 1) {
            MicaBackdrop micaBackdrop = MicaBackdrop();

            this->SystemBackdrop(micaBackdrop);

            option = micaType;
            if (option == 0) {
                micaBackdrop.Kind(MicaKind::Base);
            }
            else {
                micaBackdrop.Kind(MicaKind::BaseAlt);
            }

            if (backgroundImage.empty()) InfoWindowGrid().Background(nullptr);
        }
        else if (backgroundType == 2) {
            CustomAcrylicBackdrop acrylicBackdrop = CustomAcrylicBackdrop();

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

            if (backgroundImage.empty()) InfoWindowGrid().Background(nullptr);
        }
        else
        {
            this->SystemBackdrop(nullptr);
            if (backgroundImage.empty()) {
                InfoWindowGrid().Background(SolidColorBrush(slg::GetConfiguredElementTheme() == ElementTheme::Dark
                    ? Color{ 255,32,32,32 }
                    : Color{ 255,243,243,243 }));
            }
        }

        LOG_INFO(L"InfoWindow", L"Loading backdrop async with options: [%d, %d]", backgroundType, option);
        co_return;
    }

    slg::coroutine InfoWindow::LoadBackground()
    {
        if (backgroundImage.empty()) {
            InfoWindowGrid().Background(nullptr);
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

                    InfoWindowGrid().Background(brush);

                    LOG_INFO(L"InfoWindow", L"Loading background async with options: [%s, %d, %d]", to_hstring(backgroundImage).c_str(), imageOpacity, imageStretch);
                }
            }
            catch (hresult_error) {
                InfoWindowGrid().Background(nullptr);
                LoadBackdrop();
                LOG_ERROR(L"InfoWindow", L"Unable to load window backgroud! Applying configured backdrop instead.");
            }
        }
        else {
            InfoWindowGrid().Background(nullptr);
            LoadBackdrop();
            LOG_ERROR(L"InfoWindow", L"Background file does not exist. Applying configured backdrop instead.");
        }
        co_return;
    }

    slg::coroutine InfoWindow::LoadNavigation()
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

        LOG_INFO(L"InfoWindow", L"Loading navigation async with options: [%d]", navigationStyle);
        co_return;
    }

    HWND InfoWindow::GetWindowHandle()
    {
        return m_windowHandle;
    }

    LRESULT CALLBACK InfoWindow::InfoWindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR referenceData)
    {

        switch (message)
        {
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;
            UINT dpi = GetDpiForWindow(windowHandle);
            if (dpi == 0) dpi = USER_DEFAULT_SCREEN_DPI;
            minMaxInfo->ptMinTrackSize.x = MulDiv(800, dpi, USER_DEFAULT_SCREEN_DPI);
            minMaxInfo->ptMinTrackSize.y = MulDiv(600, dpi, USER_DEFAULT_SCREEN_DPI);
            return 0;
        }

        case WM_NCDESTROY:
        {
            RemoveWindowSubclass(windowHandle, &InfoWindowProc, subclassId);
            break;
        }
        }
        return DefSubclassProc(windowHandle, message, wParam, lParam);
    }

    void InfoWindow::SetupLocalization()
    {
        NavHandleUid().Content(tbox(L"Nav.Handle"));
		NavKCTUid().Content(tbox(L"Nav.KCT"));
		NavModuleUid().Content(tbox(L"Nav.Module"));
		NavThreadUid().Content(tbox(L"Nav.Thread"));
	}
}
