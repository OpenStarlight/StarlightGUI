#pragma once

#include "InfoWindow.g.h"
#include "Utils/Coroutine.h"
#include <Utils/ProcessInfo.h>

namespace slg { struct coroutine; }

namespace winrt::StarlightGUI::implementation
{
    struct InfoWindow : InfoWindowT<InfoWindow>
    {
        InfoWindow();
        InfoWindow(winrt::StarlightGUI::ProcessInfo const& process);
        void SetupLocalization();

        HWND GetWindowHandle();

        void RootNavigation_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView sender, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs args);
        void AppTitleBar_PaneToggleRequested(Microsoft::UI::Xaml::Controls::TitleBar sender, winrt::Windows::Foundation::IInspectable const& args);

        // 外观
        slg::coroutine LoadBackdrop();
        slg::coroutine LoadBackground();
        slg::coroutine LoadNavigation();

        // 窗口
        static LRESULT CALLBACK InfoWindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR referenceData);

        winrt::StarlightGUI::ProcessInfo m_process{ nullptr };
        HWND m_windowHandle{ nullptr };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct InfoWindow : InfoWindowT<InfoWindow, implementation::InfoWindow>
    {
    };
}
