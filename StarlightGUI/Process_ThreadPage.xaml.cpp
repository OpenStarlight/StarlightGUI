#include "pch.h"
#include "Process_ThreadPage.xaml.h"
#if __has_include("Process_ThreadPage.g.cpp")
#include "Process_ThreadPage.g.cpp"
#endif


#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <array>
#include <sstream>
#include <iomanip>
#include <Utils/Utils.h>
#include <Utils/TaskUtils.h>
#include <Utils/KernelBase.h>
#include <InfoWindow.xaml.h>
#include <MainWindow.xaml.h>

using namespace winrt;
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
    static int safeAcceptedPID = -1;

    Process_ThreadPage::Process_ThreadPage() {
        InitializeComponent();

        ThreadListView().ItemsSource(m_threadList);

        this->Loaded([this](auto&&, auto&&) {
            LoadThreadList();
            });

        LOG_INFO(L"Process_ThreadPage", L"Process_ThreadPage initialized.");
    }

    void Process_ThreadPage::ThreadListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
    {
        auto listView = sender.as<ListView>();

        slg::SelectItemOnRightTapped(listView, e);

        if (!listView.SelectedItem()) return;

        auto item = listView.SelectedItem().as<winrt::StarlightGUI::ThreadInfo>();

        auto flyoutStyles = slg::GetStyles();

        MenuFlyout menuFlyout;

        auto itemRefresh = slg::CreateMenuItem(flyoutStyles, L"\ue72c", GetLocalizedString(L"ProcThread_Refresh").c_str(), [this](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            LoadThreadList();
            co_return;
            });

        MenuFlyoutSeparator separatorR;

        // 选项1.1
        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", GetLocalizedString(L"ProcThread_Terminate").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::_TerminateThread(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"ProcThread_TerminateSuccess") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
                LoadThreadList();
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"ProcThread_TerminateFailed") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });

        // 选项1.2
        auto item1_2 = slg::CreateMenuItem(flyoutStyles, L"\ue8f0", GetLocalizedString(L"ProcThread_TerminateKernel").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_ZwTerminateThread(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"ProcThread_TerminateSuccess") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
                LoadThreadList();
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"ProcThread_TerminateFailed") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });

        // 选项1.3
        auto item1_3 = slg::CreateMenuItem(flyoutStyles, L"\ue945", GetLocalizedString(L"ProcThread_TerminateMurder").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (safeAcceptedPID == item.Id() || !dangerous_confirm) {
                if (KernelInstance::MurderThread(item.Id())) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"ProcThread_TerminateSuccess") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
                    LoadThreadList();
                }
                else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"ProcThread_TerminateFailed") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            }
            else {
                safeAcceptedPID = item.Id();
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"ProcThread_DangerousWarning").c_str(), InfoBarSeverity::Warning, g_infoWindowInstance);
            }
            co_return;
            });

        // 分割线1
        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue912", GetLocalizedString(L"ProcThread_SetState").c_str());
        auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue769", GetLocalizedString(L"ProcThread_Suspend").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_SuspendThread(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"ProcThread_SuspendSuccess") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
                LoadThreadList();
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"ProcThread_SuspendFailed") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub1);
        auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\ue768", GetLocalizedString(L"ProcThread_Resume").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (KernelInstance::_ResumeThread(item.Id())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), (GetLocalizedString(L"ProcThread_ResumeSuccess") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
                LoadThreadList();
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"ProcThread_ResumeFailed") + L": " + item.Address() + L" (" + to_hstring(item.Id()) + L")" + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item2_1.Items().Append(item2_1_sub2);

        // 分割线2
        MenuFlyoutSeparator separator2;

        // 选项3.1
        auto item3_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", GetLocalizedString(L"ProcThread_CopyInfo").c_str());
        auto item3_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", L"TID", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(std::to_wstring(item.Id()))) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub1);
        auto item3_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", L"ETHREAD", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.EThread().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub2);
        auto item3_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", GetLocalizedString(L"ProcThread_Address").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(item.Address().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_infoWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), (GetLocalizedString(L"Msg_CopyFailed") + GetLocalizedString(L"Msg_ErrorCode") + to_hstring((int)GetLastError())).c_str(), InfoBarSeverity::Error, g_infoWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub3);
        
        menuFlyout.Items().Append(itemRefresh);
        menuFlyout.Items().Append(separatorR);
        menuFlyout.Items().Append(item1_1);
        menuFlyout.Items().Append(item1_2);
        menuFlyout.Items().Append(item1_3);
        menuFlyout.Items().Append(separator1);
        menuFlyout.Items().Append(item2_1);
        menuFlyout.Items().Append(separator2);
        menuFlyout.Items().Append(item3_1);

        slg::ShowAt(menuFlyout, listView, e);
    }

    void Process_ThreadPage::ThreadListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {

    }

    winrt::Windows::Foundation::IAsyncAction Process_ThreadPage::LoadThreadList()
    {
        if (!processForInfoWindow) co_return;

        LOG_INFO(__WFUNCTION__, L"Loading thread list... (pid=%d)", processForInfoWindow.Id());
        m_threadList.Clear();
        LoadingRing().IsActive(true);

        auto start = std::chrono::high_resolution_clock::now();

        auto lifetime = get_strong();

        co_await winrt::resume_background();

        std::vector<winrt::StarlightGUI::ThreadInfo> threads;
        threads.reserve(100);

        // 获取线程列表
        KernelInstance::EnumProcessThreads(processForInfoWindow.EProcessULong(), threads);
        LOG_INFO(__WFUNCTION__, L"Enumerated threads, %d entry(s).", threads.size());

        co_await wil::resume_foreground(DispatcherQueue());

        for (const auto& thread : threads) {
            if (thread.ModuleInfo().empty()) thread.ModuleInfo(GetLocalizedString(L"ProcThread_Unknown"));

            m_threadList.Append(thread);
        }

        ApplySort(currentSortingOption, currentSortingType);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 更新线程数量文本
        std::wstringstream countText;
        countText << GetLocalizedString(L"ProcThread_CountPrefix").c_str() << L" " << m_threadList.Size() << L" " << GetLocalizedString(L"ProcThread_CountSuffix").c_str() << L" (" << duration.count() << " ms)";
        ThreadCountText().Text(countText.str());
        LoadingRing().IsActive(false);

        LOG_INFO(__WFUNCTION__, L"Loaded thread list, %d entry(s) in total.", m_threadList.Size());
    }


    void Process_ThreadPage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();

        struct SortBinding {
            wchar_t const* tag;
            char const* column;
            bool* ascending;
        };

        static const std::array<SortBinding, 4> bindings{ {
            { L"Id", "Id", &Process_ThreadPage::m_isIdAscending },
            { L"EThread", "EThread", &Process_ThreadPage::m_isEThreadAscending },
            { L"Address", "Address", &Process_ThreadPage::m_isAddressAscending },
            { L"Priority", "Priority", &Process_ThreadPage::m_isPriorityAscending },
        } };

        for (auto const& binding : bindings) {
            if (columnName == binding.tag) {
                ApplySort(*binding.ascending, binding.column);
                break;
            }
        }
    }

    // 排序切换
    void Process_ThreadPage::ApplySort(bool& isAscending, const std::string& column)
    {
        SortThreadList(isAscending, column, true);

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;
    }

    void Process_ThreadPage::SortThreadList(bool isAscending, const std::string& column, bool updateHeader)
    {
        if (column.empty()) return;

        enum class SortColumn {
            Unknown,
            Id,
            EThread,
            Address,
            Priority
        };

        auto resolveSortColumn = [&](const std::string& key) -> SortColumn {
            if (key == "Id") return SortColumn::Id;
            if (key == "EThread") return SortColumn::EThread;
            if (key == "Address") return SortColumn::Address;
            if (key == "Priority") return SortColumn::Priority;
            return SortColumn::Unknown;
            };

        auto activeColumn = resolveSortColumn(column);
        if (activeColumn == SortColumn::Unknown) return;

        if (updateHeader) {
            IdHeaderButton().Content(box_value(L"TID"));
            EThreadHeaderButton().Content(box_value(L"ETHREAD"));
            AddressHeaderButton().Content(tbox(L"ProcThread_HeaderAddress"));
            PriorityHeaderButton().Content(tbox(L"ProcThread_HeaderPriority"));

            if (activeColumn == SortColumn::Id) IdHeaderButton().Content(box_value(isAscending ? L"TID \u2193" : L"TID \u2191"));
            if (activeColumn == SortColumn::EThread) EThreadHeaderButton().Content(box_value(isAscending ? L"ETHREAD \u2193" : L"ETHREAD \u2191"));
            if (activeColumn == SortColumn::Address) AddressHeaderButton().Content(box_value(isAscending ? GetLocalizedString(L"ProcThread_HeaderAddress") + L" \u2193" : GetLocalizedString(L"ProcThread_HeaderAddress") + L" \u2191"));
            if (activeColumn == SortColumn::Priority) PriorityHeaderButton().Content(box_value(isAscending ? GetLocalizedString(L"ProcThread_HeaderPriority") + L" \u2193" : GetLocalizedString(L"ProcThread_HeaderPriority") + L" \u2191"));
        }

        std::vector<winrt::StarlightGUI::ThreadInfo> sortedThreads;
        sortedThreads.reserve(m_threadList.Size());
        for (auto const& process : m_threadList) {
            sortedThreads.push_back(process);
        }

        auto parseHex = [](winrt::hstring const& text) -> ULONG64 {
            ULONG64 value = 0;
            if (HexStringToULong(text.c_str(), value)) return value;
            return 0;
            };

        auto sortActiveColumn = [&](const winrt::StarlightGUI::ThreadInfo& a, const winrt::StarlightGUI::ThreadInfo& b) -> bool {
            switch (activeColumn) {
            case SortColumn::Id:
                return a.Id() < b.Id();
            case SortColumn::EThread:
            {
                auto aValue = parseHex(a.EThread());
                auto bValue = parseHex(b.EThread());
                if (aValue != bValue) return aValue < bValue;
                return a.EThread() < b.EThread();
            }
            case SortColumn::Address:
            {
                auto aValue = parseHex(a.Address());
                auto bValue = parseHex(b.Address());
                if (aValue != bValue) return aValue < bValue;
                return a.Address() < b.Address();
            }
            case SortColumn::Priority:
                return a.Priority() < b.Priority();
            default:
                return false;
            }
            };

        if (isAscending) {
            std::sort(sortedThreads.begin(), sortedThreads.end(), sortActiveColumn);
        }
        else {
            std::sort(sortedThreads.begin(), sortedThreads.end(), [&](const auto& a, const auto& b) {
                return sortActiveColumn(b, a);
                });
        }

        m_threadList.Clear();
        for (auto& thread : sortedThreads) {
            m_threadList.Append(thread);
        }
    }
}





