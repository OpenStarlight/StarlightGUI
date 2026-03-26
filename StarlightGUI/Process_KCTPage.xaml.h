#pragma once

#include "Process_KCTPage.g.h"
#include <map>
#include <TlHelp32.h>
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::StarlightGUI::implementation
{
    struct Process_KCTPage : Process_KCTPageT<Process_KCTPage>
    {
        Process_KCTPage();
        void SetupLocalization();

        void KCTListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void KCTListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        winrt::Windows::Foundation::IAsyncAction LoadKCTList();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::KCTInfo> m_kctList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::KCTInfo>()
        };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct Process_KCTPage : Process_KCTPageT<Process_KCTPage, implementation::Process_KCTPage>
    {
    };
}
