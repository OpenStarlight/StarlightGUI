#pragma once

#include "MainWindow.g.h"
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <shellapi.h>

namespace slg { struct coroutine; }

namespace winrt::StarlightGUI::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        ~MainWindow();
        void SetupLocalization();

        void RootNavigation_ItemInvoked(Microsoft::UI::Xaml::Controls::NavigationView sender, Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs args);
        void AppTitleBar_PaneToggleRequested(Microsoft::UI::Xaml::Controls::TitleBar sender, winrt::Windows::Foundation::IInspectable const& args);

        HWND GetWindowHandle();

        // 外观
        slg::coroutine LoadBackdrop();
        slg::coroutine LoadBackground();
        slg::coroutine LoadNavigation();

        // 驱动和模块
        winrt::Windows::Foundation::IAsyncAction LoadModules();
        winrt::Windows::Foundation::IAsyncAction CheckUpdate();
        void SetTrayBackgroundRun(bool enabled);
        void NavigateToTaskPage();

        // 窗口
        static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        std::vector<winrt::StarlightGUI::InfoWindow> m_openWindows;
        NOTIFYICONDATAW m_notifyIconData{};

        inline static bool loaded = false;
        inline static HWND globalHWND;
        inline static constexpr UINT WM_TRAYICON = WM_APP + 100;
        inline static constexpr ULONG_PTR COPYDATA_NAVIGATE_TASK = 0x534C4754;
        inline static constexpr UINT TRAY_ID = 1;
        inline static constexpr UINT TRAY_CMD_RESTORE = 1001;
        inline static constexpr UINT TRAY_CMD_EXIT = 1002;
        bool m_trayIconAdded = false;
        bool m_allowClose = false;

        void InitializeTrayIcon();
        void RemoveTrayIcon();
        void HideWindowToTray();
        void RestoreWindowFromTray();
        void ShowTrayMenu();
    };

    extern MainWindow* g_mainWindowInstance;
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
