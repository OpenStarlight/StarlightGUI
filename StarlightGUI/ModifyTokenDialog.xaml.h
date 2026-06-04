#pragma once

#include "ModifyTokenDialog.g.h"

namespace winrt::StarlightGUI::implementation
{
    struct ModifyTokenDialog : ModifyTokenDialogT<ModifyTokenDialog>
    {
        ModifyTokenDialog();

        ULONG TargetPID() const { return m_targetPID; }

        void OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args);

    private:
        ULONG m_targetPID{ 0 };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct ModifyTokenDialog : ModifyTokenDialogT<ModifyTokenDialog, implementation::ModifyTokenDialog>
    {
    };
}
