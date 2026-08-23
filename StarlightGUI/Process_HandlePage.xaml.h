#pragma once

#include "Process_HandlePage.g.h"
#include "Utils/HandleInfo.h"
#include "Utils/ProcessInfo.h"
#include <winrt/Microsoft.UI.Xaml.Navigation.h>
#include <map>
#include <TlHelp32.h>
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::StarlightGUI::implementation
{
    struct Process_HandlePage : Process_HandlePageT<Process_HandlePage>
    {
        Process_HandlePage();
        void SetupLocalization();
        void OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

        void HandleListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void HandleListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        winrt::Windows::Foundation::IAsyncAction LoadHandleList();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::HandleInfo> m_handleList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::HandleInfo>()
        };
        winrt::StarlightGUI::ProcessInfo m_process{ nullptr };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct Process_HandlePage : Process_HandlePageT<Process_HandlePage, implementation::Process_HandlePage>
    {
    };
}
