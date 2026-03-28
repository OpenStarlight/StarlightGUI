#pragma once

#include "TaskPage.g.h"
#include <map>
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::StarlightGUI::implementation
{
    struct TaskPage : TaskPageT<TaskPage>
    {
        TaskPage();
        void SetupLocalization();

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
        void SortProcessList(bool isAscending, const std::string& column, bool updateHeader);
        void ProcessSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& e);
        void ProcessSearchBox_SuggestionChosen(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxSuggestionChosenEventArgs const& e);
        void ProcessSearchBox_QuerySubmitted(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& e);
        bool ApplyFilter(const winrt::StarlightGUI::ProcessInfo& process, hstring& query);

        winrt::Windows::Foundation::IAsyncAction LoadProcessList(bool fullReload = true);
        winrt::Windows::Foundation::IAsyncAction LoadMetaForCurrentList(std::vector<winrt::StarlightGUI::ProcessInfo> processes, uint64_t loadToken, bool fullReload);
        winrt::Windows::Foundation::IAsyncAction WaitAndReloadAsync(int interval);
        winrt::Windows::Foundation::IAsyncAction GetProcessIconAsync(winrt::StarlightGUI::ProcessInfo process);
        void UpdateRealizedItemIcon(winrt::StarlightGUI::ProcessInfo const& process, winrt::Microsoft::UI::Xaml::Media::ImageSource const& icon);
        void UpdateRealizedItemDescription(winrt::StarlightGUI::ProcessInfo const& process, winrt::hstring const& description);
        void UpdateRealizedItemMetrics(winrt::StarlightGUI::ProcessInfo const& process);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::ProcessInfo> m_processList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::ProcessInfo>()
        };

        bool m_isLoadingProcesses = false;
        bool m_isPostLoading = false;
        bool m_isSorting = false;
        uint64_t m_currentLoadToken = 0;
        uint64_t m_reloadRequestVersion = 0;
        winrt::Microsoft::UI::Xaml::DispatcherTimer autoRefreshTimer;

        inline static bool m_isLoading = false;
        inline static bool m_isNameAscending = true;
        inline static bool m_isEProcessAscending = true;
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
