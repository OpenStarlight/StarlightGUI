#pragma once

#include "UtilityPage.g.h"

namespace winrt::StarlightGUI::implementation
{
    struct UtilityPage : UtilityPageT<UtilityPage>
    {
        UtilityPage();
        void SetupLocalization();

        slg::coroutine Button_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine LoadDriverHypervisorButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct UtilityPage : UtilityPageT<UtilityPage, implementation::UtilityPage>
    {
    };
}
