#pragma once

#include "Process_ThreadPage.g.h"
#include <map>
#include <TlHelp32.h>
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::StarlightGUI::implementation
{
    struct Process_ThreadPage : Process_ThreadPageT<Process_ThreadPage>
    {
        Process_ThreadPage();
        void SetupLocalization();

        void ThreadListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void ThreadListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        winrt::Windows::Foundation::IAsyncAction LoadThreadList();

        void ColumnHeader_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ApplySort(bool& isAscending, const std::string& column);
        void SortThreadList(bool isAscending, const std::string& column, bool updateHeader);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::ThreadInfo> m_threadList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::ThreadInfo>()
        };

        inline static bool m_isIdAscending = true;
        inline static bool m_isEThreadAscending = true;
        inline static bool m_isAddressAscending = true;
        inline static bool m_isPriorityAscending = true;
        inline static bool currentSortingOption;
        inline static std::string currentSortingType;
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct Process_ThreadPage : Process_ThreadPageT<Process_ThreadPage, implementation::Process_ThreadPage>
    {
    };
}
