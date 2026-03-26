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
#include <commctrl.h>
#include <MainWindow.xaml.h>
#include <Utils/ProcessInfo.h>

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
    static HWND globalHWND;
    InfoWindow* g_infoWindowInstance = nullptr;
    winrt::StarlightGUI::ProcessInfo processForInfoWindow = nullptr;

    InfoWindow::InfoWindow() {
        InitializeComponent();

        auto windowNative{ this->try_as<::IWindowNative>() };
        HWND hWnd{ 0 };
        windowNative->get_WindowHandle(&hWnd);
        globalHWND = hWnd;

        SetWindowPos(hWnd, g_mainWindowInstance->GetWindowHandle(), 0, 0, 1200, 800, SWP_NOMOVE);

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
        AppWindow().TitleBar().PreferredHeightOption(winrt::Microsoft::UI::Windowing::TitleBarHeightOption::Tall);
        AppWindow().SetIcon(GetInstalledLocationPath() + L"\\Assets\\Starlight.ico");
        SetWindowSubclass(hWnd, &InfoWindowProc, 1, reinterpret_cast<DWORD_PTR>(this));

        int32_t width = ReadConfig("window_width", 1200);
        int32_t height = ReadConfig("window_height", 800);
        AppWindow().Resize(SizeInt32{ width, height });

        // 外观
        LoadBackdrop();
        LoadBackground();
        LoadNavigation();

        g_infoWindowInstance = this;

        for (auto& window : g_mainWindowInstance->m_openWindows) {
            if (window) {
                window.Close();
            }
        }
        g_mainWindowInstance->m_openWindows.push_back(*this);

        MainFrame().Navigate(xaml_typename<StarlightGUI::Process_ThreadPage>());
        RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
        ProcessName().Text(processForInfoWindow.Name());
        Title(processForInfoWindow.Name());

        Closed([this](auto&& sender, const winrt::Microsoft::UI::Xaml::WindowEventArgs& args) {
            g_mainWindowInstance->m_openWindows.clear();
            g_infoWindowInstance = nullptr;
            });
    }

    void InfoWindow::RootNavigation_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView sender, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs args)
    {
        auto invokedItem = unbox_value<winrt::hstring>(args.InvokedItemContainer().Tag());

        if (invokedItem == L"Thread")
        {
            MainFrame().Navigate(xaml_typename<StarlightGUI::Process_ThreadPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(0));
        }
        else if (invokedItem == L"Handle")
        {
            MainFrame().Navigate(xaml_typename<StarlightGUI::Process_HandlePage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(1));
        }
        else if (invokedItem == L"Module")
        {
            MainFrame().Navigate(xaml_typename<StarlightGUI::Process_ModulePage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(2));
        }
        else if (invokedItem == L"KCT")
        {
            MainFrame().Navigate(xaml_typename<StarlightGUI::Process_KCTPage>());
            RootNavigation().SelectedItem(RootNavigation().MenuItems().GetAt(3));
        }
    }

    slg::coroutine InfoWindow::LoadBackdrop()
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

        LOG_INFO(L"InfoWindow", L"Loading backdrop async with options: [%d, %d]", background_type, option);
        co_return;
    }

    slg::coroutine InfoWindow::LoadBackground()
    {
        if (background_image.empty()) {
            SolidColorBrush brush;
            brush.Color(Colors::Transparent());

            InfoWindowGrid().Background(brush);
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

                    InfoWindowGrid().Background(brush);

                    LOG_INFO(L"InfoWindow", L"Loading background async with options: [%s, %d, %d]", to_hstring(background_image).c_str(), image_opacity, image_stretch);
                }
            }
            catch (hresult_error) {
                SolidColorBrush brush;
                brush.Color(Colors::Transparent());

                InfoWindowGrid().Background(brush);
                LOG_ERROR(L"InfoWindow", L"Unable to load window backgroud! Applying transparent brush instead.");
            }
        }
        else {
            SolidColorBrush brush;
            brush.Color(Colors::Transparent());

            InfoWindowGrid().Background(brush);
            LOG_ERROR(L"InfoWindow", L"Background file does not exist. Applying transparent brush instead.");
        }
        co_return;
    }

    slg::coroutine InfoWindow::LoadNavigation()
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

        LOG_INFO(L"InfoWindow", L"Loading navigation async with options: [%d]", navigation_style);
        co_return;
    }

    HWND InfoWindow::GetWindowHandle()
    {
        return globalHWND;
    }

    LRESULT CALLBACK InfoWindow::InfoWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
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
            RemoveWindowSubclass(hWnd, &InfoWindowProc, uIdSubclass);
            break;
        }
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}
