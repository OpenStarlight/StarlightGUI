#include "pch.h"
#include "TaskPage.xaml.h"
#if __has_include("TaskPage.g.cpp")
#include "TaskPage.g.cpp"
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
#include <Psapi.h>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <shellapi.h>
#include <unordered_set>
#include <InfoWindow.xaml.h>
#include <MainWindow.xaml.h>
#include <RunProcessDialog.xaml.h>
#include <InjectDLLDialog.xaml.h>
#include <ModifyTokenDialog.xaml.h>
#undef EnumProcesses

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

namespace winrt::StarlightGUI::implementation
{
    static std::unordered_map<hstring, std::optional<winrt::Microsoft::UI::Xaml::Media::ImageSource>> iconCache;
    static std::unordered_map<hstring, hstring> descriptionCache;
    static std::unordered_map<DWORD, int> processIndexMap;
    static HDC hdc{ nullptr };
    static std::unordered_set<int> filteredPids;
    static std::vector<winrt::StarlightGUI::ProcessInfo> fullRecordedProcesses;
    static std::mutex safelock;
    static int safeAcceptedPID = -1;
    static bool infoWindowOpen = false;

    TaskPage::TaskPage() {
        InitializeComponent();

        ProcessListView().ItemsSource(m_processList);
        if (!list_animation) ProcessListView().ItemContainerTransitions().Clear();

        TaskUtils::EnsurePrivileges();

        this->Loaded([this](auto&&, auto&&) {
            hdc = GetDC(NULL);
            LoadProcessList();
			});

        this->Unloaded([this](auto&&, auto&&) {
            reloadTimer.Stop();
            ReleaseDC(NULL, hdc);
            });

        LOG_INFO(L"TaskPage", L"TaskPage initialized.");
    }

