#include "pch.h"
#include "RunProcessDialog.xaml.h"
#if __has_include("RunProcessDialog.g.cpp")
#include "RunProcessDialog.g.cpp"
#endif
#include <shellapi.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::StarlightGUI::implementation
{
    RunProcessDialog::RunProcessDialog()
    {
        InitializeComponent();

        this->Title(tbox(L"RunProcess_Dialog.Title"));
        this->PrimaryButtonText(t(L"RunProcess_Dialog.PrimaryButtonText"));
        this->SecondaryButtonText(t(L"RunProcess_Dialog.SecondaryButtonText"));
        RunProcessDescriptionText().Text(t(L"RunProcess_Description.Text"));
        ProcessPathTextBox().PlaceholderText(t(L"RunProcess_Path.PlaceholderText"));
        PermissionComboBox().Text(t(L"RunProcess_Permission.Text"));
        FullPrivilegesCheckBox().Content(tbox(L"RunProcess_FullPrivileges.Content"));
    }

    void RunProcessDialog::OnPrimaryButtonClick(ContentDialog const& sender,
        ContentDialogButtonClickEventArgs const& args)
    {
        auto deferral = args.GetDeferral();

        m_processPath = ProcessPathTextBox().Text();
        m_permission = PermissionComboBox().SelectedIndex();
        m_fullPrivileges = FullPrivilegesCheckBox().IsChecked().GetBoolean();

        std::wstring wideProcessName = std::wstring_view(m_processPath.c_str()).data();

        if (wideProcessName.find(L"\"") != std::wstring::npos) {
            wideProcessName.erase(wideProcessName.end());
            wideProcessName.erase(wideProcessName.begin());
        }

        if (wideProcessName.find(L".exe") == std::wstring::npos) {
            wideProcessName += L".exe";
        }

        m_processPath = wideProcessName;

        deferral.Complete();
    }
}



