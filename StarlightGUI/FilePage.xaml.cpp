#include "pch.h"
#include "FilePage.xaml.h"
#if __has_include("FilePage.g.cpp")
#include "FilePage.g.cpp"
#endif

#include <chrono>
#include <shellapi.h>
#include <CopyFileDialog.xaml.h>

using namespace winrt;
using namespace WinUI3Package;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;

namespace winrt::StarlightGUI::implementation
{
	hstring currentDirectory = L"C:\\";
    static hstring safeAcceptedName = L"";
    static std::unordered_map<std::wstring, std::optional<winrt::Microsoft::UI::Xaml::Media::ImageSource>> iconCache;
    static HDC hdc{ nullptr };

    FilePage::FilePage() {
        InitializeComponent();

        hdc = GetDC(NULL);
        FileListView().ItemsSource(m_fileList);
        if (!list_animation) FileListView().ItemContainerTransitions().Clear();

        m_scrollCheckTimer = winrt::Microsoft::UI::Xaml::DispatcherTimer();
        m_scrollCheckTimer.Interval(std::chrono::milliseconds(100));
        m_scrollCheckTimer.Tick([this](auto&&, auto&&) {
            if (!m_isLoadingFiles) CheckAndLoadMoreItems();
            });

        this->Loaded([this](auto&&, auto&&) {
            m_scrollCheckTimer.Start();
            LoadFileList();
            });

        this->Unloaded([this](auto&&, auto&&) {
            if (m_scrollCheckTimer) {
                m_scrollCheckTimer.Stop();
            }
            ReleaseDC(NULL, hdc);
            });

        LOG_INFO(L"FilePage", L"FilePage initialized.");
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
                slg::CreateInfoBarAndDisplay(L"警告", L"该文件可能为重要文件，如果确认继续请再次点击！", InfoBarSeverity::Warning, g_mainWindowInstance);
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
        auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue8e5", L"打开", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (item.Directory()) {
                    currentDirectory = currentDirectory + L"\\" + item.Name();
                    LoadFileList();
                }
                else ShellExecuteW(nullptr, L"open", item.Path().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            });
        // 当选中多个内容并且其中一个是文件夹时禁用打开按钮
        if (selectedFiles.size() > 1) {
            for (const auto& item : selectedFiles) {
                if (item.Directory()) {
                    item1_1.IsEnabled(false);
                    break;
                }
            }
        }

        MenuFlyoutSeparator separator1;

        // 选项2.1
        auto item2_1 = slg::CreateMenuItem(flyoutStyles, L"\ue74d", L"删除", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (KernelInstance::DeleteFileAuto(item.Path().c_str())) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"成功删除文件/文件夹: " + item.Name() + L" (" + item.Path() + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(L"失败", L"无法删除文件/文件夹: " + item.Name() + L" (" + item.Path() + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            });

        // 选项2.2
        auto item2_2 = slg::CreateMenuItem(flyoutStyles, L"\ue733", L"删除 (内核)", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (KernelInstance::_DeleteFileAuto(item.Path().c_str())) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"成功删除文件/文件夹: " + item.Name() + L" (" + item.Path() + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(L"失败", L"无法删除文件/文件夹: " + item.Name() + L" (" + item.Path() + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            });

        // 选项2.3
        auto item2_3 = slg::CreateMenuItem(flyoutStyles, L"\uf5ab", L"删除 (内存抹杀)", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (KernelInstance::MurderFileAuto(item.Path().c_str())) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"成功删除文件/文件夹: " + item.Name() + L" (" + item.Path() + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(L"失败", L"无法删除文件/文件夹: " + item.Name() + L" (" + item.Path() + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            });

        // 选项2.4
        auto item2_4 = slg::CreateMenuItem(flyoutStyles, L"\ue72e", L"锁定", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            for (const auto& item : selectedFiles) {
                if (KernelInstance::LockFile(item.Path().c_str())) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"成功锁定文件: " + item.Name() + L" (" + item.Path() + L")", InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(L"失败", L"无法锁定文件: " + item.Name() + L" (" + item.Path() + L"), 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
            });

        // 选项2.5
        auto item2_5 = slg::CreateMenuItem(flyoutStyles, L"\ue8c8", L"复制", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) {
            CopyFiles();
            });

        MenuFlyoutSeparator separator2;

        // 选项3.1
        auto item3_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", L"复制信息");
        auto item3_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue8ac", L"名称", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(selectedFiles[0].Name().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub1);
        auto item3_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", L"路径", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(selectedFiles[0].Path().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub2);
        auto item3_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\uec92", L"修改日期", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::CopyToClipboard(selectedFiles[0].ModifyTime().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已复制内容至剪贴板", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制内容至剪贴板, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });
        item3_1.Items().Append(item3_1_sub3);

        // 选项3.2
        auto item3_2 = slg::CreateMenuItem(flyoutStyles, L"\uec50", L"在文件管理器内打开", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::OpenFolderAndSelectFile(selectedFiles[0].Path().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已打开文件夹", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法打开文件夹, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });


        // 选项3.3
        auto item3_3 = slg::CreateMenuItem(flyoutStyles, L"\ue8ec", L"属性", [this, selectedFiles](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
            if (TaskUtils::OpenFileProperties(selectedFiles[0].Path().c_str())) {
                slg::CreateInfoBarAndDisplay(L"成功", L"已打开文件属性", InfoBarSeverity::Success, g_mainWindowInstance);
            }
            else slg::CreateInfoBarAndDisplay(L"失败", L"无法打开文件属性, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            co_return;
            });

		// 当选中多个内容并且其中一个是文件夹时禁用锁定部分只能操作文件的按钮
        for (const auto& item : selectedFiles) {
            if (item.Directory()) {
                item2_4.IsEnabled(false);
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
            currentDirectory = GetParentDirectory(currentDirectory.c_str());
            LoadFileList();
        } else if (item.Directory()) {
            currentDirectory = currentDirectory + L"\\" + item.Name();
            LoadFileList();
        }
        else {
            ShellExecuteW(nullptr, L"open", item.Path().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
	}

    void FilePage::FileListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {
        if (args.InRecycleQueue())
            return;

        // 将 Tag 设到容器上，便于 ListViewItemPresenter 通过 TemplatedParent 绑定
        if (auto itemContainer = args.ItemContainer())
            itemContainer.Tag(sender.Tag());
    }

    void FilePage::CheckAndLoadMoreItems() {
        if (!m_listScrollViewer && !FindScrollViewer(FileListView())) return;
        if (m_isLoadingMore || !m_hasMoreFiles) return;
        LoadMoreFiles();
    }

    slg::coroutine FilePage::LoadMoreFiles() {
        if (m_isLoadingMore || m_loadedCount >= m_allFiles.size()) {
            m_hasMoreFiles = false;
            co_return;
        }
        m_isLoadingMore = true;

        auto lifetime = get_strong();

        try {
            size_t start = m_loadedCount;
            size_t end = (start + 100) < m_allFiles.size() ? (start + 100) : m_allFiles.size();

            co_await wil::resume_foreground(DispatcherQueue());

            winrt::hstring query = SearchBox().Text();

            for (size_t i = start; i < end; ++i) {
                bool shouldRemove = query.empty() ? false : ApplyFilter(m_allFiles[i], query);
                if (shouldRemove) continue;

                co_await GetFileIconAsync(m_allFiles[i]);
                m_fileList.Append(m_allFiles[i]);
            }

            m_loadedCount = end;
            m_hasMoreFiles = (m_loadedCount < m_allFiles.size());

            LOG_INFO(__WFUNCTION__, L"Loading file list range [%d,%d)", start, end);
        }
        catch (...) {
            LOG_ERROR(__WFUNCTION__, L"Error while loading file list!");
        }

        m_isLoadingMore = false;
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::LoadFileList()
    {
        if (m_isLoadingFiles) co_return;
        m_isLoadingFiles = true;

        LOG_INFO(__WFUNCTION__, L"Loading file list...");
        ResetState();
        LoadingRing().IsActive(true);

        auto start = std::chrono::steady_clock::now();

        auto lifetime = get_strong();

        std::wstring path = FixBackSplash(currentDirectory);
        currentDirectory = path;
        PathBox().Text(currentDirectory);
        LOG_INFO(__WFUNCTION__, L"Path = %s", path.c_str());

        // 简单判断根目录
        PreviousButton().IsEnabled(path.length() > 3);

        co_await winrt::resume_background();

        m_allFiles.clear();

        KernelInstance::QueryFile(path, m_allFiles);
        LOG_INFO(__WFUNCTION__, L"Enumerated files (kernel mode), %d entry(s).", m_allFiles.size());

        for (const auto& file : m_allFiles) {
            co_await GetFileInfoAsync(file);
            file.Path(FixBackSplash(file.Path()));
        }

        co_await wil::resume_foreground(DispatcherQueue());

        ApplySort(currentSortingOption, currentSortingType);

        // 将文件夹放在文件前面
        std::sort(m_allFiles.begin(), m_allFiles.end(), [](const auto& a, const auto& b) {
            if (a.Directory() && !b.Directory()) {
                return true;
            }
            return false;
            });

        AddPreviousItem();

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::wstringstream countText;
        countText << L"共 " << m_allFiles.size() << L" 个文件 (" << duration << " ms)";
        FileCountText().Text(countText.str());
        LoadingRing().IsActive(false);

        LOG_INFO(__WFUNCTION__, L"Loaded file list, %d entry(s) in total.", m_allFiles.size());

        // 立刻加载一次
        LoadMoreFiles();

        m_isLoadingFiles = false;
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::GetFileInfoAsync(const winrt::StarlightGUI::FileInfo& file)
    {
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile(file.Path().c_str(), &findFileData);

        if (hFind != INVALID_HANDLE_VALUE) {
            if (file.SizeULong() == 0) {
                file.SizeULong(((ULONG64)findFileData.nFileSizeHigh << 32) | findFileData.nFileSizeLow);
            }
            file.Size(FormatMemorySize(file.SizeULong()));

            file.ModifyTimeULong(((ULONG64)findFileData.ftLastAccessTime.dwHighDateTime << 32) | findFileData.ftLastAccessTime.dwLowDateTime);
            SYSTEMTIME st;
            if (FileTimeToSystemTime(&findFileData.ftLastAccessTime, &st))
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
                file.ModifyTime(L"(未知)");
            }

            FindClose(hFind);
        }
        else {
            file.Size(L"-1 (未知)");
            file.ModifyTime(L"(未知)");
        }

        if (file.Directory()) file.Size(L"");

        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::GetFileIconAsync(const winrt::StarlightGUI::FileInfo& file)
    {
        std::wstring filePath = file.Path().c_str();
        if (iconCache.find(filePath) == iconCache.end()) {
            SHFILEINFO shfi;
            if (!SHGetFileInfoW(filePath.c_str(), 0, &shfi, sizeof(SHFILEINFO), SHGFI_ICON)) {
                SHGetFileInfoW(L"", 0, &shfi, sizeof(SHFILEINFO), SHGFI_ICON);
            }
            ICONINFO iconInfo;
            if (GetIconInfo(shfi.hIcon, &iconInfo)) {
                BITMAP bmp;
                GetObject(iconInfo.hbmColor, sizeof(bmp), &bmp);
                BITMAPINFOHEADER bmiHeader = { 0 };
                bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmiHeader.biWidth = bmp.bmWidth;
                bmiHeader.biHeight = bmp.bmHeight;
                bmiHeader.biPlanes = 1;
                bmiHeader.biBitCount = 32;
                bmiHeader.biCompression = BI_RGB;

                int dataSize = bmp.bmWidthBytes * bmp.bmHeight;
                std::vector<BYTE> buffer(dataSize);

                GetDIBits(hdc, iconInfo.hbmColor, 0, bmp.bmHeight, buffer.data(), reinterpret_cast<BITMAPINFO*>(&bmiHeader), DIB_RGB_COLORS);

                winrt::Microsoft::UI::Xaml::Media::Imaging::WriteableBitmap writeableBitmap(bmp.bmWidth, bmp.bmHeight);

                // 将数据写入 WriteableBitmap
                uint8_t* data = writeableBitmap.PixelBuffer().data();
                int rowSize = bmp.bmWidth * 4;
                for (int i = 0; i < bmp.bmHeight; ++i) {
                    int srcOffset = i * rowSize;
                    int dstOffset = (bmp.bmHeight - 1 - i) * rowSize;
                    std::memcpy(data + dstOffset, buffer.data() + srcOffset, rowSize);
                }

                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                DestroyIcon(shfi.hIcon);

                // 将图标缓存到 map 中
                iconCache[filePath] = writeableBitmap.as<winrt::Microsoft::UI::Xaml::Media::ImageSource>();
            }
        }
        file.Icon(iconCache[filePath].value());

        co_return;
    }

    void FilePage::SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
        if (!IsLoaded()) return;

        WaitAndReloadAsync(200);
    }

    void FilePage::PathBox_KeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& e) {
        if (e.Key() == winrt::Windows::System::VirtualKey::Enter)
        {
            try
            {
                fs::path path(PathBox().Text().c_str());
                if (fs::exists(path) && fs::is_directory(path)) {
                    currentDirectory = PathBox().Text();
                }
            }
            catch (...) {}
            PathBox().Text(currentDirectory);
            LoadFileList();
            e.Handled(true);
        }
    }

    bool FilePage::ApplyFilter(const winrt::StarlightGUI::FileInfo& file, hstring& query) {
        std::wstring name = file.Name().c_str();
        std::wstring queryWStr = query.c_str();

        // 不比较大小写
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        std::transform(queryWStr.begin(), queryWStr.end(), queryWStr.begin(), ::towlower);

        return name.find(queryWStr) == std::wstring::npos;
    }

    void FilePage::ColumnHeader_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        Button clickedButton = sender.as<Button>();
        winrt::hstring columnName = clickedButton.Tag().as<winrt::hstring>();

        if (columnName == L"Previous") {
            currentDirectory = GetParentDirectory(currentDirectory.c_str());
            LoadFileList();
            return;
        }

        if (columnName == L"Name")
        {
            ApplySort(m_isNameAscending, "Name");
        }
        else if (columnName == L"ModifyTime")
        {
            ApplySort(m_isModifyTimeAscending, "ModifyTime");
        }
        else if (columnName == L"Size")
        {
            ApplySort(m_isSizeAscending, "Size");
        }

        ResetState();
        AddPreviousItem();
    }

    // 排序切换
    slg::coroutine FilePage::ApplySort(bool& isAscending, const std::string& column)
    {
        NameHeaderButton().Content(box_value(L"文件"));
        ModifyTimeHeaderButton().Content(box_value(L"修改时间"));
        SizeHeaderButton().Content(box_value(L"大小"));

        if (column == "Name") {
            if (isAscending) {
                NameHeaderButton().Content(box_value(L"文件 ↓"));
                std::sort(m_allFiles.begin(), m_allFiles.end(), [](auto a, auto b) {
                    std::wstring aName = a.Name().c_str();
                    std::wstring bName = b.Name().c_str();
                    std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

                    return aName < bName;
                    });

            }
            else {
                NameHeaderButton().Content(box_value(L"文件 ↑"));
                std::sort(m_allFiles.begin(), m_allFiles.end(), [](auto a, auto b) {
                    std::wstring aName = a.Name().c_str();
                    std::wstring bName = b.Name().c_str();
                    std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
                    std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

                    return aName > bName;
                    });
            }
        } else if (column == "ModifyTime") {
            if (isAscending) {
                ModifyTimeHeaderButton().Content(box_value(L"修改时间 ↓"));
                std::sort(m_allFiles.begin(), m_allFiles.end(), [](auto a, auto b) {
                    return a.ModifyTimeULong() < b.ModifyTimeULong();
                    });

            }
            else {
                ModifyTimeHeaderButton().Content(box_value(L"修改时间 ↑"));
                std::sort(m_allFiles.begin(), m_allFiles.end(), [](auto a, auto b) {
                    return a.ModifyTimeULong() > b.ModifyTimeULong();
                    });
            }
        } else if (column == "Size") {
            if (isAscending) {
                SizeHeaderButton().Content(box_value(L"大小 ↓"));
                std::sort(m_allFiles.begin(), m_allFiles.end(), [](auto a, auto b) {
                    return a.SizeULong() < b.SizeULong();
                    });

            }
            else {
                SizeHeaderButton().Content(box_value(L"大小 ↑"));
                std::sort(m_allFiles.begin(), m_allFiles.end(), [](auto a, auto b) {
                    return a.SizeULong() > b.SizeULong();
                    });
            }
        }

        isAscending = !isAscending;
        currentSortingOption = !isAscending;
        currentSortingType = column;

        co_return;
    }

    slg::coroutine FilePage::RefreshButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        RefreshButton().IsEnabled(false);
        co_await LoadFileList();
        RefreshButton().IsEnabled(true);
        co_return;
    }

    slg::coroutine FilePage::NextDriveButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        static std::vector<std::wstring> drives;
        static int currentIndex = 1;

        if (drives.empty()) {
            wchar_t driveBuffer[256];
            GetLogicalDriveStrings(255, driveBuffer);

            for (wchar_t* drive = driveBuffer; *drive; drive += wcslen(drive) + 1) {
                drives.push_back(drive);
            }

            if (drives.empty()) drives = { L"C:\\" };
        }

        currentDirectory = drives[currentIndex];
        currentIndex = (currentIndex + 1) % drives.size();

        co_await LoadFileList();
    }

    bool FilePage::FindScrollViewer(DependencyObject parent) {
        if (!m_listScrollViewer) {
            if (auto sv = parent.try_as<winrt::Microsoft::UI::Xaml::Controls::ScrollViewer>()) {
                m_listScrollViewer = sv;
                return true;
            }
            auto childrenCount = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetChildrenCount(parent);
            for (int i = 0; i < childrenCount; ++i) {
                auto child = winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetChild(parent, i);
                auto result = FindScrollViewer(child);
                if (result) break;
            }
            if (!m_listScrollViewer) return false;
        }
        return true;
    }

    void FilePage::AddPreviousItem() {
        // 简单判断根目录
        if (currentDirectory.size() <= 3) return;
        if (m_fileList.Size() > 0 && m_fileList.GetAt(0).Flag() == 999) return;
        auto previousPage = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
        previousPage.Name(L"上个文件夹");
        previousPage.Flag(999);
        GetFileIconAsync(previousPage);
        m_fileList.InsertAt(0, previousPage);
    }

    void FilePage::ResetState() {
        m_fileList.Clear();
        m_loadedCount = 0;
        m_isLoadingMore = false;
        m_hasMoreFiles = true;
    }

    winrt::Windows::Foundation::IAsyncAction FilePage::WaitAndReloadAsync(int interval) {
        auto lifetime = get_strong();

        reloadTimer.Stop();
        reloadTimer.Interval(std::chrono::milliseconds(interval));
        reloadTimer.Tick([this](auto&&, auto&&) {
            LoadFileList();
            reloadTimer.Stop();
            });
        reloadTimer.Start();

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
                selectedFiles.push_back(item);
            }

            for (const auto& item : selectedFiles) {
                if (KernelInstance::_CopyFile(std::wstring(item.Path().c_str()).substr(0, item.Path().size() - item.Name().size()), copyPath + L"\\" + item.Name().c_str(), item.Name().c_str())) {
                    slg::CreateInfoBarAndDisplay(L"成功", L"成功复制文件至: " + dialog.CopyPath(), InfoBarSeverity::Success, g_mainWindowInstance);
                    WaitAndReloadAsync(1000);
                }
                else slg::CreateInfoBarAndDisplay(L"失败", L"无法复制文件, 错误码: " + to_hstring((int)GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
            }
        }
    }
}