    void TaskPage::ProcessListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
    {
        auto listView = ProcessListView();

        slg::SelectItemOnRightTapped(listView, e);

        if (!listView.SelectedItem()) return;

        auto item = listView.SelectedItem().as<winrt::StarlightGUI::ProcessInfo>();
        auto flyoutStyles = slg::GetStyles();

        if (item.Name() == L"StarlightGUI.exe") {
            slg::CreateInfoBarAndDisplay(L"警告", L"你要干什么？", InfoBarSeverity::Warning, g_mainWindowInstance);
            return;
        }

        MenuFlyout menuFlyout;

        // 选项1.1
        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", L"终止", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::_TerminateProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功终止进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法终止进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项1.2
        auto item1_2 = slg::CreateMenuItem(flyoutStyles, L"\ue8f0", L"终止 (内核)", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_ZwTerminateProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功终止进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法终止束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项1.3
        auto item1_3 = slg::CreateMenuItem(flyoutStyles, L"\ue945", L"终止 (内存抹杀)", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (safeAcceptedPID == item.Id() || !dangerous_confirm) {
                if (KernelInstance::MurderProcess(item.Id())) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"成功终止进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(L"失败", L"无法终止进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            else {
                safeAcceptedPID = item.Id();
                slg::CreateInfoBarAndDisplay(L"警告", L"该操作可能导致系统崩溃或进程数据丢失，如果确认继续请再次点击！", InfoBarSeverity::Warning, g_mainWindowInstance);
            }
            co_return;
            });

        // 分割线1
        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue912", L"设置状态");
        auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue769", L"暂停", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_SuspendProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功暂停进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法暂停进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub1);
        auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\ue768", L"恢复", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_ResumeProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功恢复进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法恢复进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub2);

        // 选项2.2
        auto item2_2 = slg::CreateMenuItem(flyoutStyles, L"\ued1a", L"隐藏", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::HideProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功隐藏进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法隐藏进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项2.3
        auto item2_3 = slg::CreateMenuSubItem(flyoutStyles, L"\uea18", L"设置 PPL");
        
        // PPL等级
        auto item2_3_sub1 = slg::CreateMenuItem(flyoutStyles, L"None", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_None)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置进程 PPL 等级为 None (0x00).", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置进程 PPL 等级, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub1);
        auto item2_3_sub2 = slg::CreateMenuItem(flyoutStyles, L"Authenticode", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Authenticode)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置进程 PPL 等级为 Authenticode (0x11).", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置进程 PPL 等级, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub2);
        auto item2_3_sub3 = slg::CreateMenuItem(flyoutStyles, L"Codegen", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Codegen)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置进程 PPL 等级为 Codegen (0x21).", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置进程 PPL 等级, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub3);
        auto item2_3_sub4 = slg::CreateMenuItem(flyoutStyles, L"Antimalware", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Antimalware)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置进程 PPL 等级为 Antimalware (0x31).", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置进程 PPL 等级, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub4);
        auto item2_3_sub5 = slg::CreateMenuItem(flyoutStyles, L"Lsa", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Lsa)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置进程 PPL 等级为 Lsa (0x41).", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置进程 PPL 等级, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub5);
        auto item2_3_sub6 = slg::CreateMenuItem(flyoutStyles, L"Windows", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Windows)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置进程 PPL 等级为 Windows (0x51).", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置进程 PPL 等级, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub6);
        auto item2_3_sub7 = slg::CreateMenuItem(flyoutStyles, L"WinTcb", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_WinTcb)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置进程 PPL 等级为 WinTcb (0x61).", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置进程 PPL 等级, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub7);
        auto item2_3_sub8 = slg::CreateMenuItem(flyoutStyles, L"WinSystem", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_WinSystem)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功设置进程 PPL 等级为 WinSystem (0x71).", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置进程 PPL 等级, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub8);

        // 选项2.4
        auto item2_4 = slg::CreateMenuItem(flyoutStyles, L"\ue8c9", L"设置为关键进程", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (safeAcceptedPID == item.Id() || !dangerous_confirm) {
                if (KernelInstance::SetCriticalProcess(item.Id())) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"成功设置为关键进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(L"失败", L"无法设置为关键进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            else {
                safeAcceptedPID = item.Id();
                slg::CreateInfoBarAndDisplay(L"警告", L"设置为关键进程后，该进程退出会导致蓝屏，如果确认继续请再次点击！", InfoBarSeverity::Warning, g_mainWindowInstance);
            }
            co_return;
            });

        // 选项2.5
        auto item2_5 = slg::CreateMenuItem(flyoutStyles, L"\uebe8", L"注入DLL", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            InjectDLL(item.Id());
            co_return;
            });

        // 选项2.6
        auto item2_6 = slg::CreateMenuItem(flyoutStyles, L"\ue70f", L"修改令牌", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            ModifyToken(item.Id());
            co_return;
            });

        // 选项2.7
        auto item2_7 = slg::CreateMenuItem(flyoutStyles, L"\uf1e8", L"效率模式", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::EnableProcessPerformanceMode(item)) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功为进程开启效率模式: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法为进程开启效率模式: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 分割线2
        MenuFlyoutSeparator separator2;

        // 选项3.1
        auto item3_1 = slg::CreateMenuItem(flyoutStyles, L"\ue946", L"更多信息", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            processForInfoWindow = item;
            auto infoWindow = winrt::make<InfoWindow>();
            infoWindow.Activate();
            co_return;
            });

