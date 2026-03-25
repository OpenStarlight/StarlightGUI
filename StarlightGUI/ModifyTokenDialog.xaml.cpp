#include "pch.h"
#include "ModifyTokenDialog.xaml.h"
#if __has_include("ModifyTokenDialog.g.cpp")
#include "ModifyTokenDialog.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// 了解更多 WinUI、WinUI 项目结构与项目模板，请参见：
// http://aka.ms/winui-project-info.

namespace winrt::StarlightGUI::implementation
{
    ModifyTokenDialog::ModifyTokenDialog()
    {
        InitializeComponent();

        this->Title(tbox(L"ModifyToken_Dialog.Title"));
    }

    void ModifyTokenDialog::OnPrimaryButtonClick(ContentDialog const& sender,
        ContentDialogButtonClickEventArgs const& args)
    {
        auto deferral = args.GetDeferral();

        m_token = TokenComboBox().SelectedIndex();

        deferral.Complete();
    }
}



