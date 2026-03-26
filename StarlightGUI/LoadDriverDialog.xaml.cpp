#include "pch.h"
#include "LoadDriverDialog.xaml.h"
#if __has_include("LoadDriverDialog.g.cpp")
#include "LoadDriverDialog.g.cpp"
#endif
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Storage;
using namespace Microsoft::Windows::Storage::Pickers;

namespace winrt::StarlightGUI::implementation
{
    LoadDriverDialog::LoadDriverDialog() {
        InitializeComponent();

        this->Title(tbox(L"LoadDriver.Title"));
        this->PrimaryButtonText(t(L"LoadDriver.ButtonPrimary"));
        this->SecondaryButtonText(t(L"LoadDriver.ButtonSecondary"));
        LoadDriverDescriptionText().Text(t(L"LoadDriver.Desc"));
        DriverPathTextBox().PlaceholderText(t(L"LoadDriver.Placeholder"));
        ExploreButton().Content(tbox(L"LoadDriver.Browse"));
        BypassCheckBox().Content(tbox(L"LoadDriver.Bypass"));
    }

    void LoadDriverDialog::OnPrimaryButtonClick(ContentDialog const& sender,
        ContentDialogButtonClickEventArgs const& args)
    {
        auto deferral = args.GetDeferral();

        m_driverPath = DriverPathTextBox().Text();
        m_bypass = BypassCheckBox().IsChecked().GetBoolean();

        std::wstring wideProcessName = std::wstring_view(m_driverPath.c_str()).data();

        if (wideProcessName.find(L"\"") != std::wstring::npos) {
            wideProcessName.erase(wideProcessName.end());
            wideProcessName.erase(wideProcessName.begin());
        }

        m_driverPath = wideProcessName;

        deferral.Complete();
    }

    slg::coroutine LoadDriverDialog::ExploreButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        HWND hWnd = g_mainWindowInstance->GetWindowHandle();

        FileOpenPicker picker = FileOpenPicker(winrt::Microsoft::UI::GetWindowIdFromWindow(hWnd));

        picker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
        picker.FileTypeFilter().Append(L".sys");

        auto result = co_await picker.PickSingleFileAsync();

        if (!result) co_return;

        auto file = co_await StorageFile::GetFileFromPathAsync(result.Path());

        if (file != nullptr && file.IsAvailable()) {
            if (file.FileType() == L".sys") {
                DriverPathTextBox().Text(file.Path());
            }
            else {
                ErrorText().Text(t(L"LoadDriver.Msg.NotSYS"));
            }
        }
        else {
            ErrorText().Text(t(L"Msg.FileNotFound"));
        }
    }
}



