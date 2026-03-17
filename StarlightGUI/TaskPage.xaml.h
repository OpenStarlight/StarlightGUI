#pragma once

#include "TaskPage.g.h"
#include <map>
#include <TlHelp32.h>
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::StarlightGUI::implementation
{
    struct TaskPage : TaskPageT<TaskPage>
    {
        TaskPage();

        slg::coroutine RefreshProcessListButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine TerminateProcessButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine CreateProcessButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine InjectDLL(ULONG pid);
        slg::coroutine ModifyToken(ULONG pid);

        void ProcessListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void ProcessListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        void ColumnHeader_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        slg::coroutine ApplySort(bool& isAscending, const std::string& column);
        void ProcessSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        bool ApplyFilter(const winrt::StarlightGUI::ProcessInfo& process, hstring& query);

        winrt::Windows::Foundation::IAsyncAction LoadProcessList();
        winrt::Windows::Foundation::IAsyncAction WaitAndReloadAsync(int interval);
        winrt::Windows::Foundation::IAsyncAction GetProcessInfoAsync(const PROCESSENTRY32W& pe32, std::vector<winrt::StarlightGUI::ProcessInfo>& processes);
        winrt::Windows::Foundation::IAsyncAction GetProcessIconAsync(const winrt::StarlightGUI::ProcessInfo& process);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::ProcessInfo> m_processList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::ProcessInfo>()
        };

        bool m_isLoadingProcesses = false;
        winrt::Microsoft::UI::Xaml::DispatcherTimer reloadTimer;

        inline static bool m_isLoading = false;
        inline static bool m_isNameAscending = true;
        inline static bool m_isCpuAscending = true;
        inline static bool m_isMemoryAscending = true;
        inline static bool m_isIdAscending = true;
        inline static bool currentSortingOption;
        inline static std::string currentSortingType;
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct TaskPage : TaskPageT<TaskPage, implementation::TaskPage>
    {
    };
}
