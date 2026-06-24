#pragma once

#include "MonitorPage.g.h"
#include <vector>

namespace winrt::StarlightGUI::implementation
{
    struct MonitorPage : MonitorPageT<MonitorPage>
    {
        MonitorPage();
		void SetupLocalization();

        void MainSegmented_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void CallbackTypeMenuItem_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void HALTableMenuItem_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine HandleSegmentedChange(int index, bool force);

        void ObjectTreeView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void ObjectListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void CallbackListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void MiniFilterListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void SSDTListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void PiDDBListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void HALTableListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);

        slg::coroutine RefreshButton_Click(IInspectable const&, RoutedEventArgs const&);

        winrt::Windows::Foundation::IAsyncAction LoadItemList();
        winrt::Windows::Foundation::IAsyncAction LoadPartitionList(std::wstring path, bool reportError = true);
        winrt::Windows::Foundation::IAsyncAction LoadObjectList();
        winrt::Windows::Foundation::IAsyncAction LoadGeneralList(bool force);
        winrt::Windows::Foundation::IAsyncAction WaitAndReloadAsync(int interval);

        void InitializeFlyout();
        void UpdateCallbackColumns();
        void SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& e);
        void SearchBox_SuggestionChosen(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxSuggestionChosenEventArgs const& e);
        void SearchBox_QuerySubmitted(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& e);
        bool ApplyFilter(const hstring& target, const hstring& query);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::ObjectEntry> m_objectList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::ObjectEntry>()
        };
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::WinUI3Package::SegmentedItem> m_itemList{
            winrt::single_threaded_observable_vector<winrt::WinUI3Package::SegmentedItem>()
        };
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::GeneralEntry> m_generalList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::GeneralEntry>()
        };

		int segmentedIndex = 0;
		int m_callbackType = 0;
		int m_halTableType = 0;
        bool m_isLoading = false;
        uint64_t m_reloadRequestVersion = 0;

        struct ColumnSyncBinding
        {
            winrt::Microsoft::UI::Xaml::Controls::Grid HeaderGrid{ nullptr };
            winrt::Microsoft::UI::Xaml::Controls::Grid BodyGrid{ nullptr };
            winrt::Microsoft::UI::Xaml::Controls::ListView ListView{ nullptr };
            uint32_t RowOffset = 0;
        };

        void InitializeColumnSyncBindings();
        void AttachColumnSyncToSection(winrt::Microsoft::UI::Xaml::Controls::Grid const& sectionRoot, uint32_t rowOffset = 0);
        static void EnsureHeaderSplitters(winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid);
        std::vector<ColumnSyncBinding> m_columnSyncBindings;

        inline static bool currentSortingOption;
        inline static hstring currentSortingType;
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct MonitorPage : MonitorPageT<MonitorPage, implementation::MonitorPage>
    {
    };
}
