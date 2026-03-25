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
        DontShowAgainCheckBox().Content(tbox(L"Update_DontShow"));

        this->Loaded([this](auto&&, auto&&) {
            if (IsUpdate()) {
                Title(tbox(L"Update_Found"));
                LatestVersionText().Text(LatestVersion());
                PrimaryButtonText(GetLocalizedString(L"Update_Download"));
                SecondaryButtonText(GetLocalizedString(L"Update_Cancel"));
				UpdateStackPanel().Visibility(Visibility::Visible);
				AnnouncementStackPanel().Visibility(Visibility::Collapsed);
            }
            else {
                Title(tbox(L"Update_Announcement"));
                UpdateTimeText().Text(LatestVersion());
                AnnouncementLine1().Text(GetAnLine(1));
                AnnouncementLine2().Text(GetAnLine(2));
                AnnouncementLine3().Text(GetAnLine(3));
                PrimaryButtonText(GetLocalizedString(L"Update_Confirm"));
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



