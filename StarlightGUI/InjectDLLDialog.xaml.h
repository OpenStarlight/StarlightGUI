#pragma once

#include "InjectDLLDialog.g.h"
#include "Utils/Coroutine.h"

namespace winrt::StarlightGUI::implementation
{
    struct InjectDLLDialog : InjectDLLDialogT<InjectDLLDialog>
    {
        InjectDLLDialog();

        hstring DLLPath() const { return m_DLLPath; }

        void OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args);
        slg::coroutine ExploreButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        hstring m_DLLPath{ L"" };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct InjectDLLDialog : InjectDLLDialogT<InjectDLLDialog, implementation::InjectDLLDialog>
    {
    };
}
