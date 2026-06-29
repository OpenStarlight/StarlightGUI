#include "pch.h"
#include "UpdateDialog.xaml.h"
#if __has_include("UpdateDialog.g.cpp")
#include "UpdateDialog.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation
{
    void UpdateDialog::Announcement(const hstring& value)
    {
        std::wstring text{ value };
        size_t pos = 0;

        while ((pos = text.find(L"\\r\\n", pos)) != std::wstring::npos) {
            text.replace(pos, 4, L"\n");
            ++pos;
        }

        pos = 0;
        while ((pos = text.find(L"\\n", pos)) != std::wstring::npos) {
            text.replace(pos, 2, L"\n");
            ++pos;
        }

        pos = 0;
        while ((pos = text.find(L"\\r", pos)) != std::wstring::npos) {
            text.replace(pos, 2, L"\n");
            ++pos;
        }

        m_announcement = hstring{ text };
    }

    UpdateDialog::UpdateDialog() {
        InitializeComponent();
        this->RequestedTheme(slg::GetConfiguredElementTheme());
        NewVersionAvailableText().Text(t(L"Update.Text.NewVersionAvailable"));
        CurrentVersionLabelRun().Text(t(L"Update.Text.CurrentVersion"));
        LatestVersionLabelRun().Text(t(L"Update.Text.LatestVersion"));
        UpdateDescriptionText().Text(t(L"Update.Text.Desc"));
        UpdateTipText().Text(t(L"Update.Text.Tip"));
        QuarkCodeText().Text(t(L"Update.Text.QuarkCode"));
        NoDirectLinkText().Text(t(L"Update.Text.NoDirectLink"));
        UpdateTimeLabelRun().Text(t(L"Update.Text.UpdateTimeLabel"));
        DontShowAgainCheckBox().Content(tbox(L"Update.Text.DontShow"));

        this->Loaded([this](auto&&, auto&&) {
            if (IsUpdate()) {
                Title(tbox(L"Update.Found"));
                LatestVersionText().Text(LatestVersion());
                PrimaryButtonText(t(L"Update.Download"));
                SecondaryButtonText(t(L"Update.Cancel"));
				UpdateStackPanel().Visibility(Visibility::Visible);
				AnnouncementStackPanel().Visibility(Visibility::Collapsed);
            }
            else {
                Title(tbox(L"Update.Announcement"));
                UpdateTimeText().Text(LatestVersion());
                AnnouncementText().Text(Announcement());
                PrimaryButtonText(t(L"Update.Confirm"));
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

