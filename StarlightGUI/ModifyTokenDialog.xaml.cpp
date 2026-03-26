#include "pch.h"
#include "ModifyTokenDialog.xaml.h"
#if __has_include("ModifyTokenDialog.g.cpp")
#include "ModifyTokenDialog.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation
{
    ModifyTokenDialog::ModifyTokenDialog()
    {
        InitializeComponent();

        this->Title(tbox(L"ModifyToken.Title"));
        this->PrimaryButtonText(t(L"ModifyToken.ButtonPrimary"));
        this->SecondaryButtonText(t(L"ModifyToken.ButtonSecondary"));
        ModifyTokenDescriptionText().Text(t(L"ModifyToken.Desc"));
        TokenComboBox().Text(t(L"ModifyToken.Token"));
    }

    void ModifyTokenDialog::OnPrimaryButtonClick(ContentDialog const& sender,
        ContentDialogButtonClickEventArgs const& args)
    {
        auto deferral = args.GetDeferral();

        m_token = TokenComboBox().SelectedIndex();

        deferral.Complete();
    }
}



