#include "pch.h"
#include "WindowPage.xaml.h"
#if __has_include("WindowPage.g.cpp")
#include "WindowPage.g.cpp"
#endif

#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include <shellapi.h>
#include <array>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <unordered_set>
#include <InfoWindow.xaml.h>
#include <MainWindow.xaml.h>
#include <psapi.h>
#include <dwmapi.h>

using namespace winrt;
using namespace WinUI3Package;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Imaging;
using namespace Windows::System;

typedef BOOL(*WTMInit_t)(void);
typedef BOOL(*WTMUninit_t)(void);
typedef BOOL(*WTMSetWindowBand_t)(HWND hWnd, HWND hWndInsertAfter, DWORD dwBand);
typedef BOOL(*WTMGetWindowBand_t)(HWND hWnd, PDWORD pdwBand);

namespace winrt::StarlightGUI::implementation
{
    static std::unordered_map<hstring, std::optional<winrt::Microsoft::UI::Xaml::Media::ImageSource>> iconCache;
    static std::chrono::steady_clock::time_point lastRefresh;
    static int safeAcceptedImage = -1;
    static WTMInit_t WTMInit = nullptr;
    static WTMUninit_t WTMUninit = nullptr;
    static WTMSetWindowBand_t WTMSetWindowBand = nullptr;
    static WTMGetWindowBand_t WTMGetWindowBand = nullptr;

    WindowPage::WindowPage() {
        InitializeComponent();

        WindowListView().ItemsSource(m_windowList);
        WindowListView().ItemContainerTransitions().Clear();
        WindowListView().ItemContainerTransitions().Append(EntranceThemeTransition());
        WindowListView().SizeChanged([weak = get_weak()](auto&&, auto&&) {
            if (auto self = weak.get()) {
                slg::UpdateVisibleListViewMarqueeByNames(
                    self->WindowListView(),
                    self->m_windowList.Size(),
                    L"WindowTextContainer",
                    L"WindowDescriptionTextBlock",
                    L"WindowDescriptionMarquee");
                slg::UpdateVisibleListViewMarqueeByNames(
                    self->WindowListView(),
                    self->m_windowList.Size(),
                    L"WindowTextContainer",
                    L"WindowNameTextBlock",
                    L"WindowNameMarquee");
            }
            });

        this->Loaded([this](auto&&, auto&&) -> IAsyncAction {
            // 初始化一次，后面不释放，程序退出时自动释放
            HMODULE hModule = LoadLibraryW(wtmPath.c_str());

            if (hModule) {
                WTMInit = (WTMInit_t)GetProcAddress(hModule, "WTMInit");
                WTMUninit = (WTMUninit_t)GetProcAddress(hModule, "WTMUninit");
                WTMSetWindowBand = (WTMSetWindowBand_t)GetProcAddress(hModule, "WTMSetWindowBand");
                WTMGetWindowBand = (WTMGetWindowBand_t)GetProcAddress(hModule, "WTMGetWindowBand");
            }
            LoadWindowList();
            co_return;
            });

        this->Unloaded([this](auto&&, auto&&) {
            ++m_reloadRequestVersion;
            });

        LOG_INFO(L"WindowPage", L"WindowPage initialized.");
    }

