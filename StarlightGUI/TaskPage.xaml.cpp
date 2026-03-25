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
#include <array>
#include <sstream>
#include <iomanip>
#include <shellapi.h>
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
    static std::wstring NormalizeCacheKey(std::wstring value)
    {
        return ToLowerCase(value);
    }

    static std::unordered_map<std::wstring, winrt::Microsoft::UI::Xaml::Media::ImageSource> iconCache;
    static std::unordered_map<std::wstring, winrt::hstring> descriptionCache;
    static int safeAcceptedPID = -1;

    TaskPage::TaskPage() {
        InitializeComponent();

        TaskTitleUid().Text(GetLocalizedString(L"Task_Title.Text"));
        ProcessCountText().Text(GetLocalizedString(L"Task_Loading.Text"));
        RefreshProcessListButton().Label(GetLocalizedString(L"Task_Refresh.Label"));
        TaskRunUid().Label(GetLocalizedString(L"Task_Run.Label"));
        TerminateProcessButton().Label(GetLocalizedString(L"Task_Terminate.Label"));
        ProcessSearchBox().PlaceholderText(GetLocalizedString(L"Task_SearchBox.PlaceholderText"));
        NameHeaderButton().Content(tbox(L"Task_ColProcess.Content"));
        MemoryHeaderButton().Content(tbox(L"Task_ColMemory.Content"));
        StatusHeaderButton().Content(tbox(L"Task_ColStatus.Content"));

        ProcessListView().ItemsSource(m_processList);
        ProcessListView().SizeChanged([weak = get_weak()](auto&&, auto&&) {
            if (auto self = weak.get()) {
                slg::UpdateVisibleListViewMarqueeByNames(
                    self->ProcessListView(),
                    self->m_processList.Size(),
                    L"ProcessTextContainer",
                    L"ProcessDescriptionTextBlock",
                    L"ProcessDescriptionMarquee");
            }
            });
        autoRefreshTimer.Interval(std::chrono::seconds(2));
        autoRefreshTimer.Tick([this](auto&&, auto&&) {
            if (!task_auto_refresh) return;
            if (!IsLoaded()) return;
            if (m_isSorting) return;
            if (m_isLoadingProcesses || m_isPostLoading) return;
            if (!g_mainWindowInstance->m_openWindows.empty()) return;

            LoadProcessList(false);
            });

        TaskUtils::EnsurePrivileges();

        this->Loaded([this](auto&&, auto&&) {
            autoRefreshTimer.Start();
            LoadProcessList();
			});

        this->Unloaded([this](auto&&, auto&&) {
            ++m_reloadRequestVersion;
            autoRefreshTimer.Stop();
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
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Msg_WhatAreYouDoing").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
            return;
        }

        MenuFlyout menuFlyout;

        // 选项1.1
        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", GetLocalizedString(L"TaskMenu_Terminate").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::_TerminateProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_TerminateSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_TerminateFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项1.2
        auto item1_2 = slg::CreateMenuItem(flyoutStyles, L"\ue8f0", GetLocalizedString(L"TaskMenu_TerminateKernel").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_ZwTerminateProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_TerminateSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_TerminateFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项1.3
        auto item1_3 = slg::CreateMenuItem(flyoutStyles, L"\ue945", GetLocalizedString(L"TaskMenu_TerminateMurder").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (safeAcceptedPID == item.Id() || !dangerous_confirm) {
                if (KernelInstance::MurderProcess(item.Id())) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_TerminateSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_TerminateFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            else {
                safeAcceptedPID = item.Id();
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Task_MurderWarning").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
            }
            co_return;
            });

        // 分割线1
        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue912", GetLocalizedString(L"TaskMenu_SetStatus").c_str());
        auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue769", GetLocalizedString(L"TaskMenu_Suspend").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_SuspendProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_SuspendSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SuspendFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub1);
        auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\ue768", GetLocalizedString(L"TaskMenu_Resume").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_ResumeProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_ResumeSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_ResumeFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub2);

        // 选项2.2
        auto item2_2 = slg::CreateMenuItem(flyoutStyles, L"\ued1a", GetLocalizedString(L"TaskMenu_Hide").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::HideProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_HideSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_HideFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项2.3
        auto item2_3 = slg::CreateMenuSubItem(flyoutStyles, L"\uea18", GetLocalizedString(L"TaskMenu_SetPPL").c_str());
        
        // PPL等级
        auto item2_3_sub1 = slg::CreateMenuItem(flyoutStyles, L"None", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_None)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_SetPPLSuccess_None").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetPPLFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub1);
        auto item2_3_sub2 = slg::CreateMenuItem(flyoutStyles, L"Authenticode", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Authenticode)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_SetPPLSuccess_Authenticode").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetPPLFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub2);
        auto item2_3_sub3 = slg::CreateMenuItem(flyoutStyles, L"Codegen", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Codegen)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_SetPPLSuccess_Codegen").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetPPLFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub3);
        auto item2_3_sub4 = slg::CreateMenuItem(flyoutStyles, L"Antimalware", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Antimalware)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_SetPPLSuccess_Antimalware").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetPPLFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub4);
        auto item2_3_sub5 = slg::CreateMenuItem(flyoutStyles, L"Lsa", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Lsa)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_SetPPLSuccess_Lsa").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetPPLFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub5);
        auto item2_3_sub6 = slg::CreateMenuItem(flyoutStyles, L"Windows", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_Windows)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_SetPPLSuccess_Windows").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetPPLFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub6);
        auto item2_3_sub7 = slg::CreateMenuItem(flyoutStyles, L"WinTcb", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_WinTcb)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_SetPPLSuccess_WinTcb").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetPPLFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub7);
        auto item2_3_sub8 = slg::CreateMenuItem(flyoutStyles, L"WinSystem", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::SetPPL(item.Id(), PPL_WinSystem)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_SetPPLSuccess_WinSystem").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetPPLFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item2_3.Items().Append(item2_3_sub8);

        // 选项2.4
        auto item2_4 = slg::CreateMenuItem(flyoutStyles, L"\ue8c9", GetLocalizedString(L"TaskMenu_SetCritical").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (safeAcceptedPID == item.Id() || !dangerous_confirm) {
                if (KernelInstance::SetCriticalProcess(item.Id())) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_SetCriticalSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_SetCriticalFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            else {
                safeAcceptedPID = item.Id();
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Task_SetCriticalWarning").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
            }
            co_return;
            });

        // 选项2.5
        auto item2_5 = slg::CreateMenuItem(flyoutStyles, L"\uebe8", GetLocalizedString(L"TaskMenu_InjectDLL").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            InjectDLL(item.Id());
            co_return;
            });

        // 选项2.6
        auto item2_6 = slg::CreateMenuItem(flyoutStyles, L"\ue70f", GetLocalizedString(L"TaskMenu_ModifyToken").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            ModifyToken(item.Id());
            co_return;
            });

        // 选项2.7
        auto item2_7 = slg::CreateMenuItem(flyoutStyles, L"\uf1e8", GetLocalizedString(L"TaskMenu_EfficiencyMode").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::EnableProcessPerformanceMode(item)) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_EfficiencyModeSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_EfficiencyModeFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 分割线2
        MenuFlyoutSeparator separator2;

        // 选项3.1
        auto item3_1 = slg::CreateMenuItem(flyoutStyles, L"\ue946", GetLocalizedString(L"TaskMenu_MoreInfo").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            processForInfoWindow = item;
            auto infoWindow = winrt::make<InfoWindow>();
            infoWindow.Activate();
            co_return;
            });

        // 选项3.2
        auto item3_2 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", GetLocalizedString(L"TaskMenu_CopyInfo").c_str());
        auto item3_2_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue8ac", GetLocalizedString(L"TaskMenu_CopyName").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Name().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub1);
        auto item3_2_sub2 = slg::CreateMenuItem(flyoutStyles, L"\ue943", L"PID", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(std::to_wstring(item.Id()))) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub2);
        auto item3_2_sub3 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", GetLocalizedString(L"TaskMenu_CopyPath").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.ExecutablePath().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub3);
        auto item3_2_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", L"EPROCESS", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.EProcess().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_2.Items().Append(item3_2_sub4);

        // 选项3.3
        auto item3_3 = slg::CreateMenuItem(flyoutStyles, L"\uec50", GetLocalizedString(L"TaskMenu_OpenFileLocation").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::OpenFolderAndSelectFile(item.ExecutablePath().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_OpenFolderSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_OpenFolderFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 选项3.4
        auto item3_4 = slg::CreateMenuItem(flyoutStyles, L"\ue8ec", GetLocalizedString(L"TaskMenu_Properties").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::OpenFileProperties(item.ExecutablePath().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_OpenPropertiesSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_OpenPropertiesFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
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

        if (args.InRecycleQueue()) return;

        if (auto process = args.Item().try_as<winrt::StarlightGUI::ProcessInfo>()) {
            UpdateRealizedItemDescription(process, process.Description());
            UpdateRealizedItemMetrics(process);

            auto itemContainer = args.ItemContainer().try_as<winrt::Microsoft::UI::Xaml::Controls::ListViewItem>();
            if (itemContainer) {
                auto contentRoot = itemContainer.ContentTemplateRoot().try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
                if (contentRoot) {
                    slg::UpdateTextMarqueeByNames(
                        contentRoot,
                        L"ProcessTextContainer",
                        L"ProcessDescriptionTextBlock",
                        L"ProcessDescriptionMarquee");
                    DispatcherQueue().TryEnqueue([weak = get_weak(), contentRoot]() {
                        if (auto self = weak.get()) {
                            slg::UpdateTextMarqueeByNames(
                                contentRoot,
                                L"ProcessTextContainer",
                                L"ProcessDescriptionTextBlock",
                                L"ProcessDescriptionMarquee");
                        }
                        });
                }
            }

            if (process.Icon()) {
                UpdateRealizedItemIcon(process, process.Icon());
            }
            else {
                GetProcessIconAsync(process);
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::LoadProcessList(bool fullReload)
    {
        if (m_isLoadingProcesses) {
            co_return;
        }

        auto loadToken = ++m_currentLoadToken;
        m_isPostLoading = false;
        m_isLoadingProcesses = true;

        LOG_INFO(__WFUNCTION__, L"Loading process list...");
        if (fullReload) {
            LoadingRing().IsActive(true);
        }

        auto start = std::chrono::steady_clock::now();

        auto lifetime = get_strong();

        winrt::hstring query = ProcessSearchBox().Text();

        co_await winrt::resume_background();

        std::vector<winrt::StarlightGUI::ProcessInfo> processes;
        std::vector<winrt::StarlightGUI::ProcessInfo> visibleProcesses;

        processes.reserve(200);
        visibleProcesses.reserve(200);

        // 对于 Windows 11，我们使用 AstralX 进行枚举
        // 对于 Windows 10 及以下版本，我们使用 SKT64 进行枚举
        if (TaskUtils::GetWindowsBuildNumber() >= 22000 && !enum_strengthen) {
            KernelInstance::EnumProcesses(processes);
        }
        else {
            KernelInstance::EnumProcesses2(processes);
        }

        LOG_INFO(__WFUNCTION__, L"Enumerated processes, %d entry(s).", processes.size());

        co_await wil::resume_foreground(DispatcherQueue());
        if (!IsLoaded() || loadToken != m_currentLoadToken) {
            m_isLoadingProcesses = false;
            co_return;
        }
        
		// 保留选中项避免刷新后丢失选中状态
        auto selectedItem = ProcessListView().SelectedItem();
        int selectedItemId = -1;
        if (selectedItem) selectedItemId = selectedItem.as<winrt::StarlightGUI::ProcessInfo>().Id();

        auto listScrollViewer = slg::FindVisualChild<ScrollViewer>(ProcessListView());
        double verticalOffset = 0.0;
        if (listScrollViewer) verticalOffset = listScrollViewer.VerticalOffset();

        std::wstring lowerQuery;
        if (!query.empty()) lowerQuery = ToLowerCase(query.c_str());

        for (auto& process : processes) {
            bool shouldRemove = lowerQuery.empty() ? false : !ContainsIgnoreCaseLowerQuery(process.Name().c_str(), lowerQuery);
            if (shouldRemove) continue;

            // 如果有缓存的话就直接用，不在获取的时候再跑一遍了
            std::wstring path = process.ExecutablePath().c_str();
            if (!path.empty()) {
                auto key = NormalizeCacheKey(path);

                auto descIt = descriptionCache.find(key);
                if (descIt != descriptionCache.end()) process.Description(descIt->second);

                auto iconIt = iconCache.find(key);
                if (iconIt != iconCache.end()) process.Icon(iconIt->second);
            }

            visibleProcesses.push_back(process);
        }

        std::vector<winrt::StarlightGUI::ProcessInfo> currentProcesses;

        if (fullReload) {
            m_processList.Clear();
            for (auto const& process : visibleProcesses) {
                m_processList.Append(process);
            }

            // 恢复排序
            ApplySort(currentSortingOption, currentSortingType);

            currentProcesses.reserve(m_processList.Size());
            for (auto const& process : m_processList) currentProcesses.push_back(process);
        }
        else {
            // 增量更新，不改整体列表，只更新有变化的项目
            std::unordered_map<int32_t, winrt::StarlightGUI::ProcessInfo> incomingByPid;
            incomingByPid.reserve(visibleProcesses.size());
            for (auto const& process : visibleProcesses) {
                incomingByPid[process.Id()] = process;
            }

            std::unordered_map<int32_t, winrt::StarlightGUI::ProcessInfo> existingByPid;
            existingByPid.reserve(m_processList.Size());
            for (auto const& process : m_processList) {
                existingByPid[process.Id()] = process;
            }

            for (int i = static_cast<int>(m_processList.Size()) - 1; i >= 0; --i) {
                auto process = m_processList.GetAt(i);
                if (incomingByPid.find(process.Id()) == incomingByPid.end()) {
                    m_processList.RemoveAt(i);
                }
            }

            for (auto const& process : visibleProcesses) {
                auto existingIt = existingByPid.find(process.Id());
                if (existingIt == existingByPid.end()) {
                    m_processList.Append(process);
                    continue;
                }

                auto existing = existingIt->second;
                auto oldPath = std::wstring(existing.ExecutablePath().c_str());
                auto newPath = std::wstring(process.ExecutablePath().c_str());
                bool pathChanged = _wcsicmp(oldPath.c_str(), newPath.c_str()) != 0;

                existing.Name(process.Name());
                existing.ExecutablePath(process.ExecutablePath());
                existing.Status(process.Status());
                existing.EProcess(process.EProcess());
                existing.EProcessULong(process.EProcessULong());
                existing.MemoryUsageByte(process.MemoryUsageByte());

                if (!process.Description().empty()) {
                    existing.Description(process.Description());
                    UpdateRealizedItemDescription(existing, process.Description());
                }
                else if (pathChanged) {
                    existing.Description(L"");
                }

                if (pathChanged) {
                    existing.Icon(nullptr);
                }
                if (process.Icon()) {
                    existing.Icon(process.Icon());
                    UpdateRealizedItemIcon(existing, process.Icon());
                }

                UpdateRealizedItemMetrics(existing);
            }

            currentProcesses.reserve(m_processList.Size());
            for (auto const& process : m_processList) currentProcesses.push_back(process);
        }

        m_isPostLoading = true;
        LoadMetaForCurrentList(currentProcesses, loadToken, fullReload);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        // 更新进程数量文本
        std::wstringstream countText;
        countText << GetLocalizedString(L"Task_ProcessCount_Prefix").c_str() << m_processList.Size() << GetLocalizedString(L"Task_ProcessCount_Suffix").c_str() << L" (" << duration << " ms)";
        ProcessCountText().Text(countText.str());
        if (fullReload) {
            LoadingRing().IsActive(false);
        }

        if (fullReload) {
            bool restoredSelection = false;
            if (selectedItemId != -1) {
                for (auto const& process : m_processList) {
                    if (process.Id() == selectedItemId) {
                        ProcessListView().SelectedItem(process);
                        restoredSelection = true;
                        break;
                    }
                }
            }

            if (!restoredSelection) {
                ProcessListView().SelectedItem(nullptr);
            }

            if (listScrollViewer) {
                listScrollViewer.ChangeView(nullptr, verticalOffset, nullptr, true);
            }
        }
        else if (selectedItemId != -1) {
            bool foundSelected = false;
            for (auto const& process : m_processList) {
                if (process.Id() == selectedItemId) {
                    foundSelected = true;
                    break;
                }
            }
            if (!foundSelected) {
                ProcessListView().SelectedItem(nullptr);
            }
        }

        LOG_INFO(__WFUNCTION__, L"Loaded process list, %d entry(s) in total.", m_processList.Size());
        m_isLoadingProcesses = false;
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::LoadMetaForCurrentList(std::vector<winrt::StarlightGUI::ProcessInfo> processes, uint64_t loadToken, bool fullReload)
    {
        auto lifetime = get_strong();

        struct DescriptionUpdate {
            int32_t Pid{ 0 };
            std::wstring CacheKey;
            winrt::hstring Description{ GetLocalizedString(L"Task_Desc_Application") };
        };

        std::vector<DescriptionUpdate> descriptions;
        descriptions.reserve(processes.size());
        std::vector<winrt::StarlightGUI::ProcessInfo> missingDescriptionProcesses;
        missingDescriptionProcesses.reserve(processes.size());
        std::unordered_map<int32_t, winrt::hstring> descriptionTable;

        std::map<DWORD, hstring> processCpuTable;

        for (auto const& process : processes) {
            if (!process) continue;

            if (process.Name() == L"Idle" || process.Name() == L"System" || process.Name() == L"Registry" || process.Name() == L"Secure System" || process.Name() == L"Memory Compression") {
                descriptionTable[process.Id()] = GetLocalizedString(L"Task_Desc_SystemProcess");
                continue;
            }

            std::wstring path = process.ExecutablePath().c_str();
            if (path.empty()) {
                descriptionTable[process.Id()] = GetLocalizedString(L"Task_Desc_Application");
                continue;
            }

            auto key = NormalizeCacheKey(path);
            auto cacheIt = descriptionCache.find(key);
            if (cacheIt != descriptionCache.end()) {
                descriptionTable[process.Id()] = cacheIt->second;
            }
            else {
                missingDescriptionProcesses.push_back(process);
            }
        }

        co_await winrt::resume_background();

        co_await TaskUtils::FetchProcessCpuUsage(processCpuTable);

        for (auto const& process : missingDescriptionProcesses) {
            if (!process) continue;

            std::wstring description = GetLocalizedString(L"Task_Desc_Application").c_str();
            std::wstring path = process.ExecutablePath().c_str();
            std::wstring cacheKey;
            if (!path.empty()) {
                cacheKey = NormalizeCacheKey(path);
                DWORD dummy = 0;
                DWORD versionInfoSize = GetFileVersionInfoSizeW(path.c_str(), &dummy);
                if (versionInfoSize > 0) {
                    std::vector<BYTE> versionInfo(versionInfoSize);
                    if (GetFileVersionInfoW(path.c_str(), 0, versionInfoSize, versionInfo.data())) {
                        wchar_t* fileDescription = nullptr;
                        UINT descriptionSize = 0;
                        if (VerQueryValueW(versionInfo.data(), L"\\StringFileInfo\\040904b0\\FileDescription", (LPVOID*)&fileDescription, &descriptionSize) &&
                            fileDescription && descriptionSize > 0) {
                            description = fileDescription;
                        }
                    }
                }
            }
            descriptions.push_back({ process.Id(), cacheKey, winrt::hstring(description) });
        }

        co_await wil::resume_foreground(DispatcherQueue());
        if (!IsLoaded() || loadToken != m_currentLoadToken) co_return;
        for (auto const& item : descriptions) {
            descriptionTable[item.Pid] = item.Description;
            if (!item.CacheKey.empty()) {
                descriptionCache[item.CacheKey] = item.Description;
            }
        }

        for (auto const& process : processes) {
            if (!process) continue;
            if (loadToken != m_currentLoadToken) co_return;

            auto cpuIt = processCpuTable.find((DWORD)process.Id());
            if (cpuIt != processCpuTable.end()) process.CpuUsage(cpuIt->second);
            else process.CpuUsage(L"-1 " + GetLocalizedString(L"Task_Unknown"));

            if (process.MemoryUsageByte() != 0) process.MemoryUsage(FormatMemorySize(process.MemoryUsageByte()));
            else process.MemoryUsage(L"-1 " + GetLocalizedString(L"Task_Unknown"));

            if (process.Status().empty()) process.Status(GetLocalizedString(L"Task_Unknown"));
            if (process.EProcess().empty()) process.EProcess(GetLocalizedString(L"Task_Unknown"));
            UpdateRealizedItemMetrics(process);

            auto descIt = descriptionTable.find(process.Id());
            if (descIt != descriptionTable.end()) {
                process.Description(descIt->second);
                UpdateRealizedItemDescription(process, descIt->second);
            }
            else if (process.Description().empty()) {
                process.Description(GetLocalizedString(L"Task_Desc_Application"));
                UpdateRealizedItemDescription(process, process.Description());
            }

            if (!process.Icon()) {
                std::wstring path = process.ExecutablePath().c_str();
                if (!path.empty()) {
                    auto key = NormalizeCacheKey(path);
                    auto iconIt = iconCache.find(key);
                    if (iconIt != iconCache.end()) {
                        process.Icon(iconIt->second);
                        UpdateRealizedItemIcon(process, iconIt->second);
                    }
                    else {
                        co_await GetProcessIconAsync(process);
                    }
                }
                else {
                    co_await GetProcessIconAsync(process);
                }
            }
        }

        if (loadToken != m_currentLoadToken) co_return;

        if (fullReload) {
            auto selectedItem = ProcessListView().SelectedItem();
            int selectedItemId = -1;
            if (selectedItem) selectedItemId = selectedItem.as<winrt::StarlightGUI::ProcessInfo>().Id();

            auto listScrollViewer = slg::FindVisualChild<ScrollViewer>(ProcessListView());
            double verticalOffset = 0.0;
            if (listScrollViewer) verticalOffset = listScrollViewer.VerticalOffset();

            auto oldTransitions = ProcessListView().ItemContainerTransitions();
            ProcessListView().ItemContainerTransitions().Clear();
            ProcessListView().ItemsSource(nullptr);
            ProcessListView().ItemsSource(m_processList);
            ProcessListView().ItemContainerTransitions(oldTransitions);

            if (selectedItemId != -1) {
                for (auto const& process : m_processList) {
                    if (process.Id() == selectedItemId) {
                        ProcessListView().SelectedItem(process);
                        break;
                    }
                }
            }

            if (listScrollViewer) {
                listScrollViewer.ChangeView(nullptr, verticalOffset, nullptr, true);
            }
        }
        else if (!currentSortingType.empty()) {
            SortProcessList(currentSortingOption, currentSortingType, false);
        }

        m_isPostLoading = false;
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::GetProcessIconAsync(winrt::StarlightGUI::ProcessInfo process) {
        if (!process) co_return;
        auto loadToken = m_currentLoadToken;
        co_await wil::resume_foreground(DispatcherQueue());
        if (!IsLoaded() || loadToken != m_currentLoadToken) co_return;

        std::wstring path = process.ExecutablePath().c_str();
        std::wstring cacheKey = NormalizeCacheKey(path);

        // 查询缓存，有就直接用
        auto cacheIt = iconCache.find(cacheKey);
        if (cacheIt != iconCache.end()) {
            process.Icon(cacheIt->second);
            UpdateRealizedItemIcon(process, cacheIt->second);
            co_return;
        }

        auto icon = slg::GetShellIconImage(path, false, 16, false, cacheKey);

		// 系统进程使用 ntoskrnl.exe 的图标，因为没有可执行文件
        if (!icon || process.Name() == L"Idle" || process.Name() == L"System" || process.Name() == L"Registry" || process.Name() == L"Secure System" || process.Name() == L"Memory Compression" || process.Name() == L"Unknown") {
            icon = slg::GetShellIconImage(L"C:\\Windows\\System32\\ntoskrnl.exe", false, 16, false, L"__ntoskrnl__");
        }

        if (!IsLoaded() || loadToken != m_currentLoadToken) co_return;

        if (!cacheKey.empty()) iconCache.insert_or_assign(cacheKey, icon);
        process.Icon(icon);
        UpdateRealizedItemIcon(process, icon);

        co_return;
    }

    void TaskPage::UpdateRealizedItemIcon(winrt::StarlightGUI::ProcessInfo const& process, winrt::Microsoft::UI::Xaml::Media::ImageSource const& icon)
    {
        if (!process || !icon || !IsLoaded()) return;

        auto container = ProcessListView().ContainerFromItem(process).try_as<ListViewItem>();
        if (!container) return;

        auto root = container.ContentTemplateRoot();
        if (!root) return;

        auto image = slg::FindVisualChild<Image>(root);
        if (image) {
            image.Source(icon);
        }
    }

    void TaskPage::UpdateRealizedItemDescription(winrt::StarlightGUI::ProcessInfo const& process, winrt::hstring const& description)
    {
        if (!process || !IsLoaded()) return;

        auto container = ProcessListView().ContainerFromItem(process).try_as<ListViewItem>();
        if (!container) return;

        auto root = container.ContentTemplateRoot();
        if (!root) return;

        auto text = root.as<winrt::Microsoft::UI::Xaml::FrameworkElement>().FindName(L"ProcessDescriptionTextBlock").try_as<TextBlock>();
        if (text) text.Text(description);

        auto marquee = root.as<winrt::Microsoft::UI::Xaml::FrameworkElement>().FindName(L"ProcessDescriptionMarquee").try_as<winrt::WinUI3Package::MarqueeText>();
        if (marquee) marquee.Text(description);

        auto contentRoot = root.try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();
        if (contentRoot) {
            slg::UpdateTextMarqueeByNames(
                contentRoot,
                L"ProcessTextContainer",
                L"ProcessDescriptionTextBlock",
                L"ProcessDescriptionMarquee");
        }
    }

    void TaskPage::UpdateRealizedItemMetrics(winrt::StarlightGUI::ProcessInfo const& process)
    {
        if (!process || !IsLoaded()) return;

        auto container = ProcessListView().ContainerFromItem(process).try_as<ListViewItem>();
        if (!container) return;

        auto root = container.ContentTemplateRoot().try_as<Grid>();
        if (!root) return;

        for (auto const& child : root.Children()) {
            if (auto text = child.try_as<TextBlock>()) {
                int column = Grid::GetColumn(text);
                if (column == 2) {
                    text.Text(process.EProcess());
                }
                else if (column == 3) {
                    text.Text(process.CpuUsage());
                }
                else if (column == 4) {
                    text.Text(process.MemoryUsage());
                }
                else if (column == 5) {
                    text.Text(process.Status());
                }
            }
        }
    }

    void TaskPage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();
        
        struct SortBinding {
            wchar_t const* tag;
            char const* column;
            bool* ascending;
        };

        static const std::array<SortBinding, 5> bindings{ {
            { L"Name", "Name", &TaskPage::m_isNameAscending },
            { L"EProcess", "EProcess", &TaskPage::m_isEProcessAscending },
            { L"CpuUsage", "CpuUsage", &TaskPage::m_isCpuAscending },
            { L"MemoryUsage", "MemoryUsage", &TaskPage::m_isMemoryAscending },
            { L"Id", "Id", &TaskPage::m_isIdAscending },
        } };

        for (auto const& binding : bindings) {
            if (columnName == binding.tag) {
                ApplySort(*binding.ascending, binding.column);
                break;
            }
        }
    }

    // 排序切换
    slg::coroutine TaskPage::ApplySort(bool& isAscending, const std::string& column)
    {
        m_isSorting = true;
        SortProcessList(isAscending, column, true);
        m_isSorting = false;

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;

        co_return;
    }

    void TaskPage::SortProcessList(bool isAscending, const std::string& column, bool updateHeader)
    {
        if (column.empty()) return;

        enum class SortColumn {
            Unknown,
            Name,
            EProcess,
            CpuUsage,
            MemoryUsage,
            Id
        };

        auto resolveSortColumn = [&](const std::string& key) -> SortColumn {
            if (key == "Name") return SortColumn::Name;
            if (key == "EProcess") return SortColumn::EProcess;
            if (key == "CpuUsage") return SortColumn::CpuUsage;
            if (key == "MemoryUsage") return SortColumn::MemoryUsage;
            if (key == "Id") return SortColumn::Id;
            return SortColumn::Unknown;
            };

        auto activeColumn = resolveSortColumn(column);
        if (activeColumn == SortColumn::Unknown) return;

        if (updateHeader) {
            NameHeaderButton().Content(tbox(L"Task_ColProcess_Text"));
            EProcessHeaderButton().Content(box_value(L"EPROCESS"));
            CpuHeaderButton().Content(box_value(L"CPU"));
            MemoryHeaderButton().Content(tbox(L"Task_ColMemory_Text"));
            IdHeaderButton().Content(box_value(L"PID"));
        }

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

        std::unordered_map<int, double> cpuUsageCache;
        if (activeColumn == SortColumn::CpuUsage) {
            cpuUsageCache.reserve(processes.size() * 2);
            for (auto const& process : processes) {
                cpuUsageCache.insert_or_assign(process.Id(), parseCpu(process.CpuUsage()));
            }
        }

        if (updateHeader) {
            if (activeColumn == SortColumn::Name) NameHeaderButton().Content(isAscending ? tbox(L"Task_ColProcess_Down") : tbox(L"Task_ColProcess_Up"));
            if (activeColumn == SortColumn::EProcess) EProcessHeaderButton().Content(box_value(isAscending ? L"EPROCESS \u2193" : L"EPROCESS \u2191"));
            if (activeColumn == SortColumn::CpuUsage) CpuHeaderButton().Content(box_value(isAscending ? L"CPU \u2193" : L"CPU \u2191"));
            if (activeColumn == SortColumn::MemoryUsage) MemoryHeaderButton().Content(isAscending ? tbox(L"Task_ColMemory_Down") : tbox(L"Task_ColMemory_Up"));
            if (activeColumn == SortColumn::Id) IdHeaderButton().Content(box_value(isAscending ? L"PID \u2193" : L"PID \u2191"));
        }

        auto sortActiveColumn = [&](const winrt::StarlightGUI::ProcessInfo& a, const winrt::StarlightGUI::ProcessInfo& b) -> bool {
            switch (activeColumn) {
            case SortColumn::Name:
                return LessIgnoreCase(a.Name().c_str(), b.Name().c_str());
            case SortColumn::CpuUsage:
            {
                auto aIt = cpuUsageCache.find(a.Id());
                auto bIt = cpuUsageCache.find(b.Id());
                double aValue = (aIt != cpuUsageCache.end()) ? aIt->second : parseCpu(a.CpuUsage());
                double bValue = (bIt != cpuUsageCache.end()) ? bIt->second : parseCpu(b.CpuUsage());
                return aValue < bValue;
            }
            case SortColumn::EProcess:
                return a.EProcessULong() < b.EProcessULong();
            case SortColumn::MemoryUsage:
                return a.MemoryUsageByte() < b.MemoryUsageByte();
            case SortColumn::Id:
                return a.Id() < b.Id();
            default:
                return false;
            }
            };

        if (isAscending) {
            std::sort(processes.begin(), processes.end(), sortActiveColumn);
        }
        else {
            std::sort(processes.begin(), processes.end(), [&](const auto& a, const auto& b) {
                return sortActiveColumn(b, a);
                });
        }

        if (updateHeader) {
            auto selectedItem = ProcessListView().SelectedItem();
            int selectedItemId = -1;
            if (selectedItem) selectedItemId = selectedItem.as<winrt::StarlightGUI::ProcessInfo>().Id();

            auto listScrollViewer = slg::FindVisualChild<ScrollViewer>(ProcessListView());
            double verticalOffset = 0.0;
            if (listScrollViewer) verticalOffset = listScrollViewer.VerticalOffset();

            m_processList.Clear();
            for (auto const& process : processes) {
                m_processList.Append(process);
            }

            bool restoredSelection = false;
            if (selectedItemId != -1) {
                for (auto const& process : m_processList) {
                    if (process.Id() == selectedItemId) {
                        ProcessListView().SelectedItem(process);
                        restoredSelection = true;
                        break;
                    }
                }
            }
            if (!restoredSelection) {
                ProcessListView().SelectedItem(nullptr);
            }

            if (listScrollViewer) {
                listScrollViewer.ChangeView(nullptr, verticalOffset, nullptr, true);
            }
            return;
        }

        std::vector<winrt::StarlightGUI::ProcessInfo> reordered;
        reordered.reserve(m_processList.Size());
        for (auto const& process : m_processList) {
            reordered.push_back(process);
        }

        std::unordered_map<int, uint32_t> indexById;
        indexById.reserve(reordered.size() * 2);
        for (uint32_t i = 0; i < reordered.size(); ++i) {
            indexById[reordered[i].Id()] = i;
        }

        for (uint32_t targetIndex = 0; targetIndex < processes.size() && targetIndex < reordered.size(); ++targetIndex) {
            int desiredId = processes[targetIndex].Id();
            auto desiredIt = indexById.find(desiredId);
            if (desiredIt == indexById.end()) continue;

            uint32_t currentIndex = desiredIt->second;
            if (currentIndex == targetIndex) continue;

            int displacedId = reordered[targetIndex].Id();
            std::swap(reordered[targetIndex], reordered[currentIndex]);

            indexById[desiredId] = targetIndex;
            indexById[displacedId] = currentIndex;
        }

        for (uint32_t i = 0; i < reordered.size(); ++i) {
            if (m_processList.GetAt(i).Id() == reordered[i].Id()) continue;
            m_processList.SetAt(i, reordered[i]);
        }
    }

    void TaskPage::ProcessSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (!IsLoaded()) return;
        WaitAndReloadAsync(250);
    }

    bool TaskPage::ApplyFilter(const winrt::StarlightGUI::ProcessInfo& process, hstring& query) {
        return !ContainsIgnoreCase(process.Name().c_str(), query.c_str());
    }


    slg::coroutine TaskPage::RefreshProcessListButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (m_isLoadingProcesses || m_isPostLoading) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Task_StillLoading").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
            co_return;
        }

        RefreshProcessListButton().IsEnabled(false);

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
                    if (CreateProcessElevated(processPath.c_str(), fullPrivileges)) {
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_ProcessStartSuccess").c_str(),
                            InfoBarSeverity::Success, g_mainWindowInstance);
                    }
                    else {
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_ProcessStartFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(),
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
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_ProcessStartSuccess").c_str(),
                            InfoBarSeverity::Success, g_mainWindowInstance);
                    }
                    else {
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_ProcessStartFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(),
                            InfoBarSeverity::Error, g_mainWindowInstance);
                    }
                }

                WaitAndReloadAsync(1000);
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), (GetLocalizedString(L"Task_DialogFailed") + ex.message()).c_str(),
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
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Task_NotDLLFile").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                    co_return;
                }
                else {
                    std::wstring extension = path.substr(dotPos + 1);
                    if (_wcsicmp(extension.c_str(), L"dll") != 0 && _wcsicmp(extension.c_str(), L"DLL") != 0) {
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Task_NotDLLFile").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                        co_return;
                    }
                }

                HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                if (hFile == INVALID_HANDLE_VALUE) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), GetLocalizedString(L"Task_FileNotExist").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                    co_return;
                }

                CloseHandle(hFile);
                
                if (KernelInstance::InjectDLLToProcess(pid, const_cast<PWCHAR>(path.c_str()))) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_InjectSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_InjectFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                }
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), (GetLocalizedString(L"Task_DialogFailed") + ex.message()).c_str(),
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

                // 如果是 TrustedInstaller 的话要先启动服务，检测一下
                if (tokenType == 1) {
                    if (FindProcessId(L"TrustedInstaller.exe") == 0) {
                        slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"Task_TrustedInstallerNotRunning").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                        co_return;
                    }
                }

                if (KernelInstance::ModifyProcessToken(pid, tokenType)) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Task_ModifyTokenSuccess").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_ModifyTokenFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
                }
            }
        }
        catch (winrt::hresult_error const& ex) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Error").c_str(), (GetLocalizedString(L"Task_DialogFailed") + ex.message()).c_str(),
                InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    slg::coroutine TaskPage::TerminateProcessButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (ProcessListView().SelectedItem()) {
            auto item = ProcessListView().SelectedItem().as<winrt::StarlightGUI::ProcessInfo>();

            if (item.Name() == L"StarlightGUI.exe") {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"Msg_WhatAreYouDoing").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
                co_return;
            }

            // 普通无法结束时，尝试使用内核结束
            if (TaskUtils::_TerminateProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_TerminateSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else if (KernelInstance::_ZwTerminateProcess(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"Task_TerminateSuccess") + item.Name() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                WaitAndReloadAsync(1000);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Task_TerminateFailed") + item.Name() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
        }
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction TaskPage::WaitAndReloadAsync(int interval) {
        auto lifetime = get_strong();
        auto requestVersion = ++m_reloadRequestVersion;

        co_await winrt::resume_after(std::chrono::milliseconds(interval));
        co_await wil::resume_foreground(DispatcherQueue());

        if (!IsLoaded() || requestVersion != m_reloadRequestVersion) co_return;
        if (g_mainWindowInstance->m_openWindows.empty()) LoadProcessList(true);

        co_return;
    }
}






