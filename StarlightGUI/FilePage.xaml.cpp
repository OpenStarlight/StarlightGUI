#include "pch.h"
#include "FilePage.xaml.h"
#if __has_include("FilePage.g.cpp")
#include "FilePage.g.cpp"
#endif

#include <chrono>
#include <shellapi.h>
#include <CopyFileDialog.xaml.h>
#include <array>
#include <unordered_set>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

using namespace winrt;
using namespace WinUI3Package;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation
{
    // 文件页虚拟根目录，代表此电脑
    static const std::wstring kFileHomePage = L"::drives::";

    FilePage* g_filePageInstance = nullptr;

	hstring currentDirectory = L"C:\\";
    static hstring safeAcceptedName = L"";
    static std::unordered_map<std::wstring, winrt::Microsoft::UI::Xaml::Media::ImageSource> iconCache;
    static std::unordered_set<std::wstring> iconLoadingKeys;
    static std::unordered_map<std::wstring, std::vector<winrt::StarlightGUI::FileInfo>> iconPendingFiles;

    // 这些拓展名只缓存一次，因为图标通常不变
    static bool IsFastIconCacheExtension(std::wstring ext)
    {
        ext = ToLowerCase(ext);
        static const std::unordered_set<std::wstring> fastExts = {
            L".txt", L".log", L".ini", L".inf", L".cfg", L".conf",
            L".json", L".xml", L".yaml", L".yml", L".csv",
            L".dll", L".sys", L".mui", L"bin",
            L".dat", L".bak", L".tmp",
            L".reg", L".md", L".bat", L".cmd", L".ps1", L".vbs", L".js", L".jse", L".wsf",
			L".c", L".cpp", L".java", L".kt", L".cs", L".h", L".hpp", L".py", L".rb", L".go", L".rs"
        };
        return fastExts.find(ext) != fastExts.end();
    }

    static std::wstring BuildTabTitle(std::wstring const& path)
    {
        if (path.empty() || path == kFileHomePage) return std::wstring(GetLocalizedString(L"File_ThisPC"));
        fs::path p(path);
        auto name = p.filename().wstring();
        if (!name.empty()) return name;

        std::wstring cleanPath = FixBackSplash(hstring(path));
        if (cleanPath.size() >= 2 && cleanPath[1] == L':') return cleanPath.substr(0, 2);
        return cleanPath;
    }

    FilePage::FilePage() {
        InitializeComponent();

        SearchBox().PlaceholderText(GetLocalizedString(L"File_SearchBox.PlaceholderText"));
        NameHeaderButton().Content(tbox(L"File_ColFile.Content"));
        ModifyTimeHeaderButton().Content(tbox(L"File_ColModifyTime.Content"));
        SizeHeaderButton().Content(tbox(L"File_ColSize.Content"));

        g_filePageInstance = this;

        FileListView().ItemsSource(m_fileList);

        this->Loaded([this](auto&&, auto&&) {
            g_filePageInstance = this;
            if (FileTabView().TabItems().Size() == 0) {
                CreateNewTab(kFileHomePage, true);
            }
            });

        this->Unloaded([this](auto&&, auto&&) {
            ++m_reloadRequestVersion;
            if (g_filePageInstance == this) g_filePageInstance = nullptr;
            });

        LOG_INFO(L"FilePage", L"FilePage initialized.");
    }

    void FilePage::HandleExternalDropFiles(std::vector<std::wstring> const& paths)
    {
        if (paths.empty()) return;

        auto lifetime = get_strong();
        CopyDroppedPathsAsync(paths);
    }

    std::wstring FilePage::GetCurrentTabId()
    {
        auto selectedObject = FileTabView().SelectedItem();
        if (!selectedObject) return L"";

        auto tab = selectedObject.try_as<Microsoft::UI::Xaml::Controls::TabViewItem>();
        if (!tab || !tab.Tag()) return L"";
        return unbox_value<hstring>(tab.Tag()).c_str();
    }

    void FilePage::CreateNewTab(std::wstring path, bool shouldSelect)
    {
        // 新标签页默认进入虚拟根目录，不继承上一标签路径
        std::wstring normalizedPath = path == kFileHomePage ? kFileHomePage : FixBackSplash(hstring(path));
        if (normalizedPath.empty()) normalizedPath = kFileHomePage;

        std::wstring tabId = L"tab_" + std::to_wstring(m_nextTabId++);

        FileTabState tabState{};
        tabState.history.push_back(normalizedPath);
        tabState.historyIndex = 0;
        tabState.searchText = L"";
        tabState.title = BuildTabTitle(normalizedPath);
        m_tabStates.insert_or_assign(tabId, std::move(tabState));

        Microsoft::UI::Xaml::Controls::TabViewItem tabItem;
        tabItem.Tag(box_value(hstring(tabId)));
        tabItem.Header(box_value(hstring(m_tabStates[tabId].title)));
        tabItem.IsClosable(true);

        FileTabView().TabItems().Append(tabItem);

        if (shouldSelect) {
            m_isSyncingTab = true;
            FileTabView().SelectedItem(tabItem);
            m_isSyncingTab = false;
            SelectTab(tabId, true);
        }
    }

    void FilePage::SelectTab(std::wstring const& tabId, bool reload)
    {
        auto stateIt = m_tabStates.find(tabId);
        if (stateIt == m_tabStates.end()) return;

        auto& state = stateIt->second;
        if (state.history.empty() || state.historyIndex < 0 || state.historyIndex >= static_cast<int>(state.history.size())) return;

        currentDirectory = hstring(state.history[state.historyIndex]);
        SyncCurrentTabUI();
        if (reload) LoadFileList();
    }

    void FilePage::NavigateTo(std::wstring path, bool pushHistory)
    {
        if (m_isLoadingFiles || m_isPostLoading) return;

        std::wstring tabId = GetCurrentTabId();
        if (tabId.empty()) return;

        auto stateIt = m_tabStates.find(tabId);
        if (stateIt == m_tabStates.end()) return;

        std::wstring normalizedPath = path == kFileHomePage ? kFileHomePage : FixBackSplash(hstring(path));
        if (normalizedPath.empty()) return;

        auto& state = stateIt->second;

        if (pushHistory) {
            if (state.historyIndex >= 0 && state.historyIndex < static_cast<int>(state.history.size()) - 1) {
                state.history.erase(state.history.begin() + state.historyIndex + 1, state.history.end());
            }
            if (state.history.empty() || state.history.back() != normalizedPath) {
                state.history.push_back(normalizedPath);
                state.historyIndex = static_cast<int>(state.history.size()) - 1;
            }
            else {
                state.historyIndex = static_cast<int>(state.history.size()) - 1;
            }
        }

        currentDirectory = hstring(normalizedPath);
        state.title = BuildTabTitle(normalizedPath);
        UpdateCurrentTabHeader();
        SyncCurrentTabUI();
        LoadFileList();
    }

	// 同步当前标签页的 UI 状态，不触发文件列表刷新
    void FilePage::SyncCurrentTabUI()
    {
        std::wstring tabId = GetCurrentTabId();
        if (tabId.empty()) return;

        auto stateIt = m_tabStates.find(tabId);
        if (stateIt == m_tabStates.end()) return;

        m_isSyncingTab = true;
        UpdateBreadcrumbItems();
        SearchBox().Text(hstring(stateIt->second.searchText));
        m_isSyncingTab = false;
        UpdateNavigationButtons();
    }

    // 构建标签页标题（图标和文本）
    void FilePage::UpdateCurrentTabHeader()
    {
        auto selectedObject = FileTabView().SelectedItem();
        if (!selectedObject) return;
        auto tab = selectedObject.try_as<Microsoft::UI::Xaml::Controls::TabViewItem>();
        if (!tab || !tab.Tag()) return;

        std::wstring tabId = unbox_value<hstring>(tab.Tag()).c_str();
        auto stateIt = m_tabStates.find(tabId);
        if (stateIt == m_tabStates.end()) return;
        if (stateIt->second.history.empty() || stateIt->second.historyIndex < 0 || stateIt->second.historyIndex >= static_cast<int>(stateIt->second.history.size())) return;

        auto headerPanel = StackPanel();
        headerPanel.Orientation(Orientation::Horizontal);
        headerPanel.Spacing(8);

        auto headerIcon = Image();
        headerIcon.Width(16);
        headerIcon.Height(16);
        headerIcon.VerticalAlignment(VerticalAlignment::Center);
        auto const& currentPath = stateIt->second.history[stateIt->second.historyIndex];
        headerIcon.Source(slg::GetShellIconImage(currentPath == kFileHomePage ? L"C:\\" : currentPath, true, 16, false));

        auto headerText = TextBlock();
        headerText.Text(hstring(stateIt->second.title));
        headerText.VerticalAlignment(VerticalAlignment::Center);

        headerPanel.Children().Append(headerIcon);
        headerPanel.Children().Append(headerText);

        tab.Header(headerPanel);
    }

    void FilePage::UpdateNavigationButtons()
    {
        std::wstring tabId = GetCurrentTabId();
        if (tabId.empty()) return;

        auto stateIt = m_tabStates.find(tabId);
        if (stateIt == m_tabStates.end()) return;

        auto& state = stateIt->second;
        bool canGoBack = state.historyIndex > 0;
        bool canGoForward = !state.history.empty() && state.historyIndex < static_cast<int>(state.history.size()) - 1;
        bool canGoUp = currentDirectory != kFileHomePage && currentDirectory.size() > 3;

        BackButton().IsEnabled(canGoBack);
        ForwardButton().IsEnabled(canGoForward);
        UpButton().IsEnabled(canGoUp);
    }

    // 构建路径 UI 样式
    void FilePage::UpdateBreadcrumbItems()
    {
        m_breadcrumbItems.Clear();
        PathSegmentPanel().Children().Clear();

        std::wstring path = FixBackSplash(currentDirectory.c_str());
        if (path.empty()) return;

        if (path == kFileHomePage) {
            m_breadcrumbItems.Append(tbox(L"File_ThisPC"));
        }
        else if (path.size() >= 2 && path[1] == L':') {
            m_breadcrumbItems.Append(tbox(L"File_ThisPC"));
            std::wstring drive = path.substr(0, 2);
            m_breadcrumbItems.Append(box_value(hstring(drive)));

            if (path.size() > 3) {
                std::wstring remaining = path.substr(3);
                std::wstringstream ss(remaining);
                std::wstring part;
                while (std::getline(ss, part, L'\\')) {
                    if (!part.empty()) m_breadcrumbItems.Append(box_value(hstring(part)));
                }
            }
        }
        else {
            m_breadcrumbItems.Append(box_value(hstring(path)));
        }

        auto children = PathSegmentPanel().Children();
        for (uint32_t i = 0; i < m_breadcrumbItems.Size(); ++i) {
            hstring text = unbox_value<hstring>(m_breadcrumbItems.GetAt(i));

            Microsoft::UI::Xaml::Controls::Button segmentButton{};
            segmentButton.Style(Resources().Lookup(box_value(L"PathSegmentButtonStyle")).as<Microsoft::UI::Xaml::Style>());
            segmentButton.Content(box_value(text));
            segmentButton.Tag(box_value(i));
            segmentButton.Click({ this, &FilePage::PathSegmentButton_Click });
            children.Append(segmentButton);

            if (i + 1 < m_breadcrumbItems.Size()) {
                TextBlock separator{};
                separator.Text(L">");
                separator.Opacity(0.6);
                separator.Margin(Thickness{ 1, 0, 1, 0 });
                separator.VerticalAlignment(VerticalAlignment::Center);
                children.Append(separator);
            }
        }

        auto scrollViewer = PathSegmentScrollViewer();
        scrollViewer.UpdateLayout();
        auto horizontalOffset = box_value(scrollViewer.ScrollableWidth()).as<Windows::Foundation::IReference<double>>();
        scrollViewer.ChangeView(horizontalOffset, nullptr, nullptr, true);
    }

    void FilePage::PathBreadcrumbBar_ItemClicked(Microsoft::UI::Xaml::Controls::BreadcrumbBar const& sender, Microsoft::UI::Xaml::Controls::BreadcrumbBarItemClickedEventArgs const& args)
    {
        uint32_t index = args.Index();
        if (m_breadcrumbItems.Size() == 0 || index >= m_breadcrumbItems.Size()) return;

        std::wstring newPath;

        for (uint32_t i = 0; i <= index; ++i) {
            std::wstring part = unbox_value<hstring>(m_breadcrumbItems.GetAt(i)).c_str();
            if (i == 0) {
                if (part == GetLocalizedString(L"File_ThisPC")) {
                    newPath = kFileHomePage;
                    continue;
                }
                if (part.size() >= 2 && part[1] == L':') newPath = part + L"\\";
                else newPath = part;
            }
            else {
                if (newPath == kFileHomePage) {
                    if (part.size() >= 2 && part[1] == L':') newPath = part + L"\\";
                    continue;
                }
                if (!newPath.empty() && newPath.back() != L'\\') newPath += L"\\";
                newPath += part;
            }
        }

        NavigateTo(newPath, true);
    }

    void FilePage::PathSegmentButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        auto button = sender.try_as<Microsoft::UI::Xaml::Controls::Button>();
        if (!button || !button.Tag()) return;
        uint32_t index = unbox_value<uint32_t>(button.Tag());
        if (m_breadcrumbItems.Size() == 0 || index >= m_breadcrumbItems.Size()) return;

        std::wstring newPath;

        for (uint32_t i = 0; i <= index; ++i) {
            std::wstring part = unbox_value<hstring>(m_breadcrumbItems.GetAt(i)).c_str();
            if (i == 0) {
                if (part == GetLocalizedString(L"File_ThisPC")) {
                    newPath = kFileHomePage;
                    continue;
                }
                if (part.size() >= 2 && part[1] == L':') newPath = part + L"\\";
                else newPath = part;
            }
            else {
                if (newPath == kFileHomePage) {
                    if (part.size() >= 2 && part[1] == L':') newPath = part + L"\\";
                    continue;
                }
                if (!newPath.empty() && newPath.back() != L'\\') newPath += L"\\";
                newPath += part;
            }
        }

        NavigateTo(newPath, true);
    }

    void FilePage::FileTabView_AddTabButtonClick(Microsoft::UI::Xaml::Controls::TabView const& sender, winrt::Windows::Foundation::IInspectable const& args)
    {
        CreateNewTab(kFileHomePage, true);
    }

    void FilePage::FileTabView_TabCloseRequested(Microsoft::UI::Xaml::Controls::TabView const& sender, Microsoft::UI::Xaml::Controls::TabViewTabCloseRequestedEventArgs const& args)
    {
        auto tab = args.Tab();
        if (!tab || !tab.Tag()) return;

        std::wstring tabId = unbox_value<hstring>(tab.Tag()).c_str();
        uint32_t removedIndex = 0;
        if (sender.TabItems().IndexOf(tab, removedIndex)) {
            sender.TabItems().RemoveAt(removedIndex);
        }
        m_tabStates.erase(tabId);

        if (sender.TabItems().Size() == 0) {
            CreateNewTab(kFileHomePage, true);
            return;
        }

        auto selected = sender.SelectedItem().try_as<Microsoft::UI::Xaml::Controls::TabViewItem>();
        if (!selected) {
            auto fallbackIndex = removedIndex;
            if (fallbackIndex >= sender.TabItems().Size()) fallbackIndex = sender.TabItems().Size() - 1;
            sender.SelectedItem(sender.TabItems().GetAt(fallbackIndex));
        }

        auto selectedTab = sender.SelectedItem().try_as<Microsoft::UI::Xaml::Controls::TabViewItem>();
        if (!selectedTab || !selectedTab.Tag()) return;
        SelectTab(unbox_value<hstring>(selectedTab.Tag()).c_str(), true);
    }

    void FilePage::FileTabView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e)
    {
        if (m_isSyncingTab) return;

        auto tabId = GetCurrentTabId();
        if (tabId.empty()) return;
        SelectTab(tabId, true);
    }

    void FilePage::FileListView_RightTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
    {
        auto listView = FileListView();

        slg::SelectItemOnRightTapped(listView, e, true);

        if (!listView.SelectedItems() || listView.SelectedItems().Size() == 0 ||
            // 只选择"上个文件夹"时不显示，多选的话在下面跳过处理，认为是误选
            (listView.SelectedItems().Size() == 1 && listView.SelectedItems().GetAt(0).as<winrt::StarlightGUI::FileInfo>().Flag() == 999))
            return;

        auto list = listView.SelectedItems();

        std::vector<winrt::StarlightGUI::FileInfo> selectedFiles;

        for (const auto& file : list) {
            auto item = file.as<winrt::StarlightGUI::FileInfo>();
            // 跳过"上个文件夹"选项
            if (item.Flag() == 999) continue;
            if ((item.Name() == L"Windows" || item.Name() == L"Boot" || item.Name() == L"System32" || item.Name() == L"SysWOW64" || item.Name() == L"Microsoft") &&
                (safeAcceptedName != L"Windows" && safeAcceptedName != L"Boot" && safeAcceptedName != L"System32" && safeAcceptedName != L"SysWOW64" && safeAcceptedName != L"Microsoft")) {
                safeAcceptedName = item.Name();
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"File_ImportantFileWarning").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
                return;
            }
            selectedFiles.push_back(item);
        }

        safeAcceptedName = L"";

        auto flyoutStyles = slg::GetStyles();

        MenuFlyout menuFlyout;

        /*
        * 注意，由于这里是磁盘 IO，我们不要使用异步，否则刷新时可能会出问题
        */
        // 选项1.1
        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue8e5", GetLocalizedString(L"FileMenu_Open").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (item.Directory()) {
                    if (currentDirectory == hstring(kFileHomePage)) NavigateTo(item.Path().c_str(), true);
                    else NavigateTo(FixBackSplash(currentDirectory.c_str()) + L"\\" + item.Name().c_str(), true);
                }
                else ShellExecuteW(nullptr, L"open", item.Path().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            });

        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuItem(flyoutStyles, L"\ue74d", GetLocalizedString(L"FileMenu_Delete").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (KernelInstance::DeleteFileAuto(item.Path().c_str())) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"File_DeleteSuccess").c_str() + item.Name() + L" (" + item.Path() + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"File_DeleteFailed").c_str() + item.Name() + L" (" + item.Path() + L")" + GetLocalizedString(L"Msg_ErrorCode").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            });

        // 选项2.2
        auto item2_2 = slg::CreateMenuItem(flyoutStyles, L"\ue733", GetLocalizedString(L"FileMenu_DeleteKernel").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (KernelInstance::_DeleteFileAuto(item.Path().c_str())) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"File_DeleteSuccess").c_str() + item.Name() + L" (" + item.Path() + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"File_DeleteFailed").c_str() + item.Name() + L" (" + item.Path() + L")" + GetLocalizedString(L"Msg_ErrorCode").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            });

        // 选项2.3
        auto item2_3 = slg::CreateMenuItem(flyoutStyles, L"\uf5ab", GetLocalizedString(L"FileMenu_DeleteMurder").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (KernelInstance::MurderFileAuto(item.Path().c_str())) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"File_DeleteSuccess").c_str() + item.Name() + L" (" + item.Path() + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"File_DeleteFailed").c_str() + item.Name() + L" (" + item.Path() + L")" + GetLocalizedString(L"Msg_ErrorCode").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            });

        // 选项2.4
        auto item2_4 = slg::CreateMenuItem(flyoutStyles, L"\ue72e", GetLocalizedString(L"FileMenu_Lock").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (KernelInstance::LockFile(item.Path().c_str())) {
                    slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"File_LockSuccess").c_str() + item.Name() + L" (" + item.Path() + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"File_LockFailed").c_str() + item.Name() + L" (" + item.Path() + L")" + GetLocalizedString(L"Msg_ErrorCode").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            });

        // 选项2.5
        auto item2_5 = slg::CreateMenuItem(flyoutStyles, L"\ue8c8", GetLocalizedString(L"FileMenu_Copy").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            CopyFiles();
            });

        MenuFlyoutSeparator separator2;

        // 选项3.1
        auto item3_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", GetLocalizedString(L"FileMenu_CopyInfo").c_str());
        auto item3_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue8ac", GetLocalizedString(L"FileMenu_Name").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(selectedFiles[0].Name().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"Msg_CopyFailed").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub1);
        auto item3_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", GetLocalizedString(L"FileMenu_Path").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(selectedFiles[0].Path().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"Msg_CopyFailed").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub2);
        auto item3_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\uec92", GetLocalizedString(L"FileMenu_ModDate").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(selectedFiles[0].ModifyTime().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"Msg_CopiedToClipboard").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"Msg_CopyFailed").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub3);

        // 选项3.2
        auto item3_2 = slg::CreateMenuItem(flyoutStyles, L"\uec50", GetLocalizedString(L"FileMenu_OpenInExplorer").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::OpenFolderAndSelectFile(selectedFiles[0].Path().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"File_OpenedFolder").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"File_OpenFolderFailed").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });


        // 选项3.3
        auto item3_3 = slg::CreateMenuItem(flyoutStyles, L"\ue8ec", GetLocalizedString(L"FileMenu_Properties").c_str(), [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::OpenFileProperties(selectedFiles[0].Path().c_str())) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"File_OpenedProperties").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"File_OpenPropertiesFailed").c_str() + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

        // 当选中多个内容并且其中一个是文件夹时禁用锁定部分只能操作文件的按钮
        // 当选中多个内容并且其中一个是文件夹时禁用打开按钮
        for (const auto& item : selectedFiles) {
            if (item.Directory()) {
                item2_4.IsEnabled(false);
                item2_5.IsEnabled(false);
                if (selectedFiles.size() > 1) item1_1.IsEnabled(false);
                break;
            }
        }

        menuFlyout.Items().Append(item1_1);
        menuFlyout.Items().Append(separator1);
        menuFlyout.Items().Append(item2_1);
        menuFlyout.Items().Append(item2_2);
        menuFlyout.Items().Append(item2_3);
        menuFlyout.Items().Append(item2_4);
        menuFlyout.Items().Append(item2_5);
        menuFlyout.Items().Append(separator2);
        menuFlyout.Items().Append(item3_1);
        menuFlyout.Items().Append(item3_2);
        menuFlyout.Items().Append(item3_3);

        slg::ShowAt(menuFlyout, listView, e);
    }

	void FilePage::FileListView_DoubleTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e)
	{
        if (!FileListView().SelectedItem()) return;

        auto item = FileListView().SelectedItem().as<winrt::StarlightGUI::FileInfo>();

        if (item.Flag() == 999) {
            if (currentDirectory != hstring(kFileHomePage) && currentDirectory.size() <= 3) {
                NavigateTo(kFileHomePage, true);
            }
            else {
                NavigateTo(GetParentDirectory(currentDirectory.c_str()), true);
            }
        } else if (item.Directory()) {
            if (currentDirectory == hstring(kFileHomePage)) NavigateTo(item.Path().c_str(), true);
            else NavigateTo(FixBackSplash(currentDirectory.c_str()) + L"\\" + item.Name().c_str(), true);
        }
        else {
            ShellExecuteW(nullptr, L"open", item.Path().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
	}

    void FilePage::FileListView_DragOver(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::DragEventArgs const& e)
    {
        using namespace winrt::Windows::ApplicationModel::DataTransfer;

        if (e.DataView().Contains(StandardDataFormats::StorageItems())) {
            e.AcceptedOperation(DataPackageOperation::Copy);
            auto dragUI = e.DragUIOverride();
            dragUI.IsCaptionVisible(true);
            dragUI.Caption(GetLocalizedString(L"FileMenu_CopyToCurrentDir"));
        }
        else {
            e.AcceptedOperation(DataPackageOperation::None);
        }
    }

    void FilePage::FileListView_Drop(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::DragEventArgs const& e)
    {
        using namespace winrt::Windows::ApplicationModel::DataTransfer;
        using namespace winrt::Windows::Storage;

        e.AcceptedOperation(DataPackageOperation::Copy);

        auto deferral = e.GetDeferral();
        auto dataView = e.DataView();
        auto lifetime = get_strong();

        [this, lifetime, deferral, dataView]() -> winrt::Windows::Foundation::IAsyncAction {
            if (m_isLoadingFiles || m_isPostLoading) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Info").c_str(), GetLocalizedString(L"File_StillLoading").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
                deferral.Complete();
                co_return;
            }

            try {
                if (!dataView.Contains(StandardDataFormats::StorageItems())) {
                    deferral.Complete();
                    co_return;
                }

                auto droppedItems = co_await dataView.GetStorageItemsAsync();
                if (!droppedItems || droppedItems.Size() == 0) {
                    deferral.Complete();
                    co_return;
                }

                std::vector<std::wstring> droppedPaths;
                droppedPaths.reserve(droppedItems.Size());
                for (auto const& item : droppedItems) {
                    auto storageItem = item.try_as<IStorageItem>();
                    if (!storageItem) {
                        continue;
                    }
                    droppedPaths.push_back(storageItem.Path().c_str());
                }

                co_await CopyDroppedPathsAsync(std::move(droppedPaths));
            }
            catch (...) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), GetLocalizedString(L"File_DragCopyException").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            }

            deferral.Complete();
            co_return;
            }();
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::CopyDroppedPathsAsync(std::vector<std::wstring> paths)
    {
        if (paths.empty()) co_return;
        if (currentDirectory == hstring(kFileHomePage)) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Info").c_str(), GetLocalizedString(L"File_EnterDriveFirst").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
            co_return;
        }
        if (m_isLoadingFiles || m_isPostLoading) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Warning").c_str(), GetLocalizedString(L"File_StillLoadingExcl").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
            co_return;
        }

        int successCount = 0, failedCount = 0;
        std::wstring targetDirectory = FixBackSplash(currentDirectory.c_str());

        for (auto const& rawPath : paths) {
            std::wstring sourcePath = FixBackSplash(rawPath.c_str());
            if (sourcePath.empty()) {
                failedCount++;
                continue;
            }

            auto pos = sourcePath.find_last_of(L'\\');
            if (pos == std::wstring::npos) {
                failedCount++;
                continue;
            }

            std::wstring itemName = sourcePath.substr(pos + 1);
            std::wstring targetPath = targetDirectory + L"\\" + itemName;

            bool copied = false;

            try {
                if (fs::is_directory(sourcePath)) {
                    fs::copy(sourcePath, targetPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                    copied = true;
                }
                else if (fs::is_regular_file(sourcePath)) {
                    copied = fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing);
                }
            }
            catch (...) {
                copied = false;
            }

            if (copied) {
                LOG_INFO(__WFUNCTION__, L"Copied by drag-drop: %s -> %s", sourcePath.c_str(), targetPath.c_str());
                successCount++;
            }
            else {
                LOG_ERROR(__WFUNCTION__, L"Failed to copy by drag-drop: %s -> %s", sourcePath.c_str(), targetPath.c_str());
                failedCount++;
            }
        }

        if (successCount > 0) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"File_CopySuccess").c_str() + to_hstring(successCount) + GetLocalizedString(L"File_ItemsSuffix").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
            co_await LoadFileList();
        }
        if (failedCount > 0) {
            slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), hstring(L"") + to_hstring(failedCount) + GetLocalizedString(L"File_CopyPartialFail").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
        }

        co_return;
    }

    void FilePage::FileListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {

        if (auto file = args.Item().try_as<winrt::StarlightGUI::FileInfo>()) {
            if (file.Icon()) {
                UpdateRealizedItemIcon(file, file.Icon());
            }
            else {
                GetFileIconAsync(file);
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::LoadFileList()
    {
        if (m_isLoadingFiles || m_isPostLoading) co_return;
        m_isLoadingFiles = true;

        LOG_INFO(__WFUNCTION__, L"Loading file list...");
        ResetState();
        LoadingRing().IsActive(true);

        auto start = std::chrono::steady_clock::now();

        auto lifetime = get_strong();

        std::wstring path = currentDirectory == hstring(kFileHomePage) ? kFileHomePage : FixBackSplash(currentDirectory);
        auto loadToken = ++m_currentLoadToken;
        iconLoadingKeys.clear();
        iconPendingFiles.clear();
        currentDirectory = path;
        std::wstring tabSearchText;
        auto tabId = GetCurrentTabId();
        if (!tabId.empty()) {
            auto stateIt = m_tabStates.find(tabId);
            if (stateIt != m_tabStates.end() && stateIt->second.historyIndex >= 0 && stateIt->second.historyIndex < static_cast<int>(stateIt->second.history.size())) {
                stateIt->second.history[stateIt->second.historyIndex] = path;
                stateIt->second.title = BuildTabTitle(path);
                tabSearchText = stateIt->second.searchText;
                UpdateCurrentTabHeader();
            }
        }
        m_isSyncingTab = true;
        UpdateBreadcrumbItems();
        m_isSyncingTab = false;
        UpdateNavigationButtons();
        LOG_INFO(__WFUNCTION__, L"Path = %s", path.c_str());

        co_await winrt::resume_background();

        m_allFiles.clear();

        // 首页展示驱动器，行为与系统文件管理器更一致
        if (path == kFileHomePage) {
            wchar_t driveBuffer[256]{};
            GetLogicalDriveStringsW(255, driveBuffer);
            for (wchar_t* drive = driveBuffer; *drive; drive += wcslen(drive) + 1) {
                auto driveInfo = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
                std::wstring drivePath = drive;
                driveInfo.Name(hstring(drivePath.substr(0, 2)));
                driveInfo.Path(hstring(drivePath));
                driveInfo.Directory(true);
                driveInfo.Size(L"");
                driveInfo.ModifyTime(L"");
                m_allFiles.push_back(driveInfo);
            }
        }
        else {
            KernelInstance::QueryFile(path, m_allFiles);
        }
        LOG_INFO(__WFUNCTION__, L"Enumerated files, %d entry(s).", m_allFiles.size());

        co_await wil::resume_foreground(DispatcherQueue());
        if (loadToken != m_currentLoadToken) {
            m_isLoadingFiles = false;
            co_return;
        }

        ApplySort(currentSortingOption, currentSortingType);
        std::partition(m_allFiles.begin(), m_allFiles.end(), [](auto const& file) { return file.Directory(); });

        auto newFileList = std::vector<winrt::StarlightGUI::FileInfo>();

        if (currentDirectory != hstring(kFileHomePage)) {
            auto previousPage = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
            previousPage.Name(currentDirectory.size() <= 3 ? GetLocalizedString(L"File_BackToPC").c_str() : GetLocalizedString(L"File_PreviousFolder").c_str());
            previousPage.Flag(999);
            newFileList.push_back(previousPage);
        }

        winrt::hstring query = hstring(tabSearchText);
        std::wstring lowerQuery;
        if (!query.empty()) lowerQuery = ToLowerCase(query.c_str());
        for (size_t i = 0; i < m_allFiles.size(); ++i) {
            bool shouldRemove = lowerQuery.empty() ? false : !ContainsIgnoreCaseLowerQuery(m_allFiles[i].Name().c_str(), lowerQuery);
            if (shouldRemove) continue;

            newFileList.push_back(m_allFiles[i]);
        }
        m_fileList.ReplaceAll(newFileList);

        if (path == kFileHomePage) {
            m_isPostLoading = false;
        }
        else {
            m_isPostLoading = true;
            LoadMetaForCurrentList(path, loadToken);
        }

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::wstringstream countText;
        countText << m_allFiles.size() << L" " << GetLocalizedString(L"File_FileCount") << L" (" << duration << " ms)";
        LoadingRing().IsActive(false);

        LOG_INFO(__WFUNCTION__, L"Loaded file list, %d entry(s) in total.", m_allFiles.size());

        m_isLoadingFiles = false;
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::LoadMetaForCurrentList(std::wstring path, uint64_t loadToken)
    {
        auto lifetime = get_strong();

        co_await winrt::resume_background();

        PopulateFileMetaBatch(path);

        co_await wil::resume_foreground(DispatcherQueue());
        if (!IsLoaded() || loadToken != m_currentLoadToken) {
            m_isPostLoading = false;
            co_return;
        }

        // 触发一次轻量刷新，确保更新后的属性及时反映到列表，并避免播放第二次刷新动画
        auto oldTransitions = FileListView().ItemContainerTransitions();
        FileListView().ItemContainerTransitions().Clear();
        FileListView().ItemsSource(nullptr);
        FileListView().ItemsSource(m_fileList);
        FileListView().ItemContainerTransitions(oldTransitions);
        m_isPostLoading = false;
    }

    void FilePage::PopulateFileMetaBatch(std::wstring const& directoryPath)
    {
        auto fillUnknownMeta = [](winrt::StarlightGUI::FileInfo const& file) {
            if (!file.Directory()) {
                if (file.SizeULong() == 0) file.Size(L"0 B");
                else file.Size(FormatMemorySize(file.SizeULong()));
            }
            else {
                file.SizeULong(0);
                file.Size(L"");
            }
            if (file.ModifyTime().empty()) file.ModifyTime(GetLocalizedString(L"File_Unknown"));
            };

        std::wstring searchPath = directoryPath + L"\\*";
        WIN32_FIND_DATAW data{};
        HANDLE hFind = FindFirstFileExW(searchPath.c_str(), FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (hFind == INVALID_HANDLE_VALUE) {
            for (auto const& file : m_allFiles) {
                file.Path(FixBackSplash(file.Path()));
                fillUnknownMeta(file);
            }
            return;
        }

        std::unordered_map<std::wstring, WIN32_FIND_DATAW> metaMap;
        metaMap.reserve(m_allFiles.size() * 2);

        auto normalize = [](std::wstring str) {
            return ToLowerCase(str);
            };

        do {
            if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
            metaMap[normalize(data.cFileName)] = data;
        } while (FindNextFileW(hFind, &data));

        FindClose(hFind);

        for (auto const& file : m_allFiles) {
            file.Path(FixBackSplash(file.Path()));
            std::wstring name = file.Name().c_str();
            auto it = metaMap.find(normalize(name));
            if (it == metaMap.end()) {
                fillUnknownMeta(file);
                continue;
            }

            auto const& d = it->second;
            const bool isDir = (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (!isDir) {
                ULONG64 size = ((ULONG64)d.nFileSizeHigh << 32) | d.nFileSizeLow;
                file.SizeULong(size);
                file.Size(FormatMemorySize(size));
            }
            else {
                file.SizeULong(0);
                file.Size(L"");
            }

            file.ModifyTimeULong(((ULONG64)d.ftLastWriteTime.dwHighDateTime << 32) | d.ftLastWriteTime.dwLowDateTime);
            SYSTEMTIME st{};
            if (FileTimeToSystemTime(&d.ftLastWriteTime, &st))
            {
                std::wstringstream ss;
                ss << std::setw(4) << std::setfill(L'0') << st.wYear << L"/"
                    << std::setw(2) << std::setfill(L'0') << st.wMonth << L"/"
                    << std::setw(2) << std::setfill(L'0') << st.wDay << L" "
                    << std::setw(2) << std::setfill(L'0') << st.wHour << L":"
                    << std::setw(2) << std::setfill(L'0') << st.wMinute << L":"
                    << std::setw(2) << std::setfill(L'0') << st.wSecond;
                file.ModifyTime(ss.str());
            }
            else
            {
                file.ModifyTime(GetLocalizedString(L"File_Unknown"));
            }
        }
    }

    std::wstring FilePage::GetIconCacheKey(winrt::StarlightGUI::FileInfo file)
    {
        if (!file) return L"__invalid__";
        if (file.Flag() == 999) return L"__dir__";
        if (file.Directory()) {
            std::wstring dirPath = FixBackSplash(file.Path());
            if (dirPath.size() >= 3 && dirPath[1] == L':' && dirPath[2] == L'\\') {
                dirPath = ToLowerCase(dirPath);
                return L"__path__" + dirPath;
            }
            return L"__dir__";
        }

        std::wstring name = file.Name().c_str();
        auto dot = name.find_last_of(L'.');
        if (dot != std::wstring::npos) {
            std::wstring ext = name.substr(dot);
            if (IsFastIconCacheExtension(ext)) {
                ext = ToLowerCase(ext);
                return L"__ext__" + ext;
            }
        }

        std::wstring path = file.Path().c_str();
        path = ToLowerCase(path);
        return L"__path__" + path;
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::GetFileIconAsync(winrt::StarlightGUI::FileInfo file)
    {
        if (!file) co_return;
        auto loadToken = m_currentLoadToken;
        co_await wil::resume_foreground(DispatcherQueue());
        if (!IsLoaded() || loadToken != m_currentLoadToken) co_return;

        std::wstring cacheKey = GetIconCacheKey(file);
        auto found = iconCache.find(cacheKey);
        if (found != iconCache.end()) {
            file.Icon(found->second);
            UpdateRealizedItemIcon(file, found->second);
            co_return;
        }

        iconPendingFiles[cacheKey].push_back(file);
        if (iconLoadingKeys.find(cacheKey) != iconLoadingKeys.end()) co_return;
        iconLoadingKeys.insert(cacheKey);
        auto cleanup = [&]() {
            iconLoadingKeys.erase(cacheKey);
            iconPendingFiles.erase(cacheKey);
            };

        try {

        std::wstring itemPath = file.Path().c_str();
        bool isDriveRoot = file.Directory() && itemPath.size() >= 3 && itemPath[1] == L':' && itemPath[2] == L'\\';
        bool useFastAttrQuery = (file.Directory() && !isDriveRoot) || cacheKey.find(L"__ext__") == 0;
        std::wstring lookup = file.Path().c_str();
        if (useFastAttrQuery && cacheKey.find(L"__ext__") == 0) {
            lookup = cacheKey.substr(7);
        }
        if (file.Directory()) {
            lookup = file.Path().c_str();
        }

        auto icon = slg::GetShellIconImage(lookup, file.Directory(), 16, useFastAttrQuery, cacheKey);
        if (!icon) {
            cleanup();
            co_return;
        }
        iconCache.insert_or_assign(cacheKey, icon);

        if (!IsLoaded() || loadToken != m_currentLoadToken) {
            cleanup();
            co_return;
        }

        auto pendingIt = iconPendingFiles.find(cacheKey);
        if (pendingIt != iconPendingFiles.end()) {
            for (auto const& pendingFile : pendingIt->second) {
                if (pendingFile && !pendingFile.Icon()) {
                    pendingFile.Icon(icon);
                    UpdateRealizedItemIcon(pendingFile, icon);
                }
            }
            iconPendingFiles.erase(pendingIt);
        }

        if (file && !file.Icon()) {
            file.Icon(icon);
            UpdateRealizedItemIcon(file, icon);
        }
        iconLoadingKeys.erase(cacheKey);
        }
        catch (...) {
            cleanup();
            co_return;
        }

        co_return;
    }

    void FilePage::UpdateRealizedItemIcon(winrt::StarlightGUI::FileInfo const& file, winrt::Microsoft::UI::Xaml::Media::ImageSource const& icon)
    {
        if (!file || !icon || !IsLoaded()) return;

        auto container = FileListView().ContainerFromItem(file).try_as<ListViewItem>();
        if (!container) return;

        auto root = container.ContentTemplateRoot();
        if (!root) return;

        auto image = slg::FindVisualChild<Image>(root);
        if (image) {
            image.Source(icon);
        }
    }

    void FilePage::SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (!IsLoaded()) return;
        if (m_isSyncingTab) return;

        auto tabId = GetCurrentTabId();
        if (!tabId.empty()) {
            auto stateIt = m_tabStates.find(tabId);
            if (stateIt != m_tabStates.end()) {
                stateIt->second.searchText = SearchBox().Text().c_str();
            }
        }
        WaitAndReloadAsync(250);
    }

    bool FilePage::ApplyFilter(const winrt::StarlightGUI::FileInfo& file, hstring& query) {
        return !ContainsIgnoreCase(file.Name().c_str(), query.c_str());
    }

    void FilePage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        ++m_currentLoadToken;

        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();

        struct SortBinding {
            wchar_t const* tag;
            char const* column;
            bool* ascending;
        };

        static const std::array<SortBinding, 3> bindings{ {
            { L"Name", "Name", &FilePage::m_isNameAscending },
            { L"ModifyTime", "ModifyTime", &FilePage::m_isModifyTimeAscending },
            { L"Size", "Size", &FilePage::m_isSizeAscending },
        } };

        for (auto const& binding : bindings) {
            if (columnName == binding.tag) {
                ApplySort(*binding.ascending, binding.column);
                break;
            }
        }

        ResetState();

        auto newFileList = winrt::multi_threaded_observable_vector<winrt::StarlightGUI::FileInfo>();

        if (currentDirectory != hstring(kFileHomePage)) {
            auto previousPage = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
            previousPage.Name(currentDirectory.size() <= 3 ? GetLocalizedString(L"File_BackToPC").c_str() : GetLocalizedString(L"File_PreviousFolder").c_str());
            previousPage.Flag(999);
            newFileList.Append(previousPage);
        }

        std::wstring tabSearchText;
        auto tabId = GetCurrentTabId();
        if (!tabId.empty()) {
            auto stateIt = m_tabStates.find(tabId);
            if (stateIt != m_tabStates.end()) tabSearchText = stateIt->second.searchText;
        }

        winrt::hstring query = hstring(tabSearchText);
        std::wstring lowerQuery;
        if (!query.empty()) lowerQuery = ToLowerCase(query.c_str());
        for (size_t i = 0; i < m_allFiles.size(); ++i) {
            bool shouldRemove = lowerQuery.empty() ? false : !ContainsIgnoreCaseLowerQuery(m_allFiles[i].Name().c_str(), lowerQuery);
            if (shouldRemove) continue;

            newFileList.Append(m_allFiles[i]);
        }

        m_fileList = newFileList;
        FileListView().ItemsSource(m_fileList);

    }

    // 排序切换
    slg::coroutine FilePage::ApplySort(bool& isAscending, const std::string& column)
    {
        SortFileList(isAscending, column, true);

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;

        co_return;
    }

    void FilePage::SortFileList(bool isAscending, const std::string& column, bool updateHeader)
    {
        if (column.empty()) return;

        enum class SortColumn {
            Unknown,
            Name,
            ModifyTime,
            Size
        };

        auto resolveSortColumn = [&](const std::string& key) -> SortColumn {
            if (key == "Name") return SortColumn::Name;
            if (key == "ModifyTime") return SortColumn::ModifyTime;
            if (key == "Size") return SortColumn::Size;
            return SortColumn::Unknown;
            };

        auto activeColumn = resolveSortColumn(column);
        if (activeColumn == SortColumn::Unknown) return;

        if (updateHeader) {
            NameHeaderButton().Content(tbox(L"File_ColFile_Text"));
            ModifyTimeHeaderButton().Content(tbox(L"File_ColModifyTime_Text"));
            SizeHeaderButton().Content(tbox(L"File_ColSize_Text"));

            if (activeColumn == SortColumn::Name) NameHeaderButton().Content(isAscending ? tbox(L"File_ColFile_Down") : tbox(L"File_ColFile_Up"));
            if (activeColumn == SortColumn::ModifyTime) ModifyTimeHeaderButton().Content(isAscending ? tbox(L"File_ColModifyTime_Down") : tbox(L"File_ColModifyTime_Up"));
            if (activeColumn == SortColumn::Size) SizeHeaderButton().Content(isAscending ? tbox(L"File_ColSize_Down") : tbox(L"File_ColSize_Up"));
        }

        auto sortActiveColumn = [&](const winrt::StarlightGUI::FileInfo& a, const winrt::StarlightGUI::FileInfo& b) -> bool {
            switch (activeColumn) {
            case SortColumn::Name:
                return LessIgnoreCase(a.Name().c_str(), b.Name().c_str());
            case SortColumn::ModifyTime:
                return a.ModifyTimeULong() < b.ModifyTimeULong();
            case SortColumn::Size:
                return a.SizeULong() < b.SizeULong();
            default:
                return false;
            }
            };

        if (isAscending) {
            std::sort(m_allFiles.begin(), m_allFiles.end(), sortActiveColumn);
            std::partition(m_allFiles.begin(), m_allFiles.end(), [](auto const& file) { return file.Directory(); });
        }
        else {
            std::sort(m_allFiles.begin(), m_allFiles.end(), [&](const auto& a, const auto& b) {
                return sortActiveColumn(b, a);
                });
            std::partition(m_allFiles.begin(), m_allFiles.end(), [](auto const& file) { return !file.Directory(); });
        }
    }

    slg::coroutine FilePage::RefreshButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (m_isLoadingFiles || m_isPostLoading) co_return;
        RefreshButton().IsEnabled(false);

        iconCache.clear();
        slg::ClearShellIconCache();
        iconLoadingKeys.clear();
        iconPendingFiles.clear();

        co_await LoadFileList();
        RefreshButton().IsEnabled(true);
        co_return;
    }

    void FilePage::BackButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (m_isLoadingFiles || m_isPostLoading) return;

        auto tabId = GetCurrentTabId();
        if (tabId.empty()) return;

        auto stateIt = m_tabStates.find(tabId);
        if (stateIt == m_tabStates.end()) return;
        auto& state = stateIt->second;

        if (state.historyIndex <= 0) return;
        state.historyIndex--;
        currentDirectory = hstring(state.history[state.historyIndex]);
        SyncCurrentTabUI();
        LoadFileList();
    }

    void FilePage::ForwardButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (m_isLoadingFiles || m_isPostLoading) return;

        auto tabId = GetCurrentTabId();
        if (tabId.empty()) return;

        auto stateIt = m_tabStates.find(tabId);
        if (stateIt == m_tabStates.end()) return;
        auto& state = stateIt->second;

        if (state.historyIndex >= static_cast<int>(state.history.size()) - 1) return;
        state.historyIndex++;
        currentDirectory = hstring(state.history[state.historyIndex]);
        SyncCurrentTabUI();
        LoadFileList();
    }

    void FilePage::UpButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (m_isLoadingFiles || m_isPostLoading) return;
        if (currentDirectory == hstring(kFileHomePage)) return;
        if (currentDirectory.size() <= 3) return;
        NavigateTo(GetParentDirectory(currentDirectory.c_str()), true);
    }

    void FilePage::ResetState() {
        m_fileList.Clear();
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::WaitAndReloadAsync(int interval) {
        auto lifetime = get_strong();
        auto requestVersion = ++m_reloadRequestVersion;

        co_await winrt::resume_after(std::chrono::milliseconds(interval));
        co_await wil::resume_foreground(DispatcherQueue());

        if (!IsLoaded() || requestVersion != m_reloadRequestVersion) co_return;
        LoadFileList();

        co_return;
    }

    slg::coroutine FilePage::CopyFiles() {
        auto dialog = winrt::make<winrt::StarlightGUI::implementation::CopyFileDialog>();
        dialog.XamlRoot(this->XamlRoot());

        auto result = co_await dialog.ShowAsync();

        if (result == ContentDialogResult::Primary) {
            std::wstring copyPath = dialog.CopyPath().c_str();

            std::vector<winrt::StarlightGUI::FileInfo> selectedFiles;

            for (const auto& file : FileListView().SelectedItems()) {
                auto item = file.as<winrt::StarlightGUI::FileInfo>();
                // 跳过上个文件夹选项
                if (item.Flag() == 999) continue;
				if (item.Directory()) continue;
                selectedFiles.push_back(item);
            }

            int successCount = 0, failedCount = 0;
            for (const auto& item : selectedFiles) {
                bool result = false;
                try {
                    if (fs::is_directory(item.Path().c_str())) {
                        fs::copy(item.Path().c_str(), copyPath + item.Name().c_str(), fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                        result = true;
                    }
                    else if (fs::is_regular_file(item.Path().c_str())) {
                        result = fs::copy_file(item.Path().c_str(), copyPath + item.Name().c_str(), fs::copy_options::overwrite_existing);
                    }
                }
                catch (...) {
                    result = false;
                }
                
                if (result) successCount++;
				else failedCount++;
            }
            if (successCount > 0) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Success").c_str(), GetLocalizedString(L"File_CopySuccess").c_str() + to_hstring(successCount) + GetLocalizedString(L"File_FilesSuffix").c_str(), InfoBarSeverity::Success, g_mainWindowInstance);
                co_await LoadFileList();
            }
            if (failedCount > 0) {
                slg::CreateInfoBarAndDisplay(GetLocalizedString(L"Msg_Failure").c_str(), hstring(L"") + to_hstring(failedCount) + GetLocalizedString(L"File_FileCopyPartialFail").c_str(), InfoBarSeverity::Error, g_mainWindowInstance);
            }
        }
    }
}