    void WindowPage::WindowListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
    {
        auto listView = WindowListView();

        slg::SelectItemOnRightTapped(listView, e);

        if (!listView.SelectedItem()) return;

        auto item = listView.SelectedItem().as<winrt::StarlightGUI::WindowInfo>();
        WINDOWINFO idk{};

        auto flyoutStyles = slg::GetStyles();

        if (item.Description() == L"StarlightGUI.exe / WinUIDesktopWin32WindowClass" || item.Description() == L"StarlightGUI.exe / ConsoleWindowClass") {
            slg::CreateInfoBarAndDisplay(L"警告", L"你要干什么？", InfoBarSeverity::Warning, g_mainWindowInstance);
            return;
        }

        MenuFlyout menuFlyout;

        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", L"关闭", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (PostMessageW((HWND)item.Hwnd(), WM_CLOSE, 0, 0)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功关闭窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法关闭窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        auto item1_2 = slg::CreateMenuItem(flyoutStyles, L"\ue8f0", L"关闭 (结束任务)", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::EndTaskByWindow((HWND)item.Hwnd())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功关闭窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法关闭窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        auto item1_3 = slg::CreateMenuItem(flyoutStyles, L"\ue945", L"关闭 (内核)", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            DWORD pid;
			GetWindowThreadProcessId((HWND)item.Hwnd(), &pid);
            if (KernelInstance::_ZwTerminateProcess(pid)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功关闭窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法关闭窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 分割线1
        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue912", L"设置状态");
        auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", L"显示", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (ShowWindow((HWND)item.Hwnd(), SW_SHOW) || GetLastError() == 0) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功显示窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法显示窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub1);
        auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", L"隐藏", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (ShowWindow((HWND)item.Hwnd(), SW_HIDE) || GetLastError() == 0) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功隐藏窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法隐藏窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub2);
        auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ue740", L"最大化", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (PostMessageW((HWND)item.Hwnd(), WM_SYSCOMMAND, SC_MAXIMIZE, 0) == ERROR_SUCCESS) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功最大化窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法最大化窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub3);
        auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ue73f", L"最小化", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (PostMessageW((HWND)item.Hwnd(), WM_SYSCOMMAND, SC_MINIMIZE, 0) == ERROR_SUCCESS) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功最小化窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法最小化窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub4);

        // 选项2.2
        auto item2_2 = slg::CreateMenuSubItem(flyoutStyles, L"\uf7ed", L"设置 ZBID");
        auto item2_2_sub1 = slg::CreateMenuItem(flyoutStyles, L"Desktop", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_DESKTOP)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Desktop", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Desktop, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub1);
        auto item2_2_sub2 = slg::CreateMenuItem(flyoutStyles, L"UIAccess", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_UIACCESS)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 UIAccess", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 UIAccess, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub2);
        auto item2_2_sub3 = slg::CreateMenuItem(flyoutStyles, L"Immersive-IHM", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_IHM)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-IHM", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-IHM, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub3);
        auto item2_2_sub4 = slg::CreateMenuItem(flyoutStyles, L"Immersive-Notification", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_NOTIFICATION)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-Notification", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-Notification, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub4);
        auto item2_2_sub5 = slg::CreateMenuItem(flyoutStyles, L"Immersive-AppChrome", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_APPCHROME)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-AppChrome", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-AppChrome, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub5);
        auto item2_2_sub6 = slg::CreateMenuItem(flyoutStyles, L"Immersive-MOGO", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_MOGO)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-MOGO", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-MOGO, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub6);
        auto item2_2_sub7 = slg::CreateMenuItem(flyoutStyles, L"Immersive-EDGY", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_EDGY)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-EDGY", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-EDGY, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub7);
        auto item2_2_sub8 = slg::CreateMenuItem(flyoutStyles, L"Immersive-InactiveMobody", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_INACTIVEMOBODY)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-InactiveMobody", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-InactiveMobody, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub8);
        auto item2_2_sub9 = slg::CreateMenuItem(flyoutStyles, L"Immersive-InactiveDock", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_INACTIVEDOCK)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-InactiveDock", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-InactiveDock, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub9);
        auto item2_2_sub10 = slg::CreateMenuItem(flyoutStyles, L"Immersive-ActiveMobody", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_ACTIVEMOBODY)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-ActiveMobody", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-ActiveMobody, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub10);
        auto item2_2_sub11 = slg::CreateMenuItem(flyoutStyles, L"Immersive-ActiveDock", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_ACTIVEDOCK)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-ActiveDock", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-ActiveDock, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub11);
        auto item2_2_sub12 = slg::CreateMenuItem(flyoutStyles, L"Immersive-Background", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_BACKGROUND)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-Background", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-Background, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub12);
        auto item2_2_sub13 = slg::CreateMenuItem(flyoutStyles, L"Immersive-Search", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_SEARCH)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-Search", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-Search, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub13);
        auto item2_2_sub14 = slg::CreateMenuItem(flyoutStyles, L"Immersive-Restricted", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_IMMERSIVE_RESTRICTED)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Immersive-Restricted", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Immersive-Restricted, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub14);
        auto item2_2_sub15 = slg::CreateMenuItem(flyoutStyles, L"GenuineWindows", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_GENUINE_WINDOWS)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 GenuineWindows", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 GenuineWindows, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub15);
        auto item2_2_sub16 = slg::CreateMenuItem(flyoutStyles, L"SystemTools", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_SYSTEM_TOOLS)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 SystemTools", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 SystemTools, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub16);
        auto item2_2_sub17 = slg::CreateMenuItem(flyoutStyles, L"Lock", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_LOCK)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 Lock", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 Lock, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub17);
        auto item2_2_sub18 = slg::CreateMenuItem(flyoutStyles, L"AboveLockUX", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (SetWindowZBID((HWND)item.Hwnd(), ZBID_ABOVELOCK_UX)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口 ZBID 为 AboveLockUX", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口 ZBID 为 AboveLockUX, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_2.Items().Append(item2_2_sub18);

        // 选项2.3
        auto item2_3 = slg::CreateMenuItem(flyoutStyles, L"\ue754", L"在任务栏闪烁", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (FlashWindow((HWND)item.Hwnd(), FALSE) || GetLastError() == 0) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功在任务栏闪烁窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法在任务栏闪烁窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项2.4
        auto item2_4 = slg::CreateMenuItem(flyoutStyles, L"\ue75c", L"重绘", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (UpdateWindow((HWND)item.Hwnd()) || GetLastError() == 0) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功重绘窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法重绘窗口: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 分割线2
        MenuFlyoutSeparator separator2;

        // 选项3.1
        auto item3_1 = slg::CreateMenuSubItem(flyoutStyles, L"\uef1f", L"设置样式");
        auto item3_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"纯色", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMSBT_NONE;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为纯色: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为纯色: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub1);
        auto item3_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"Mica (Base)", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMSBT_MAINWINDOW;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为Mica (Base): " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为Mica (Base): " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub2);
        auto item3_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"Mica (BaseAlt)", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMSBT_TABBEDWINDOW;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为Mica (BaseAlt): " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为Mica (BaseAlt): " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub3);
        auto item3_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"亚克力", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMSBT_TRANSIENTWINDOW;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为亚克力: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为亚克力: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub4);
        auto item3_1_sub5 = slg::CreateMenuItem(flyoutStyles, L"自动", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMSBT_AUTO;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为自动样式: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为自动样式: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub5);

        // 选项3.2
        auto item3_2 = slg::CreateMenuSubItem(flyoutStyles, L"\ue781", L"设置主题");
        auto item3_2_sub1 = slg::CreateMenuItem(flyoutStyles, L"深色", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            BOOL val = TRUE;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_USE_IMMERSIVE_DARK_MODE, &val, sizeof(val)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为深色: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为深色: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub1);
        auto item3_2_sub2 = slg::CreateMenuItem(flyoutStyles, L"浅色", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            BOOL val = FALSE;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_USE_IMMERSIVE_DARK_MODE, &val, sizeof(val)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为浅色: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为浅色: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub2);

        // 选项3.3
        auto item3_3 = slg::CreateMenuSubItem(flyoutStyles, L"\ue746", L"设置圆角");
        auto item3_3_sub1 = slg::CreateMenuItem(flyoutStyles, L"无圆角", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMWCP_DONOTROUND;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_WINDOW_CORNER_PREFERENCE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为无圆角: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为无圆角: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_3.Items().Append(item3_3_sub1);
        auto item3_3_sub2 = slg::CreateMenuItem(flyoutStyles, L"圆角", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMWCP_ROUND;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_WINDOW_CORNER_PREFERENCE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为圆角: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为圆角: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_3.Items().Append(item3_3_sub2);
        auto item3_3_sub3 = slg::CreateMenuItem(flyoutStyles, L"圆角 (小)", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMWCP_ROUNDSMALL;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_WINDOW_CORNER_PREFERENCE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为圆角 (小): " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为圆角 (小): " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_3.Items().Append(item3_3_sub3);
        auto item3_3_sub4 = slg::CreateMenuItem(flyoutStyles, L"自动", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            auto type = DWMWCP_DEFAULT;
            if (SUCCEEDED(DwmSetWindowAttribute((HWND)item.Hwnd(), DWMWA_WINDOW_CORNER_PREFERENCE, &type, sizeof(type)))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置窗口为自动圆角: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置窗口为自动圆角: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_3.Items().Append(item3_3_sub4);

        // 选项3.4
        auto item3_4 = slg::CreateMenuItem(flyoutStyles, L"\ue740", L"拓展标题栏至窗体", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            MARGINS margins = { -1 };
            if (SUCCEEDED(DwmExtendFrameIntoClientArea((HWND)item.Hwnd(), &margins))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功拓展窗口标题栏至窗体: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法拓展窗口标题栏至窗体: " + item.Name() + L" (" + to_hstring(item.Hwnd()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

		menuFlyout.Items().Append(item1_1);
        menuFlyout.Items().Append(item1_2);
        menuFlyout.Items().Append(item1_3);
		menuFlyout.Items().Append(separator1);
        menuFlyout.Items().Append(item2_1);
        menuFlyout.Items().Append(item2_2);
        menuFlyout.Items().Append(item2_3);
        menuFlyout.Items().Append(item2_4);
        menuFlyout.Items().Append(separator2);
        menuFlyout.Items().Append(item3_1);
        menuFlyout.Items().Append(item3_2);
        menuFlyout.Items().Append(item3_3);
        menuFlyout.Items().Append(item3_4);

        slg::ShowAt(menuFlyout, listView, e);
    }

    void WindowPage::WindowListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {
        if (args.InRecycleQueue()) return;

        auto itemContainer = args.ItemContainer().try_as<winrt::Microsoft::UI::Xaml::Controls::ListViewItem>();
        if (!itemContainer) return;

        auto contentRoot = itemContainer.ContentTemplateRoot().try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
        if (!contentRoot) return;

        slg::UpdateTextMarqueeByNames(
            contentRoot,
            L"WindowTextContainer",
            L"WindowDescriptionTextBlock",
            L"WindowDescriptionMarquee");
        slg::UpdateTextMarqueeByNames(
            contentRoot,
            L"WindowTextContainer",
            L"WindowNameTextBlock",
            L"WindowNameMarquee");

        DispatcherQueue().TryEnqueue([weak = get_weak(), contentRoot]() {
            if (auto self = weak.get()) {
                slg::UpdateTextMarqueeByNames(
                    contentRoot,
                    L"WindowTextContainer",
                    L"WindowDescriptionTextBlock",
                    L"WindowDescriptionMarquee");
                slg::UpdateTextMarqueeByNames(
                    contentRoot,
                    L"WindowTextContainer",
                    L"WindowNameTextBlock",
                    L"WindowNameMarquee");
            }
            });
    }

    winrt::Windows::Foundation::IAsyncAction WindowPage::LoadWindowList()
    {
        if (m_isLoadingWindows) {
            co_return;
        }
        m_isLoadingWindows = true;

        LOG_INFO(__WFUNCTION__, L"Loading window list...");
        m_windowList.Clear();
        LoadingRing().IsActive(true);

        auto start = std::chrono::steady_clock::now();

        auto lifetime = get_strong();

        winrt::hstring query = SearchBox().Text();
        m_showVisibleOnly = ShowVisibleOnlyCheckBox().IsChecked().GetBoolean();
        m_showNoTitle = ShowNoTitleCheckBox().IsChecked().GetBoolean();

        co_await winrt::resume_background();

        std::vector<winrt::StarlightGUI::WindowInfo> windows;
        windows.reserve(500);

        co_await GetWindowInfoAsync(windows);
        LOG_INFO(__WFUNCTION__, L"Enumerated windows, %d entry(s).", windows.size());

        lastRefresh = std::chrono::steady_clock::now();

        co_await wil::resume_foreground(DispatcherQueue());

        for (const auto& window : windows) {
            bool shouldRemove = query.empty() ? false : ApplyFilter(window, query);
            if (shouldRemove) continue;

            if (!(HWND)window.Hwnd()) continue;

            co_await GetWindowIconAsync(window);

            if (window.Name().empty()) window.Name(L"(未知)");
            if (window.Process().empty()) window.Process(L"(未知)");
            if (window.ClassName().empty()) window.ClassName(L"(未知)");

            m_windowList.Append(window);
        }

        // 恢复排序
        ApplySort(currentSortingOption, currentSortingType);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // 更新窗口数量文本
        std::wstringstream countText;
        countText << L"共 " << m_windowList.Size() << L" 个窗口 (" << duration << " ms)";
        WindowCountText().Text(countText.str());

        LoadingRing().IsActive(false);
        slg::UpdateVisibleListViewMarqueeByNames(
            WindowListView(),
            m_windowList.Size(),
            L"WindowTextContainer",
            L"WindowDescriptionTextBlock",
            L"WindowDescriptionMarquee");
        slg::UpdateVisibleListViewMarqueeByNames(
            WindowListView(),
            m_windowList.Size(),
            L"WindowTextContainer",
            L"WindowNameTextBlock",
            L"WindowNameMarquee");

        LOG_INFO(__WFUNCTION__, L"Loaded window list, %d entry(s) in total.", m_windowList.Size());
        m_isLoadingWindows = false;
    }

    winrt::Windows::Foundation::IAsyncAction WindowPage::GetWindowInfoAsync(std::vector<winrt::StarlightGUI::WindowInfo>& windows)
    {
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            std::vector<winrt::StarlightGUI::WindowInfo>& windowsRef = *reinterpret_cast<std::vector<winrt::StarlightGUI::WindowInfo>*>(lParam);

            if (m_showVisibleOnly && !IsWindowVisible(hwnd)) return TRUE;

            std::wstring windowTitle = L"(未知)";
            int length = GetWindowTextLengthW(hwnd);
            if (length > 0) {
                windowTitle = std::wstring(length + 1, '\0');
                GetWindowTextW(hwnd, &windowTitle[0], length + 1);
            }
            else if (!m_showNoTitle) return TRUE;

            DWORD processId = 0;
            GetWindowThreadProcessId(hwnd, &processId);
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, processId);
            std::wstring processName;
            if (hProcess) {
                wchar_t processNameTemp[MAX_PATH];
                if (K32GetModuleFileNameExW(hProcess, nullptr, processNameTemp, MAX_PATH)) {
                    processName = processNameTemp;
                }
                CloseHandle(hProcess);
            }

            std::wstring className = L"(未知)";
            wchar_t classNameTmp[MAX_PATH];
            GetClassNameW(hwnd, &classNameTmp[0], MAX_PATH);
            className = classNameTmp;

            DWORD windowStyle = 0;
            DWORD windowStyleEx = 0;

            WINDOWINFO pwndInfo = { 0 };
            pwndInfo.cbSize = sizeof(WINDOWINFO);
            if (GetWindowInfo(hwnd, &pwndInfo)) {
                windowStyle = pwndInfo.dwStyle;
                windowStyleEx = pwndInfo.dwExStyle;
            }

            DWORD band = 0;
			WTMGetWindowBand(hwnd, &band);

            winrt::StarlightGUI::WindowInfo windowInfo = winrt::make<winrt::StarlightGUI::implementation::WindowInfo>();
            windowInfo.Name(windowTitle);
            windowInfo.Process(processName);
            windowInfo.ClassName(className);
            windowInfo.FromPID(processId);
            windowInfo.WindowStyle(windowStyle);
            windowInfo.WindowStyleEx(windowStyleEx);
            windowInfo.Band(band);
            windowInfo.Hwnd((uint64_t)hwnd);
            windowInfo.Description(ExtractFileName(processName) + L" / " + className);
            windowsRef.push_back(windowInfo);

            return TRUE;
            }, reinterpret_cast<LPARAM>(&windows));

        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction WindowPage::GetWindowIconAsync(const winrt::StarlightGUI::WindowInfo& window) {
        if (iconCache.find(window.Name()) == iconCache.end()) {
			// 获取窗口图标 ICON
            HICON hIcon = (HICON)GetClassLongPtrW((HWND)window.Hwnd(), GCLP_HICON);
            if (!hIcon)
                hIcon = (HICON)GetClassLongPtrW((HWND)window.Hwnd(), GCLP_HICONSM);
            if (!hIcon)
				hIcon = (HICON)LoadImageW(NULL, MAKEINTRESOURCEW(32512), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
            if (!hIcon) co_return;
            auto iconSource = slg::CreateImageSourceFromHIcon(hIcon, 16, false);
            if (!iconSource) co_return;

            iconCache[window.Name()] = iconSource;
            window.Icon(iconSource);
        }
        else {
            if (iconCache[window.Name()].has_value()) {
                window.Icon(iconCache[window.Name()].value());
            }
            else {
                LOG_WARNING(__WFUNCTION__, L"File icon path (%s) does not have a value!", window.Name().c_str());
            }
        }

        co_return;
    }

    bool WindowPage::SetWindowZBID(HWND hwnd, ZBID zbid) {
        static bool inited = false;
        inited = WTMInit();
        if (!inited)
        {
            LOG_ERROR(L"WindowPage", L"WindowTopMost failed to initialize.");
        }
        if (!WTMSetWindowBand)
        {
            LOG_ERROR(__WFUNCTION__, L"WindowTopMost failed to load! Is the module broken?");
            return false;
        }

        LOG_INFO(__WFUNCTION__, L"Setting window band to %d.", (DWORD)zbid);
		BOOL result = WTMSetWindowBand(hwnd, NULL, (DWORD)zbid);

        return result;
    }

    void WindowPage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();

        struct SortBinding {
            wchar_t const* tag;
            char const* column;
            bool* ascending;
        };

        static const std::array<SortBinding, 4> bindings{ {
            { L"Name", "Name", &WindowPage::m_isNameAscending },
            { L"Band", "Band", &WindowPage::m_isBandAscending },
            { L"WindowStyle", "WindowStyle", &WindowPage::m_isWindowStyleAscending },
            { L"Hwnd", "Hwnd", &WindowPage::m_isHwndAscending },
        } };

        for (auto const& binding : bindings) {
            if (columnName == binding.tag) {
                ApplySort(*binding.ascending, binding.column);
                break;
            }
        }
    }

    // 排序切换
    slg::coroutine WindowPage::ApplySort(bool& isAscending, const std::string& column)
    {
        SortWindowList(isAscending, column, true);

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;

        co_return;
    }

    void WindowPage::SortWindowList(bool isAscending, const std::string& column, bool updateHeader)
    {
        if (column.empty()) return;

        enum class SortColumn {
            Unknown,
            Name,
            Band,
            WindowStyle,
            Hwnd
        };

        auto resolveSortColumn = [&](const std::string& key) -> SortColumn {
            if (key == "Name") return SortColumn::Name;
            if (key == "Band") return SortColumn::Band;
            if (key == "WindowStyle") return SortColumn::WindowStyle;
            if (key == "Hwnd") return SortColumn::Hwnd;
            return SortColumn::Unknown;
            };

        auto activeColumn = resolveSortColumn(column);
        if (activeColumn == SortColumn::Unknown) return;

        if (updateHeader) {
            NameHeaderButton().Content(box_value(L"窗口"));
            BandHeaderButton().Content(box_value(L"ZBID"));
            WindowStyleHeaderButton().Content(box_value(L"样式"));
            HwndHeaderButton().Content(box_value(L"HWND"));

            if (activeColumn == SortColumn::Name) NameHeaderButton().Content(box_value(isAscending ? L"窗口 ↓" : L"窗口 ↑"));
            if (activeColumn == SortColumn::Band) BandHeaderButton().Content(box_value(isAscending ? L"ZBID ↓" : L"ZBID ↑"));
            if (activeColumn == SortColumn::WindowStyle) WindowStyleHeaderButton().Content(box_value(isAscending ? L"样式 ↓" : L"样式 ↑"));
            if (activeColumn == SortColumn::Hwnd) HwndHeaderButton().Content(box_value(isAscending ? L"HWND ↓" : L"HWND ↑"));
        }

        std::vector<winrt::StarlightGUI::WindowInfo> sortedWindows;
        sortedWindows.reserve(m_windowList.Size());
        for (auto const& window : m_windowList) {
            sortedWindows.push_back(window);
        }

        auto sortActiveColumn = [&](const winrt::StarlightGUI::WindowInfo& a, const winrt::StarlightGUI::WindowInfo& b) -> bool {
            switch (activeColumn) {
            case SortColumn::Name:
                return LessIgnoreCase(a.Name().c_str(), b.Name().c_str());
            case SortColumn::Band:
                return a.Band() < b.Band();
            case SortColumn::WindowStyle:
                return (uint32_t)a.WindowStyle() < (uint32_t)b.WindowStyle();
            case SortColumn::Hwnd:
                return a.Hwnd() < b.Hwnd();
            default:
                return false;
            }
            };

        if (isAscending) {
            std::sort(sortedWindows.begin(), sortedWindows.end(), sortActiveColumn);
        }
        else {
            std::sort(sortedWindows.begin(), sortedWindows.end(), [&](const auto& a, const auto& b) {
                return sortActiveColumn(b, a);
                });
        }

        m_windowList.Clear();
        for (auto& window : sortedWindows) {
            m_windowList.Append(window);
        }
    }

    void WindowPage::CheckBox_Checked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        LoadWindowList();
    }

    void WindowPage::SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;

        WaitAndReloadAsync(250);
    }

    bool WindowPage::ApplyFilter(const winrt::StarlightGUI::WindowInfo& window, hstring& query) {
        return !ContainsIgnoreCase(window.Name().c_str(), query.c_str());
    }


    slg::coroutine WindowPage::RefreshButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        RefreshButton().IsEnabled(false);
        co_await LoadWindowList();
        RefreshButton().IsEnabled(true);
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction WindowPage::WaitAndReloadAsync(int interval) {
        auto lifetime = get_strong();
        auto requestVersion = ++m_reloadRequestVersion;

        co_await winrt::resume_after(std::chrono::milliseconds(interval));
        co_await wil::resume_foreground(DispatcherQueue());

        if (!IsLoaded() || requestVersion != m_reloadRequestVersion) co_return;
        LoadWindowList();

        co_return;
    }
}


