#include "pch.h"
#include "UpdateDialog.xaml.h"
#if __has_include("UpdateDialog.g.cpp")
#include "UpdateDialog.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation
{
    UpdateDialog::UpdateDialog() {
        InitializeComponent();
        NewVersionAvailableText().Text(t(L"Update_NewVersionAvailable.Text"));
        CurrentVersionLabelRun().Text(t(L"Update_CurrentVersion.Text"));
        LatestVersionLabelRun().Text(t(L"Update_LatestVersion.Text"));
        UpdateDescriptionText().Text(t(L"Update_Description.Text"));
        UpdateTipText().Text(t(L"Update_Tip.Text"));
        QuarkCodeText().Text(t(L"Update_QuarkCode.Text"));
        NoDirectLinkText().Text(t(L"Update_NoDirectLink.Text"));
        UpdateTimeLabelRun().Text(t(L"Update_UpdateTimeLabel.Text"));
        DontShowAgainCheckBox().Content(tbox(L"Update_DontShow.Content"));

        this->Loaded([this](auto&&, auto&&) {
            if (IsUpdate()) {
                Title(tbox(L"Update_Found"));
                LatestVersionText().Text(LatestVersion());
                PrimaryButtonText(t(L"Update_Download"));
                SecondaryButtonText(t(L"Update_Cancel"));
				UpdateStackPanel().Visibility(Visibility::Visible);
				AnnouncementStackPanel().Visibility(Visibility::Collapsed);
            }
            else {
                Title(tbox(L"Update_Announcement"));
                UpdateTimeText().Text(LatestVersion());
                AnnouncementLine1().Text(GetAnLine(1));
                AnnouncementLine2().Text(GetAnLine(2));
                AnnouncementLine3().Text(GetAnLine(3));
                PrimaryButtonText(t(L"Update_Confirm"));
                UpdateStackPanel().Visibility(Visibility::Collapsed);
                AnnouncementStackPanel().Visibility(Visibility::Visible);
            }
            });
    }

    void UpdateDialog::OnPrimaryButtonClick(ContentDialog const& sender,
        ContentDialogButtonClickEventArgs const& args)
    {
        auto deferral = args.GetDeferral();

        if (!IsUpdate() && DontShowAgainCheckBox().IsChecked().GetBoolean()) {
			LOG_INFO(L"", L"Opted to not show announcements again today.");
            SaveConfig("last_announcement_date", GetDateAsInt());
        }

        deferral.Complete();
    }
}



