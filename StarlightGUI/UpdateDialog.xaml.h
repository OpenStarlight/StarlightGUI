#pragma once

#include "UpdateDialog.g.h"

namespace winrt::StarlightGUI::implementation
{
    struct UpdateDialog : UpdateDialogT<UpdateDialog>
    {
        UpdateDialog();

        bool IsUpdate() const { return m_isUpdate; }
		void IsUpdate(bool value) { m_isUpdate = value; }

		int32_t Channel() const { return m_channel; }
		void Channel(int32_t value) { m_channel = value; }

        hstring LatestVersion() const { return m_latestVersion; }
        void LatestVersion(const hstring& value) { m_latestVersion = value; }

        hstring Announcement() const { return m_announcement; }
        void Announcement(const hstring& value);

        void OnPrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const& args);

    private:
        bool m_isUpdate{ false };
        int32_t m_channel{ 0 };
        hstring m_latestVersion{ L"" };
        hstring m_announcement{ L"" };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct UpdateDialog : UpdateDialogT<UpdateDialog, implementation::UpdateDialog>
    {
    };
}
