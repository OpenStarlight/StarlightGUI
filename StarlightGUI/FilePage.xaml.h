#pragma once

#include "FilePage.g.h"
#include <unordered_map>

namespace winrt::StarlightGUI::implementation
{
    struct FilePage : FilePageT<FilePage>
    {
        FilePage();
        void SetupLocalization();

        slg::coroutine RefreshButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void BackButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ForwardButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void UpButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void FileTabView_AddTabButtonClick(winrt::Microsoft::UI::Xaml::Controls::TabView const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void FileTabView_TabCloseRequested(winrt::Microsoft::UI::Xaml::Controls::TabView const& sender, winrt::Microsoft::UI::Xaml::Controls::TabViewTabCloseRequestedEventArgs const& args);
        void FileTabView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void PathBreadcrumbBar_ItemClicked(winrt::Microsoft::UI::Xaml::Controls::BreadcrumbBar const& sender, winrt::Microsoft::UI::Xaml::Controls::BreadcrumbBarItemClickedEventArgs const& args);
        void PathSegmentButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void HandleExternalDropFiles(std::vector<std::wstring> const& paths);

        void FileListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void FileListView_DoubleTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e);
        void FileListView_DragOver(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::DragEventArgs const& e);
        void FileListView_Drop(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::DragEventArgs const& e);
        void FileListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        void ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e);
        slg::coroutine ApplySort(bool& isAscending, const std::string& column);
        void SortFileList(bool isAscending, const std::string& column, bool updateHeader);

        void SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& e);
        void SearchBox_SuggestionChosen(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxSuggestionChosenEventArgs const& e);
        void SearchBox_QuerySubmitted(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& e);
        bool ApplyFilter(const winrt::StarlightGUI::FileInfo& file, hstring& query);

        winrt::Windows::Foundation::IAsyncAction LoadFileList();
        winrt::Windows::Foundation::IAsyncAction WaitAndReloadAsync(int interval);
        winrt::Windows::Foundation::IAsyncAction GetFileIconAsync(winrt::StarlightGUI::FileInfo file);
        winrt::Windows::Foundation::IAsyncAction CopyDroppedPathsAsync(std::vector<std::wstring> paths);
        void PopulateFileMetaBatch(std::wstring const& directoryPath);
        static std::wstring GetIconCacheKey(winrt::StarlightGUI::FileInfo file);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::FileInfo> m_fileList{
            winrt::multi_threaded_observable_vector<winrt::StarlightGUI::FileInfo>()
        };

        slg::coroutine CopyFiles();

        winrt::Windows::Foundation::IAsyncAction LoadMetaForCurrentList(std::wstring path, uint64_t loadToken);
        void UpdateRealizedItemIcon(winrt::StarlightGUI::FileInfo const& file, winrt::Microsoft::UI::Xaml::Media::ImageSource const& icon);
        void ResetState();
        void CreateNewTab(std::wstring path, bool shouldSelect);
        void SelectTab(std::wstring const& tabId, bool reload);
        void NavigateTo(std::wstring path, bool pushHistory);
        void SyncCurrentTabUI();
        void UpdateCurrentTabHeader();
        void UpdateNavigationButtons();
        void UpdateBreadcrumbItems();
        std::wstring GetCurrentTabId();

        struct FileTabState
        {
            std::wstring title;
            std::vector<std::wstring> history;
            int historyIndex = -1;
            std::wstring searchText;
        };

        uint64_t m_reloadRequestVersion = 0;
        std::vector<winrt::StarlightGUI::FileInfo> m_allFiles;
        bool m_isLoadingFiles = false;
        bool m_isPostLoading = false;
        uint64_t m_currentLoadToken = 0;
        std::unordered_map<std::wstring, FileTabState> m_tabStates;
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> m_breadcrumbItems{
            winrt::single_threaded_observable_vector<winrt::Windows::Foundation::IInspectable>()
        };
        uint64_t m_nextTabId = 1;
        bool m_isSyncingTab = false;

        inline static bool m_isNameAscending = true;
        inline static bool m_isModifyTimeAscending = true;
        inline static bool m_isSizeAscending = true;
        inline static bool currentSortingOption;
        inline static std::string currentSortingType;
    };

    extern winrt::hstring currentDirectory;
    extern FilePage* g_filePageInstance;
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct FilePage : FilePageT<FilePage, implementation::FilePage>
    {
    };
}
