#include "pch.h"
#include "InjectDLLDialog.xaml.h"
#if __has_include("InjectDLLDialog.g.cpp")
#include "InjectDLLDialog.g.cpp"
#endif

#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Storage;
using namespace Microsoft::Windows::Storage::Pickers;

namespace winrt::StarlightGUI::implementation
{
    InjectDLLDialog::InjectDLLDialog()
    {
        InitializeComponent();

        this->Title(tbox(L"InjectDLL.Title"));
        this->PrimaryButtonText(t(L"InjectDLL.ButtonPrimary"));
        this->SecondaryButtonText(t(L"InjectDLL.ButtonSecondary"));
        InjectDLLDescriptionText().Text(t(L"InjectDLL.Desc"));
        DLLPathTextBox().PlaceholderText(t(L"InjectDLL.Placeholder"));
        ExploreButton().Content(tbox(L"InjectDLL.Browse"));
    }

    void InjectDLLDialog::OnPrimaryButtonClick(ContentDialog const& sender,
        ContentDialogButtonClickEventArgs const& args)
    {
        auto deferral = args.GetDeferral();

        m_DLLPath = DLLPathTextBox().Text();

        std::wstring wideProcessName = std::wstring_view(m_DLLPath.c_str()).data();

        if (wideProcessName.find(L"\"") != std::wstring::npos) {
            wideProcessName.erase(wideProcessName.end());
            wideProcessName.erase(wideProcessName.begin());
        }

        m_DLLPath = wideProcessName;

        deferral.Complete();
    }

    slg::coroutine InjectDLLDialog::ExploreButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        HWND hWnd = g_mainWindowInstance->GetWindowHandle();

        FileOpenPicker picker = FileOpenPicker(winrt::Microsoft::UI::GetWindowIdFromWindow(hWnd));

        picker.SuggestedStartLocation(PickerLocationId::ComputerFolder);
        picker.FileTypeFilter().Append(L".dll");

        auto result = co_await picker.PickSingleFileAsync();

        if (!result) co_return;

        auto file = co_await StorageFile::GetFileFromPathAsync(result.Path());

        if (file != nullptr && file.IsAvailable()) {
            if (file.FileType() == L".dll") {
                DLLPathTextBox().Text(file.Path());
            }
            else {
                ErrorText().Text(t(L"InjectDLL.NotDLL"));
            }
        }
        else {
            ErrorText().Text(t(L"InjectDLL.FileNotExist"));
        }
    }
}


