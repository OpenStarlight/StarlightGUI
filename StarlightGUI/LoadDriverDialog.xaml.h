#pragma once

#include "LoadDriverDialog.g.h"
#include "Utils/Coroutine.h"

namespace winrt::StarlightGUI::implementation
{
    struct LoadDriverDialog : LoadDriverDialogT<LoadDriverDialog>
    {
        LoadDriverDialog();

        hstring DriverPath() const { return m_driverPath; }
        bool Bypass() const { return m_bypass; }

        void OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args);
        slg::coroutine ExploreButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        hstring m_driverPath{ L"" };
        bool m_bypass{ false };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct LoadDriverDialog : LoadDriverDialogT<LoadDriverDialog, implementation::LoadDriverDialog>
    {
    };
}
