#pragma once

#include "DeuteriumPage.g.h"

namespace winrt::StarlightGUI::implementation
{
    struct DeuteriumPage : DeuteriumPageT<DeuteriumPage>
    {
        DeuteriumPage();

        IAsyncAction ExecuteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        IAsyncAction AllocButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        IAsyncAction FreeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void StackListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);
        void ParamCountSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);

        bool BuildDeuteriumInvokeRequest(DEUTERIUM_PROXY_INVOKE& function);
        bool DeuteriumInvoke(DEUTERIUM_PROXY_INVOKE& function);
        bool CheckCode(int code, int index);
        int CheckParam(DEUTERIUM_PROXY_VAR_TYPE varType, hstring value);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::GeneralEntry> m_stackList{
    winrt::single_threaded_observable_vector<winrt::StarlightGUI::GeneralEntry>()
        };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct DeuteriumPage : DeuteriumPageT<DeuteriumPage, implementation::DeuteriumPage>
    {
    };
}
