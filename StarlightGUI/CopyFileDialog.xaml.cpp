#include "pch.h"
#include "CopyFileDialog.xaml.h"
#if __has_include("CopyFileDialog.g.cpp")
#include "CopyFileDialog.g.cpp"
#endif
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Storage;
using namespace Microsoft::Windows::Storage::Pickers;

namespace winrt::StarlightGUI::implementation
{
    CopyFileDialog::CopyFileDialog() {
        InitializeComponent();

        this->Title(tbox(L"CopyFile_Dialog.Title"));
    }

    void CopyFileDialog::OnPrimaryButtonClick(ContentDialog const& sender,
        ContentDialogButtonClickEventArgs const& args)
    {
        auto deferral = args.GetDeferral();

        m_copyPath = CopyPathTextBox().Text();

        std::wstring wideFolderName = std::wstring_view(m_copyPath.c_str()).data();

        if (wideFolderName.find(L"\"") != std::wstring::npos) {
            wideFolderName.erase(wideFolderName.end());
            wideFolderName.erase(wideFolderName.begin());
        }

        m_copyPath = wideFolderName;

        deferral.Complete();
    }

    slg::coroutine CopyFileDialog::ExploreButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        HWND hWnd = g_mainWindowInstance->GetWindowHandle();

        FolderPicker picker = FolderPicker(winrt::Microsoft::UI::GetWindowIdFromWindow(hWnd));

        picker.SuggestedStartLocation(PickerLocationId::ComputerFolder);

        auto result = co_await picker.PickSingleFolderAsync();

        if (!result) co_return;

        CopyPathTextBox().Text(result.Path());
    }
}



