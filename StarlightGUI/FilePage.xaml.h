#pragma once

#include "FilePage.g.h"

namespace winrt::StarlightGUI::implementation
{
    struct FilePage : FilePageT<FilePage>
    {
        FilePage();

        slg::coroutine RefreshButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        slg::coroutine NextDriveButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void FileListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void FileListView_DoubleTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e);
        void FileListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        void ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e);
        slg::coroutine ApplySort(bool& isAscending, const std::string& column);

        void PathBox_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        bool ApplyFilter(const winrt::StarlightGUI::FileInfo& file, hstring& query);

        winrt::Windows::Foundation::IAsyncAction LoadFileList();
        winrt::Windows::Foundation::IAsyncAction WaitAndReloadAsync(int interval);
        winrt::Windows::Foundation::IAsyncAction GetFileInfoAsync(const winrt::StarlightGUI::FileInfo& file);
        winrt::Windows::Foundation::IAsyncAction GetFileIconAsync(const winrt::StarlightGUI::FileInfo& file);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::FileInfo> m_fileList{
            winrt::multi_threaded_observable_vector<winrt::StarlightGUI::FileInfo>()
        };

        slg::coroutine CopyFiles();

        void AddPreviousItem();
        slg::coroutine LoadMoreFiles();
        void CheckAndLoadMoreItems();
        void ResetState();
        bool FindScrollViewer(DependencyObject parent);

        winrt::Microsoft::UI::Xaml::DispatcherTimer m_scrollCheckTimer{ nullptr };
        winrt::Microsoft::UI::Xaml::DispatcherTimer reloadTimer;

        winrt::Microsoft::UI::Xaml::Controls::ScrollViewer m_listScrollViewer{ nullptr };
        std::vector<winrt::StarlightGUI::FileInfo> m_allFiles;
        size_t m_loadedCount = 0;
        bool m_isLoadingFiles = false;
        bool m_isLoadingMore = false;
        bool m_hasMoreFiles = true;

        inline static bool m_isNameAscending = true;
        inline static bool m_isModifyTimeAscending = true;
        inline static bool m_isSizeAscending = true;
        inline static bool currentSortingOption;
        inline static std::string currentSortingType;
    };

    extern winrt::hstring currentDirectory;
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct FilePage : FilePageT<FilePage, implementation::FilePage>
    {
    };
}
