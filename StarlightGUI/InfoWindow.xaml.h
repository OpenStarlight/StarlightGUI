#pragma once

#include "InfoWindow.g.h"
#include <Utils/ProcessInfo.h>

namespace slg { struct coroutine; }

namespace winrt::StarlightGUI::implementation
{
    struct InfoWindow : InfoWindowT<InfoWindow>
    {
        InfoWindow();
        void SetupLocalization();

        HWND GetWindowHandle();

        void RootNavigation_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView sender, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs args);

        // 外观
        slg::coroutine LoadBackdrop();
        slg::coroutine LoadBackground();
        slg::coroutine LoadNavigation();

        // 窗口
        static LRESULT CALLBACK InfoWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };

    extern winrt::StarlightGUI::ProcessInfo processForInfoWindow;
    extern InfoWindow* g_infoWindowInstance;
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct InfoWindow : InfoWindowT<InfoWindow, implementation::InfoWindow>
    {
    };
}
