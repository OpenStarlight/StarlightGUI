#pragma once

#include "Process_ModulePage.g.h"
#include "Utils/MokuaiInfo.h"
#include "Utils/ProcessInfo.h"
#include <winrt/Microsoft.UI.Xaml.Navigation.h>
#include <map>
#include <TlHelp32.h>
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::StarlightGUI::implementation
{
    struct Process_ModulePage : Process_ModulePageT<Process_ModulePage>
    {
        Process_ModulePage();
        void SetupLocalization();
        void OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

        void ModuleListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void ModuleListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        winrt::Windows::Foundation::IAsyncAction LoadModuleList();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::MokuaiInfo> m_moduleList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::MokuaiInfo>()
        };
        winrt::StarlightGUI::ProcessInfo m_process{ nullptr };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct Process_ModulePage : Process_ModulePageT<Process_ModulePage, implementation::Process_ModulePage>
    {
    };
}
