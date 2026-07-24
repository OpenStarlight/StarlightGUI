#pragma once

#include "CopyFileDialog.g.h"
#include "Utils/Coroutine.h"

namespace winrt::StarlightGUI::implementation
{
    struct CopyFileDialog : CopyFileDialogT<CopyFileDialog>
    {
        CopyFileDialog();

        hstring CopyPath() const { return m_copyPath; }

        void OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args);
        slg::coroutine ExploreButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        hstring m_copyPath{ L"" };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct CopyFileDialog : CopyFileDialogT<CopyFileDialog, implementation::CopyFileDialog>
    {
    };
}
