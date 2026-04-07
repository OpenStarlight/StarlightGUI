#pragma once

#include "KernelModulePage.g.h"
#include <map>
#include <TlHelp32.h>
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::StarlightGUI::implementation
{
    struct KernelModulePage : KernelModulePageT<KernelModulePage>
    {
        KernelModulePage();
		void SetupLocalization();

        slg::coroutine RefreshKernelModuleListButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine UnloadModuleButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine LoadDriverButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void KernelModuleListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void KernelModuleListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        void ColumnHeader_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine ApplySort(bool& isAscending, const std::string& column);
        void SortKernelModuleList(bool isAscending, const std::string& column, bool updateHeader);

        void KernelModuleSearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& e);
        void KernelModuleSearchBox_SuggestionChosen(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxSuggestionChosenEventArgs const& e);
        void KernelModuleSearchBox_QuerySubmitted(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& e);
        bool ApplyFilter(const winrt::StarlightGUI::KernelModuleInfo& kernelModule, hstring& query);

        winrt::Windows::Foundation::IAsyncAction LoadKernelModuleList();
        winrt::Windows::Foundation::IAsyncAction WaitAndReloadAsync(int interval);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::KernelModuleInfo> m_kernelModuleList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::KernelModuleInfo>()
        };

        bool m_isLoadingKernelModules = false;
        uint64_t m_reloadRequestVersion = 0;

        inline static bool m_isLoading = false;
        inline static bool m_isNameAscending = true;
        inline static bool m_isImageBaseAscending = true;
        inline static bool m_isDriverObjectAscending = true;
        inline static bool m_isSizeAscending = true;
        inline static bool m_isIndexAscending = true;
        inline static bool currentSortingOption;
        inline static std::string currentSortingType;
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct KernelModulePage : KernelModulePageT<KernelModulePage, implementation::KernelModulePage>
    {
    };
}