        // 选项3.2
        auto item3_2 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", L"复制信息");
        auto item3_2_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue8ac", L"名称", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Name().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub1);
        auto item3_2_sub2 = slg::CreateMenuItem(flyoutStyles, L"\ue943", L"PID", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(std::to_wstring(item.Id()))) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub2);
        auto item3_2_sub3 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", L"文件路径", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.ExecutablePath().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub3);
        auto item3_2_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", L"EPROCESS", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.EProcess().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub4);

        // 选项3.3
        auto item3_3 = slg::CreateMenuItem(flyoutStyles, L"\uec50", L"打开文件所在位置", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::OpenFolderAndSelectFile(item.ExecutablePath().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已打开文件夹", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法打开文件夹, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项3.4
        auto item3_4 = slg::CreateMenuItem(flyoutStyles, L"\ue8ec", L"属性", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::OpenFileProperties(item.ExecutablePath().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已打开文件属性", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法打开文件属性, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
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
        menuFlyout.Items().Append(item2_5);
        menuFlyout.Items().Append(item2_6);
        menuFlyout.Items().Append(item2_7);
        menuFlyout.Items().Append(separator2);
        menuFlyout.Items().Append(item3_1);
        menuFlyout.Items().Append(item3_2);
        menuFlyout.Items().Append(item3_3);
        menuFlyout.Items().Append(item3_4);

        slg::ShowAt(menuFlyout, listView, e);
    }

    void TaskPage::ProcessListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {
        if (args.InRecycleQueue())
            return;

        // 将 Tag 设到容器上，便于 ListViewItemPresenter 通过 TemplatedParent 绑定
        if (auto itemContainer = args.ItemContainer())
            itemContainer.Tag(sender.Tag());
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::LoadProcessList()
    {
        if (m_isLoadingProcesses) {
            co_return;
        }
        m_isLoadingProcesses = true;

        LOG_INFO(__WFUNCTION__, L"Loading process list...");
        m_processList.Clear();
        LoadingRing().IsActive(true);

        auto start = std::chrono::steady_clock::now();

        auto lifetime = get_strong();
        int selectedItemId = -1;
        if (ProcessListView().SelectedItem()) selectedItemId = ProcessListView().SelectedItem().as<winrt::StarlightGUI::ProcessInfo>().Id();

        winrt::hstring query = ProcessSearchBox().Text();

        co_await winrt::resume_background();

        std::vector<winrt::StarlightGUI::ProcessInfo> processes;
        std::map<DWORD, hstring> processCpuTable;

        processes.reserve(200);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            co_await wil::resume_foreground(DispatcherQueue());
            slg::CreateInfoBarAndDisplay(L"错误", L"无法获取进程快照", InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
        }

        PROCESSENTRY32W pe32{};
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                co_await GetProcessInfoAsync(pe32, processes);
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);

        LOG_INFO(__WFUNCTION__, L"Enumerated processes (user mode), %d entry(s).", processes.size());

        processIndexMap.clear();
        for (int i = 0; i < processes.size(); i++) {
            auto& process = processes.at(i);
            processIndexMap[process.Id()] = i;
        }

        // 对于 Windows 11，我们使用 AstralX 进行枚举
        // 对于 Windows 10 及以下版本，我们使用 SKT64 进行枚举
        if (TaskUtils::GetWindowsBuildNumber() >= 22000 && !enum_strengthen) {
            KernelInstance::EnumProcesses(processIndexMap, processes);
        }
        else {
            KernelInstance::EnumProcesses2(processIndexMap, processes);
        }

        LOG_INFO(__WFUNCTION__, L"Enumerated processes (kernel mode), %d entry(s).", processes.size());

        fullRecordedProcesses = processes;

        // 异步加载CPU使用率
        co_await TaskUtils::FetchProcessCpuUsage(processCpuTable);

        co_await wil::resume_foreground(DispatcherQueue());

        for (const auto& process : processes) {
            bool shouldRemove = query.empty() ? false : ApplyFilter(process, query);
            if (shouldRemove) continue;

            // 从缓存加载图标，没有则获取
            co_await GetProcessIconAsync(process);

            // 加载CPU占用
            if (processCpuTable.find((DWORD)process.Id()) != processCpuTable.end()) process.CpuUsage(processCpuTable[(DWORD)process.Id()]);

            // 格式化内存占用
            if (process.MemoryUsageByte() != 0) process.MemoryUsage(FormatMemorySize(process.MemoryUsageByte()));

            if (process.CpuUsage().empty()) process.CpuUsage(L"-1 (未知)");
            if (process.MemoryUsage().empty()) process.MemoryUsage(L"-1 (未知)");
            if (process.Status().empty()) process.Status(L"运行中");
            if (process.EProcess().empty()) process.EProcess(L"(未知)");

            m_processList.Append(process);
        }

        // 恢复排序
        ApplySort(currentSortingOption, currentSortingType);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // 更新进程数量文本
        std::wstringstream countText;
        countText << L"共 " << m_processList.Size() << L" 个进程 (" << duration << " ms)";
        ProcessCountText().Text(countText.str());
        LoadingRing().IsActive(false);

        LOG_INFO(__WFUNCTION__, L"Loaded process list, %d entry(s) in total.", m_processList.Size());
        m_isLoadingProcesses = false;
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::GetProcessInfoAsync(const PROCESSENTRY32W& pe32, std::vector<winrt::StarlightGUI::ProcessInfo>& processes)
    {
		// 跳过筛选的PID，在搜索时性能更好
        std::lock_guard<std::mutex> lock(safelock);
        if (filteredPids.find(pe32.th32ProcessID) != filteredPids.end()) {
            co_return;
		}
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            WCHAR processName[MAX_PATH] = L"";
            DWORD size = MAX_PATH;

            if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
                uint64_t memoryUsage = TaskUtils::GetProcessWorkingSet(hProcess);
                if (memoryUsage == 0) {
                    PROCESS_MEMORY_COUNTERS pmc{};
                    if (K32GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        memoryUsage = pmc.WorkingSetSize;
                    }
                }

				// 缓存描述信息
                if (descriptionCache.find(processName) == descriptionCache.end()) {
                    DWORD dummy;
                    DWORD versionInfoSize = GetFileVersionInfoSizeW(processName, &dummy);
                    std::wstring description = L"";

                    if (versionInfoSize > 0) {
                        std::vector<BYTE> versionInfo(versionInfoSize);
                        if (GetFileVersionInfoW(processName, 0, versionInfoSize, versionInfo.data())) {
                            VS_FIXEDFILEINFO* fileInfo;
                            UINT fileInfoSize;

                            if (VerQueryValueW(versionInfo.data(), L"\\", (LPVOID*)&fileInfo, &fileInfoSize)) {
                                if (fileInfo->dwFileFlagsMask & VS_FF_INFOINFERRED) {
                                }
                            }

                            wchar_t* fileDescription = nullptr;
                            UINT descriptionSize;
                            if (VerQueryValueW(versionInfo.data(), L"\\StringFileInfo\\040904b0\\FileDescription", (LPVOID*)&fileDescription, &descriptionSize) && fileDescription) {
                                description = fileDescription;
                            }
                        }
                    }

                    if (description.empty()) {
                        description = L"应用程序";
                    }

					descriptionCache[processName] = description;
				}

                auto processInfo = winrt::make<winrt::StarlightGUI::implementation::ProcessInfo>();
                processInfo.Id((int32_t)pe32.th32ProcessID);
                processInfo.Name(pe32.szExeFile);
                processInfo.Description(descriptionCache[processName]);
                processInfo.MemoryUsageByte(memoryUsage);
                processInfo.ExecutablePath(processName);

                processes.push_back(processInfo);
            }

            CloseHandle(hProcess);
        }

        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::GetProcessIconAsync(const winrt::StarlightGUI::ProcessInfo& process) {
        if (iconCache.find(process.ExecutablePath()) == iconCache.end()) {
            SHFILEINFO shfi;
            HICON hIcon;
            if (SHGetFileInfoW(process.ExecutablePath().c_str(), 0, &shfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON)) {
				hIcon = shfi.hIcon;
            }
            else {
                hIcon = (HICON)LoadImageW(NULL, MAKEINTRESOURCEW(32512), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
            }
            ICONINFO iconInfo;
            if (GetIconInfo(hIcon, &iconInfo)) {
                BITMAP bmp;
                GetObject(iconInfo.hbmColor, sizeof(bmp), &bmp);
                BITMAPINFOHEADER bmiHeader = { 0 };
                bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmiHeader.biWidth = bmp.bmWidth;
                bmiHeader.biHeight = bmp.bmHeight;
                bmiHeader.biPlanes = 1;
                bmiHeader.biBitCount = 32;
                bmiHeader.biCompression = BI_RGB;

                int dataSize = bmp.bmWidthBytes * bmp.bmHeight;
                std::vector<BYTE> buffer(dataSize);

                GetDIBits(hdc, iconInfo.hbmColor, 0, bmp.bmHeight, buffer.data(), reinterpret_cast<BITMAPINFO*>(&bmiHeader), DIB_RGB_COLORS);

                winrt::Microsoft::UI::Xaml::Media::Imaging::WriteableBitmap writeableBitmap(bmp.bmWidth, bmp.bmHeight);

                // 将数据写入 WriteableBitmap
                uint8_t* data = writeableBitmap.PixelBuffer().data();
                int rowSize = bmp.bmWidth * 4;
                for (int i = 0; i < bmp.bmHeight; ++i) {
                    int srcOffset = i * rowSize;
                    int dstOffset = (bmp.bmHeight - 1 - i) * rowSize;
                    std::memcpy(data + dstOffset, buffer.data() + srcOffset, rowSize);
                }

                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                DestroyIcon(hIcon);

                // 将图标缓存到 map 中
                iconCache[process.ExecutablePath()] = writeableBitmap.as<winrt::Microsoft::UI::Xaml::Media::ImageSource>();
                process.Icon(writeableBitmap);
            }
        }
        else {
            if (iconCache[process.ExecutablePath()].has_value()) {
                process.Icon(iconCache[process.ExecutablePath()].value());
            }
            else {
                LOG_WARNING(__WFUNCTION__, L"File icon path (%s) does not have a value!", process.ExecutablePath().c_str());
            }
        }

        co_return;
    }

    void TaskPage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();

        if (columnName == L"Name")
        {
            ApplySort(m_isNameAscending, "Name");
        }
        else if (columnName == L"CpuUsage")
        {
            ApplySort(m_isCpuAscending, "CpuUsage");
        }
        else if (columnName == L"MemoryUsage")
        {
            ApplySort(m_isMemoryAscending, "MemoryUsage");
        }
        else if (columnName == L"Id")
        {
            ApplySort(m_isIdAscending, "Id");
        }
    }

    // 排序切换
    slg::coroutine TaskPage::ApplySort(bool& isAscending, const std::string& column)
    {
        NameHeaderButton().Content(box_value(L"进程"));
        CpuHeaderButton().Content(box_value(L"CPU"));
        MemoryHeaderButton().Content(box_value(L"内存"));
        IdHeaderButton().Content(box_value(L"PID"));

        auto parseCpu = [](winrt::hstring const& value) mutable -> double {
            if (value.empty()) return 0.0;
            try {
                size_t idx = 0;
                double result = std::stod(std::wstring(value.c_str()), &idx);
                return result;
            }
            catch (...) {
                return 0.0;
            }
        };

        std::vector<winrt::StarlightGUI::ProcessInfo> processes;
        processes.reserve(m_processList.Size());
        for (auto const& process : m_processList) {
            processes.push_back(process);
        }

        if (column == "Name") {
            if (isAscending) {
                NameHeaderButton().Content(box_value(L"进程 ↓"));
                std::sort(processes.begin(), processes.end(), [](auto a, auto b) {
                    std::wstring aName = a.Name().c_str();
                    std::wstring bName = b.Name().c_str();
                    std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

                    return aName < bName;
                    });
                
            }
            else {
                NameHeaderButton().Content(box_value(L"进程 ↑"));
                std::sort(processes.begin(), processes.end(), [](auto a, auto b) {
                    std::wstring aName = a.Name().c_str();
                    std::wstring bName = b.Name().c_str();
                    std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

                    return aName > bName;
                    });
            }
        }
        else if (column == "CpuUsage") {
            if (isAscending) {
                CpuHeaderButton().Content(box_value(L"CPU ↓"));
                std::sort(processes.begin(), processes.end(), [&](auto a, auto b) {
                    return parseCpu(a.CpuUsage()) < parseCpu(b.CpuUsage());
                    });
            }
            else {
                CpuHeaderButton().Content(box_value(L"CPU ↑"));
                std::sort(processes.begin(), processes.end(), [&](auto a, auto b) {
                    return parseCpu(a.CpuUsage()) > parseCpu(b.CpuUsage());
                    });
            }
        }
        else if (column == "MemoryUsage") {
            if (isAscending) {
                MemoryHeaderButton().Content(box_value(L"内存 ↓"));
                std::sort(processes.begin(), processes.end(), [](auto a, auto b) {
                    return a.MemoryUsageByte() < b.MemoryUsageByte();
                    });
            }
            else {
                MemoryHeaderButton().Content(box_value(L"内存 ↑"));
                std::sort(processes.begin(), processes.end(), [](auto a, auto b) {
                    return a.MemoryUsageByte() > b.MemoryUsageByte();
                    });
            }
        }
        else if (column == "Id") {
            if (isAscending) {
                IdHeaderButton().Content(box_value(L"PID ↓"));
                std::sort(processes.begin(), processes.end(), [](auto a, auto b) {
                    return a.Id() < b.Id();
                    });
            }
            else {
                IdHeaderButton().Content(box_value(L"PID ↑"));
                std::sort(processes.begin(), processes.end(), [](auto a, auto b) {
                    return a.Id() > b.Id();
                    });
            }
        }

        m_processList.Clear();
        for (auto& process : processes) {
            m_processList.Append(process);
        }

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;

        co_return;
    }

    void TaskPage::ProcessSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
		// 每次搜索都清空之前缓存的过滤结果
        filteredPids.clear();

        winrt::hstring query = ProcessSearchBox().Text();

        /*
        * 我们的思路是这样的：
        *  - 在LoadProcessList()中，如果搜索框为空，则缓存一次完整的进程列表
        *  - 搜索时，先遍历这个完整的进程列表，记录需要过滤掉的进程PID
        *  - 由于在遍历进程时，我们会先检查一遍filteredPids，所以这会直接跳过进程的所有处理
        *  - 原先的逻辑是每次搜索都重新获取进程列表，然后再筛选一遍得到需要添加到ListView的进程，这意味着我们仍然会去处理进程，即使它最终会被过滤掉
        *  - 这样可以大幅提升搜索性能，并且我们会在添加进程时再进程一次过滤，确保新增的进程也会被正常过滤
        *  - 唯一的缺点是，如果正好原先有个进程退出，然后新启动了一个进程，这个新进程的PID正好是之前被过滤掉的进程的PID，那么这个新进程就会被错误地过滤掉
        *  - 但这种情况发生的概率极低，而且影响也不大，所以可以接受，我们还有个刷新按钮可以手动刷新进程列表
        */
        if (!query.empty()) {
            for (const auto& process : fullRecordedProcesses) {
                ApplyFilter(process, query);
            }
        }

        WaitAndReloadAsync(200);
    }

    bool TaskPage::ApplyFilter(const winrt::StarlightGUI::ProcessInfo& process, hstring& query) {
        std::wstring name = process.Name().c_str();
        std::wstring queryWStr = query.c_str();

        // 不比较大小写
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        std::transform(queryWStr.begin(), queryWStr.end(), queryWStr.begin(), ::towlower);

        uint32_t index;
        bool result = name.find(queryWStr) == std::wstring::npos;
        if (result) {
			// 缓存过滤的PID，这样下次可以直接跳过
			filteredPids.insert(process.Id());
        }

        return result;
    }


    slg::coroutine TaskPage::RefreshProcessListButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        RefreshProcessListButton().IsEnabled(false);

        filteredPids.clear();

        co_await LoadProcessList();

        RefreshProcessListButton().IsEnabled(true);
        co_return;
    }

    slg::coroutine TaskPage::CreateProcessButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        try {
            auto dialog = winrt::make<winrt::StarlightGUI::implementation::RunProcessDialog>();
            dialog.XamlRoot(this->XamlRoot());

            auto result = co_await dialog.ShowAsync();

            if (result == ContentDialogResult::Primary) {
                hstring processPath = dialog.ProcessPath();
                int permission = dialog.Permission();
                bool fullPrivileges = dialog.FullPrivileges();

                if (permission == 1) {
                    int status = CreateProcessElevated(processPath.c_str(), fullPrivileges, g_mainWindowInstance);
                    if (status != 1) {
                        std::wstring content = L"程序启动成功，PID: " + std::to_wstring(status);
                        slg::CreateInfoBarAndDisplay(L"成功", content.c_str(),
                            InfoBarSeverity::Success, g_mainWindowInstance);
                    }
                    else {
                        std::wstring content = L"程序启动失败，错误码: " + std::to_wstring(GetLastError());
                        slg::CreateInfoBarAndDisplay(L"失败", content.c_str(),
                            InfoBarSeverity::Error, g_mainWindowInstance);
                    }
                }
                else {
                    SHELLEXECUTEINFOW sei = { sizeof(sei) };
                    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
                    sei.lpFile = processPath.c_str();
                    sei.nShow = SW_SHOWNORMAL;
                    sei.lpVerb = L"runas";

                    BOOL stauts = ShellExecuteExW(&sei);

                    if (stauts && sei.hProcess != NULL) {
                        DWORD processId = GetProcessId(sei.hProcess);
                        CloseHandle(sei.hProcess);
                        CloseHandle(sei.hIcon);
                        std::wstring content = L"程序启动成功，PID: " + std::to_wstring(processId);
                        slg::CreateInfoBarAndDisplay(L"成功", content.c_str(),
                            InfoBarSeverity::Success, g_mainWindowInstance);
                    }
                    else {
                        std::wstring content = L"程序启动失败，错误码: " + std::to_wstring(GetLastError());
                        slg::CreateInfoBarAndDisplay(L"失败", content.c_str(),
                            InfoBarSeverity::Error, g_mainWindowInstance);
                    }
                }

                co_await WaitAndReloadAsync(1000);
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(L"错误", L"显示对话框失败: " + ex.message(),
                InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    slg::coroutine TaskPage::InjectDLL(ULONG pid) {
        try {
            auto dialog = winrt::make<winrt::StarlightGUI::implementation::InjectDLLDialog>();
            dialog.XamlRoot(this->XamlRoot());

            auto result = co_await dialog.ShowAsync();

            if (result == ContentDialogResult::Primary) {
                std::wstring path = std::wstring(dialog.DLLPath().c_str());

                size_t dotPos = path.rfind(L'.');
                if (dotPos == std::wstring::npos) {
                    slg::CreateInfoBarAndDisplay(L"错误", L"文件不是DLL类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                    co_return;
                }
                else {
                    std::wstring extension = path.substr(dotPos + 1);
                    if (_wcsicmp(extension.c_str(), L"dll") != 0 && _wcsicmp(extension.c_str(), L"DLL") != 0) {
                        slg::CreateInfoBarAndDisplay(L"错误", L"文件不是DLL类型！", InfoBarSeverity::Error, g_mainWindowInstance);
                        co_return;
                    }
                }

                HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                if (hFile == INVALID_HANDLE_VALUE) {
                    slg::CreateInfoBarAndDisplay(L"错误", L"文件不存在！", InfoBarSeverity::Error, g_mainWindowInstance);
                    co_return;
                }

                CloseHandle(hFile);
                
                if (KernelInstance::InjectDLLToProcess(pid, const_cast<PWCHAR>(path.c_str()))) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"注入成功！", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else {
                    slg::CreateInfoBarAndDisplay(L"失败", L"注入失败，错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
                }
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(L"错误", L"显示对话框失败: " + ex.message(),
                InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    slg::coroutine TaskPage::ModifyToken(ULONG pid) {
        try {
            auto dialog = winrt::make<winrt::StarlightGUI::implementation::ModifyTokenDialog>();
            dialog.XamlRoot(this->XamlRoot());

            auto result = co_await dialog.ShowAsync();

            if (result == ContentDialogResult::Primary) {
                int tokenType = dialog.Token();

                // 如果是TrustedInstaller的话要先启动服务，检测一下
                if (tokenType == 1) {
                    if (FindProcessId(L"TrustedInstaller.exe") == 0) {
                        slg::CreateInfoBarAndDisplay(L"失败", L"未启动TrustedInstaller服务！请手动启动！", InfoBarSeverity::Error, g_mainWindowInstance);
                        co_return;
                    }
                }

                if (KernelInstance::ModifyProcessToken(pid, tokenType)) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"成功修改令牌！", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else {
                    slg::CreateInfoBarAndDisplay(L"失败", L"修改令牌失败，错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
                }
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(L"错误", L"显示对话框失败: " + ex.message(),
                InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    slg::coroutine TaskPage::TerminateProcessButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (ProcessListView().SelectedItem()) {
            auto item = ProcessListView().SelectedItem().as<winrt::StarlightGUI::ProcessInfo>();

            if (item.Name() == L"StarlightGUI.exe") {
                slg::CreateInfoBarAndDisplay(L"警告", L"你要干什么？", InfoBarSeverity::Warning, g_mainWindowInstance);
                co_return;
            }

            // 管理员权限时，尝试使用内核结束
            if (TaskUtils::_TerminateProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else if (KernelInstance::_ZwTerminateProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"成功结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法结束进程: " + item.Name() + L" (" + to_hstring(item.Id()) + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::WaitAndReloadAsync(int interval) {
        auto lifetime = get_strong();

        reloadTimer.Stop();
        reloadTimer.Interval(std::chrono::milliseconds(interval));
        reloadTimer.Tick([this](auto&&, auto&&) {
            if (g_mainWindowInstance->m_openWindows.empty()) LoadProcessList();
            reloadTimer.Stop();
            });
        reloadTimer.Start();

        co_return;
    }
}
