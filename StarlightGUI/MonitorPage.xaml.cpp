#include "pch.h"
#include "MonitorPage.xaml.h"
#if __has_include("MonitorPage.g.cpp")
#include "MonitorPage.g.cpp"
#endif
#include <winrt/XamlToolkit.WinUI.Controls.h>
#include <unordered_set>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace WinUI3Package;
using namespace XamlToolkit::WinUI::Controls;

namespace winrt::StarlightGUI::implementation
{
	static std::vector<winrt::StarlightGUI::ObjectEntry> partitions;

    static hstring GetMonitorSuggestionKey(int segmentedIndex, winrt::StarlightGUI::GeneralEntry const& entry)
    {
        switch (segmentedIndex) {
        case 4:
            if (!entry.String2().empty()) return entry.String2();
            if (!entry.String3().empty()) return entry.String3();
            return entry.String1();
        case 7:
        case 9:
        case 10:
        case 11:
            return entry.String1();
        default:
            if (!entry.String1().empty()) return entry.String1();
            if (!entry.String2().empty()) return entry.String2();
            return entry.String3();
        }
    }

    void MonitorPage::EnsureHeaderSplitters(winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid)
    {
        if (!headerGrid) return;

        auto columns = headerGrid.ColumnDefinitions();
        if (columns.Size() < 2) return;

        for (uint32_t column = 0; column + 1 < columns.Size(); ++column) {
            bool exists = false;
            for (auto const& child : headerGrid.Children()) {
                auto splitter = child.try_as<GridSplitter>();
                if (!splitter) continue;
                if (Grid::GetColumn(splitter) != static_cast<int>(column)) continue;
                exists = true;
                break;
            }

            if (exists) continue;

            GridSplitter splitter;
            splitter.Width(9);
            splitter.Margin(ThicknessHelper::FromLengths(0, 0, -5, 0));
            splitter.Opacity(0);
            splitter.HorizontalAlignment(HorizontalAlignment::Right);
            splitter.VerticalAlignment(VerticalAlignment::Stretch);
            splitter.Background(SolidColorBrush(Windows::UI::Colors::Transparent()));
            splitter.ResizeBehavior(GridResizeBehavior::BasedOnAlignment);
            splitter.ResizeDirection(GridResizeDirection::Columns);
            Grid::SetColumn(splitter, column);

            headerGrid.Children().Append(splitter);
        }
    }

    void MonitorPage::AttachColumnSyncToSection(winrt::Microsoft::UI::Xaml::Controls::Grid const& sectionRoot, uint32_t rowOffset)
    {
        if (!sectionRoot) return;

        Grid headerGrid{ nullptr };
        Grid bodyGrid{ nullptr };

        auto tryResolveHeaderBody = [&](Grid const& container) -> bool {
            if (!container) return false;

            Grid header{ nullptr };
            Grid body{ nullptr };
            for (auto const& child : container.Children()) {
                auto border = child.try_as<Border>();
                if (!border) continue;

                auto grid = border.Child().try_as<Grid>();
                if (!grid) continue;

                int row = Grid::GetRow(border);
                if (row == 0) header = grid;
                else if (row == 1) body = grid;
            }

            if (header && body) {
                headerGrid = header;
                bodyGrid = body;
                return true;
            }
            return false;
            };

        if (!tryResolveHeaderBody(sectionRoot)) {
            for (auto const& child : sectionRoot.Children()) {
                auto childGrid = child.try_as<Grid>();
                if (!childGrid) continue;
                if (tryResolveHeaderBody(childGrid)) break;
            }
        }

        if (!headerGrid || !bodyGrid) return;

        auto listView = slg::FindVisualChild<ListView>(bodyGrid);
        if (!listView) return;

        EnsureHeaderSplitters(headerGrid);

        m_columnSyncBindings.push_back({ headerGrid, bodyGrid, listView, rowOffset });

        auto weak = get_weak();
        headerGrid.LayoutUpdated([weak, headerGrid, bodyGrid, listView, rowOffset](auto&&, auto&&) {
            if (auto self = weak.get()) {
                slg::SyncListViewColumnWidths(headerGrid, bodyGrid, listView, rowOffset);
            }
            });

        listView.ContainerContentChanging([weak, headerGrid, rowOffset](auto&&, auto&& args) {
            if (args.InRecycleQueue()) return;
            auto itemContainer = args.ItemContainer().try_as<ListViewItem>();
            if (!itemContainer) return;
            if (auto self = weak.get()) {
                slg::ApplyHeaderColumnWidthsToContainer(headerGrid, itemContainer, rowOffset);
            }
            });
    }

    void MonitorPage::InitializeColumnSyncBindings()
    {
        m_columnSyncBindings.clear();

        AttachColumnSyncToSection(ObjectGrid(), 0);
        AttachColumnSyncToSection(CallbackGrid(), 0);
        AttachColumnSyncToSection(MiniFilterGrid(), 0);
        AttachColumnSyncToSection(StdFilterGrid(), 0);
        AttachColumnSyncToSection(SSDTGrid(), 0);
        AttachColumnSyncToSection(SSSDTGrid(), 0);
        AttachColumnSyncToSection(IoTimerGrid(), 0);
        AttachColumnSyncToSection(ExCallbackGrid(), 0);
        AttachColumnSyncToSection(IDTGrid(), 0);
        AttachColumnSyncToSection(GDTGrid(), 0);
        AttachColumnSyncToSection(PiDDBGrid(), 0);
        AttachColumnSyncToSection(HALDPTGrid(), 0);
        AttachColumnSyncToSection(HALPDPTGrid(), 0);

        for (auto const& binding : m_columnSyncBindings) {
            slg::SyncListViewColumnWidths(binding.HeaderGrid, binding.BodyGrid, binding.ListView, binding.RowOffset);
        }
    }

	MonitorPage::MonitorPage() {
		InitializeComponent();
		SetupLocalization();

		// 初始化所有列表
		{
			ObjectTreeView().ItemsSource(m_itemList);
			ObjectListView().ItemsSource(m_objectList);
			CallbackListView().ItemsSource(m_generalList);
			MiniFilterListView().ItemsSource(m_generalList);
			StdFilterListView().ItemsSource(m_generalList);
			SSDTListView().ItemsSource(m_generalList);
			SSSDTListView().ItemsSource(m_generalList);
			IoTimerListView().ItemsSource(m_generalList);
			ExCallbackListView().ItemsSource(m_generalList);
			IDTListView().ItemsSource(m_generalList);
			GDTListView().ItemsSource(m_generalList);
			PiDDBListView().ItemsSource(m_generalList);
			HALDPTListView().ItemsSource(m_generalList);
			HALPDPTListView().ItemsSource(m_generalList);
		}
        InitializeColumnSyncBindings();

		winrt::Microsoft::UI::Xaml::Application::Current().Resources().MergedDictionaries();

		Unloaded([this](auto&&, auto&&) {
			++m_reloadRequestVersion;
			windbgTimer.Stop();
			});

		windbgTimer.Interval(std::chrono::seconds(1));
		windbgTimer.Tick([this](auto&&, auto&&) {
			std::lock_guard<std::mutex> guard(dbgViewMutex);
			uint32_t currentLength = static_cast<uint32_t>(dbgViewData.size());
			if (currentLength == m_lastDbgViewLength) return;
			DbgViewBox().Text(dbgViewData);
			m_lastDbgViewLength = currentLength;
			});

		LOG_INFO(L"MonitorPage", L"MonitorPage initialized.");
	}

	winrt::Windows::Foundation::IAsyncAction MonitorPage::LoadPartitionList(std::wstring path) {
		if (segmentedIndex != 0) co_return;

		std::vector<winrt::StarlightGUI::ObjectEntry> partitionsInPath;

		KernelInstance::EnumObjectsByDirectory(path, partitionsInPath);
		for (const auto& object : partitionsInPath) {
			if (object.Type() == L"Directory") {
				partitions.push_back(object);
				co_await LoadPartitionList(object.Path().c_str());
			}
		}

		co_return;
	}

	winrt::Windows::Foundation::IAsyncAction MonitorPage::LoadObjectList() {
		if (m_isLoading || segmentedIndex != 0 || !ObjectTreeView().SelectedItem() || partitions.size() == 0) {
			co_return;
		}
		m_isLoading = true;

		LOG_INFO(__WFUNCTION__, L"Loading object list...");

		auto lifetime = get_strong();
		int32_t index = ObjectTreeView().SelectedIndex();
		hstring query = SearchBox().Text();
		std::wstring lowerQuery;
		if (!query.empty()) lowerQuery = ToLowerCase(query.c_str());

		co_await winrt::resume_background();

		std::vector<winrt::StarlightGUI::ObjectEntry> objects;

		// 获取对象逻辑
		winrt::StarlightGUI::ObjectEntry& selectedPartition = partitions[index];
		KernelInstance::EnumObjectsByDirectory(selectedPartition.Path().c_str(), objects);

		co_await wil::resume_foreground(DispatcherQueue());

		m_objectList.Clear();
		for (const auto& object : objects) {
			bool shouldRemove = lowerQuery.empty() ? false : !ContainsIgnoreCaseLowerQuery(object.Name().c_str(), lowerQuery);
			if (shouldRemove) continue;

			if (object.Name().empty()) object.Name(t(L"Common.Unknown"));
			if (object.Type().empty()) object.Type(t(L"Common.Unknown"));
			if (object.CreationTime().empty()) object.CreationTime(t(L"Common.Unknown"));
			if (!object.Link().empty()) object.Path(object.Link());

			m_objectList.Append(object);
		}

		LOG_INFO(__WFUNCTION__, L"Loaded object list, %d entry(s) in total.", m_objectList.Size());
		m_isLoading = false;
	}

	winrt::Windows::Foundation::IAsyncAction MonitorPage::LoadGeneralList(bool force) {
		if (m_isLoading) {
			co_return;
		}
		m_isLoading = true;

		LOG_INFO(__WFUNCTION__, L"Loading general list...");

		auto requestedIndex = segmentedIndex;
		auto lifetime = get_strong();
		hstring query = SearchBox().Text();
		std::wstring lowerQuery;
		if (!query.empty()) lowerQuery = ToLowerCase(query.c_str());

		co_await winrt::resume_background();

		std::vector<winrt::StarlightGUI::GeneralEntry> entries;
		std::vector<winrt::StarlightGUI::GeneralEntry> const* entriesSource = &entries;

		static std::vector<winrt::StarlightGUI::GeneralEntry> callbackCache, minifilterCache, standardfilterCache, ssdtCache, sssdtCache, ioTimerCache, exCallbackCache, idtCache, gdtCache, piddbCache, halDptCache, halPdptCache;

		switch (requestedIndex) {
		case 2:
			if (force || callbackCache.empty()) {
				KernelInstance::EnumNotifies(entries);
				callbackCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &callbackCache;
			break;
		case 3:
			if (force || minifilterCache.empty()) {
				KernelInstance::EnumMiniFilter(entries);
				minifilterCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &minifilterCache;
			break;
		case 4:
			if (force || standardfilterCache.empty()) {
				KernelInstance::EnumStandardFilter(entries);
				standardfilterCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &standardfilterCache;
			break;
		case 5:
			if (force || ssdtCache.empty()) {
				KernelInstance::EnumSSDT(entries);
				ssdtCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &ssdtCache;
			break;
		case 6:
			if (force || sssdtCache.empty()) {
				KernelInstance::EnumSSSDT(entries);
				sssdtCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &sssdtCache;
			break;
		case 7:
			if (force || ioTimerCache.empty()) {
				KernelInstance::EnumIoTimer(entries);
				ioTimerCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &ioTimerCache;
			break;
		case 8:
			if (force || exCallbackCache.empty()) {
				KernelInstance::EnumExCallback(entries);
				exCallbackCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &exCallbackCache;
			break;
		case 9:
			if (force || idtCache.empty()) {
				KernelInstance::EnumIDT(entries);
				idtCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &idtCache;
			break;
		case 10:
			if (force || gdtCache.empty()) {
				KernelInstance::EnumGDT(entries);
				gdtCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &gdtCache;
			break;
		case 11:
			if (force || piddbCache.empty()) {
				KernelInstance::EnumPiDDBCacheTable(entries);
				piddbCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &piddbCache;
			break;
		case 12:
			if (force || halDptCache.empty()) {
				KernelInstance::EnumHalDispatchTable(entries);
				halDptCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &halDptCache;
			break;
		case 13:
			if (force || halPdptCache.empty()) {
				KernelInstance::EnumHalPrivateDispatchTable(entries);
				halPdptCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &halPdptCache;
			break;
		}

		co_await wil::resume_foreground(DispatcherQueue());

		// 防止意外
		if (requestedIndex != segmentedIndex) {
			m_isLoading = false;
			co_return;
		}

		m_generalList.Clear();
		for (const auto& entry : *entriesSource) {
			bool shouldRemove = false;
			if (!lowerQuery.empty()) {
				switch (requestedIndex) {
				case 2:
				case 3:
				case 5:
				case 6:
				case 8:
				case 12:
				case 13:
					shouldRemove = !ContainsIgnoreCaseLowerQuery(entry.String1().c_str(), lowerQuery) && !ContainsIgnoreCaseLowerQuery(entry.String2().c_str(), lowerQuery);
					break;
				case 4:
					shouldRemove = !ContainsIgnoreCaseLowerQuery(entry.String2().c_str(), lowerQuery) && !ContainsIgnoreCaseLowerQuery(entry.String3().c_str(), lowerQuery);
					break;
				case 7:
				case 9:
				case 10:
				case 11:
					shouldRemove = !ContainsIgnoreCaseLowerQuery(entry.String1().c_str(), lowerQuery);
					break;
				}
			}
			if (shouldRemove) continue;

			if (entry.String1().empty()) entry.String1(t(L"Common.Unknown"));
			if (entry.String2().empty()) entry.String2(t(L"Common.Unknown"));
			if (entry.String3().empty()) entry.String3(t(L"Common.Unknown"));
			if (entry.String4().empty()) entry.String4(t(L"Common.Unknown"));
			if (entry.String5().empty()) entry.String5(t(L"Common.Unknown"));
			if (entry.String6().empty()) entry.String6(t(L"Common.Unknown"));

			m_generalList.Append(entry);
		}

		LOG_INFO(__WFUNCTION__, L"Loaded general list, %d entry(s) in total.", m_generalList.Size());
		m_isLoading = false;
	}

	void MonitorPage::ObjectListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = sender.as<ListView>();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem() || segmentedIndex != 0) return;
		auto item = listView.SelectedItem().as<winrt::StarlightGUI::ObjectEntry>();

		// 获取信息
		BOOL status = KernelInstance::GetObjectDetails(item.Path().c_str(), item.Type().c_str(), item);

		Flyout flyout;
		StackPanel flyoutPanel;
		auto createSelectableTextBlock = [](hstring const& text) {
			TextBlock textBlock;
			textBlock.IsTextSelectionEnabled(true);
			textBlock.Text(text);
			return textBlock;
			};

		// 基本信息
		GroupBox basicInfoBox;
		StackPanel basicInfoPanel;
		basicInfoBox.Header(t(L"Monitor.BasicInfo"));
		basicInfoBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
		auto name = createSelectableTextBlock(t(L"Monitor.Label.Name") + item.Name());
		auto type = createSelectableTextBlock(t(L"Monitor.Label.Type") + item.Type());
		auto path = createSelectableTextBlock(t(L"Monitor.Label.FullPath") + item.Path());
		CheckBox permanent;
		permanent.Content(tbox(L"Monitor.Permanent"));
		permanent.IsChecked(item.Permanent());
		permanent.IsEnabled(false);
		basicInfoPanel.Children().Append(name);
		basicInfoPanel.Children().Append(type);
		basicInfoPanel.Children().Append(path);
		basicInfoPanel.Children().Append(permanent);
		basicInfoBox.Content(basicInfoPanel);

		// 引用信息
		GroupBox referencesBox;
		StackPanel referencesPanel;
		referencesBox.Header(t(L"Monitor.ReferenceInfo"));
		referencesBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
		auto references = createSelectableTextBlock(t(L"Monitor.Label.References") + std::to_wstring(item.References()));
		auto handles = createSelectableTextBlock(t(L"Monitor.Label.Handles") + std::to_wstring(item.Handles()));
		referencesPanel.Children().Append(references);
		referencesPanel.Children().Append(handles);
		referencesBox.Content(referencesPanel);

		// 配额信息
		GroupBox quotaBox;
		StackPanel quotaPanel;
		quotaBox.Header(t(L"Monitor.QuotaInfo"));
		quotaBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
		auto paged = createSelectableTextBlock(t(L"Monitor.Label.PagedPool") + FormatMemorySize(item.PagedPool()));
		auto nonPaged = createSelectableTextBlock(t(L"Monitor.Label.NonPagedPool") + FormatMemorySize(item.NonPagedPool()));
		quotaPanel.Children().Append(paged);
		quotaPanel.Children().Append(nonPaged);
		quotaBox.Content(quotaPanel);

		// 详细信息
		bool flag = false;
		GroupBox detailBox;
		StackPanel detailPanel;
		detailBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
		if (item.Type() == L"SymbolicLink") {
			detailBox.Header(t(L"Monitor.SymbolicLink"));
			auto creationTime = createSelectableTextBlock(t(L"Monitor.Label.CreationTime") + item.CreationTime());
			auto linkTarget = createSelectableTextBlock(t(L"Monitor.Label.Link") + item.Link());
			detailPanel.Children().Append(creationTime);
			detailPanel.Children().Append(linkTarget);
		}
		else if (item.Type() == L"Event") {
			detailBox.Header(t(L"Monitor.Event"));
			hstring state = item.EventSignaled() ? L"TRUE" : L"FALSE";
			auto eventType = createSelectableTextBlock(t(L"Monitor.Label.EventType") + item.EventType());
			auto eventSignaled = createSelectableTextBlock(t(L"Monitor.Label.Signaled") + state);
			detailPanel.Children().Append(eventType);
			detailPanel.Children().Append(eventSignaled);
		}
		else if (item.Type() == L"Mutant") {
			detailBox.Header(t(L"Monitor.Mutant"));
			hstring state = item.MutantAbandoned() ? L"TRUE" : L"FALSE";
			auto mutantHoldCount = createSelectableTextBlock(t(L"Monitor.Label.HoldCount") + to_hstring(item.MutantHoldCount()));
			auto mutantAbandoned = createSelectableTextBlock(t(L"Monitor.Label.Abandoned") + state);
			detailPanel.Children().Append(mutantHoldCount);
			detailPanel.Children().Append(mutantAbandoned);
		}
		else if (item.Type() == L"Semaphore") {
			detailBox.Header(t(L"Monitor.Semaphore"));
			auto semaphoreCount = createSelectableTextBlock(t(L"Monitor.Label.CurrentCount") + to_hstring(item.SemaphoreCount()));
			auto semaphoreLimit = createSelectableTextBlock(t(L"Monitor.Label.MaxCount") + to_hstring(item.SemaphoreLimit()));
			detailPanel.Children().Append(semaphoreCount);
			detailPanel.Children().Append(semaphoreLimit);
		}
		else if (item.Type() == L"Section") {
			detailBox.Header(t(L"Monitor.Section"));
			hstring attr = item.SectionAttributes() == 0x200000 ? L"SEC_BASED" : item.SectionAttributes() == 0x800000 ? L"SEC_FILE" : item.SectionAttributes() == 0x4000000
				? L"SEC_RESERVE" : item.SectionAttributes() == 0x8000000 ? L"SEC_COMMIT" : item.SectionAttributes() == 0x1000000 ? L"SEC_IMAGE" : L"NULL";
			auto sectionBaseAddress = createSelectableTextBlock(t(L"Monitor.Label.Base") + ULongToHexString(item.SectionBaseAddress()));
			auto sectionMaximumSize = createSelectableTextBlock(t(L"Monitor.Label.Size") + FormatMemorySize(item.SectionMaximumSize()));
			auto sectionAttributes = createSelectableTextBlock(t(L"Monitor.Label.Attributes") + attr);
			detailPanel.Children().Append(sectionBaseAddress);
			detailPanel.Children().Append(sectionMaximumSize);
			detailPanel.Children().Append(sectionAttributes);
		}
		else if (item.Type() == L"Timer") {
			detailBox.Header(t(L"Monitor.Timer"));
			hstring state = item.TimerState() ? L"TRUE" : L"FALSE";
			auto timerRemainingTime = createSelectableTextBlock(t(L"Monitor.Label.RemainingTime") + to_hstring(item.TimerRemainingTime() * 100) + L"ns");
			auto timerState = createSelectableTextBlock(t(L"Monitor.Label.Signaled") + state);
			detailPanel.Children().Append(timerRemainingTime);
			detailPanel.Children().Append(timerState);
		}
		else if (item.Type() == L"IoCompletion") {
			detailBox.Header(t(L"Monitor.IoCompletion"));
			auto ioCompletionDepth = createSelectableTextBlock(t(L"Monitor.Label.Depth") + to_hstring(item.IoCompletionDepth()));
			detailPanel.Children().Append(ioCompletionDepth);
		}
		else {
			flag = true;
		}
		detailBox.Content(detailPanel);
		detailBox.Visibility(flag ? Visibility::Collapsed : Visibility::Visible);
		if (!status && !flag) {
			auto errorText = createSelectableTextBlock(t(L"Monitor.Msg.GetInfoError"));
			errorText.Foreground(Microsoft::UI::Xaml::Media::SolidColorBrush(Microsoft::UI::Colors::OrangeRed()));
			flyoutPanel.Children().Append(errorText);
		}

		flyoutPanel.Children().Append(basicInfoBox);
		flyoutPanel.Children().Append(referencesBox);
		flyoutPanel.Children().Append(quotaBox);
		flyoutPanel.Children().Append(detailBox);
		flyout.Content(flyoutPanel);

		FlyoutShowOptions options;
		options.ShowMode(FlyoutShowMode::Auto);
		options.Position(e.GetPosition(ObjectListView()));

		FlyoutHelper::SetAcrylicWorkaround(flyout, true);

		flyout.ShowAt(ObjectListView(), options);
	}

	void MonitorPage::CallbackListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = CallbackListView();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem()) return;

		auto item = listView.SelectedItem().as<winrt::StarlightGUI::GeneralEntry>();

		auto flyoutStyles = slg::GetStyles();

		MenuFlyout menuFlyout;

		// 选项1.1
		auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", t(L"Monitor.Menu.Remove").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) mutable -> winrt::Windows::Foundation::IAsyncAction {
			if (KernelInstance::RemoveNotify(item)) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});

		// 分割线1
		MenuFlyoutSeparator separator1;

		// 选项2.1
		auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
		auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", t(L"Common.Type").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String2().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub1);
		auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Module").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String1().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub2);
		auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", t(L"Monitor.Header.Entry").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String3().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub3);
		auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", t(L"Common.Handle").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String4().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub4);

		menuFlyout.Items().Append(item1_1);
		menuFlyout.Items().Append(separator1);
		menuFlyout.Items().Append(item2_1);

		slg::ShowAt(menuFlyout, listView, e);
	}

	void MonitorPage::MiniFilterListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = MiniFilterListView();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem()) return;

		auto item = listView.SelectedItem().as<winrt::StarlightGUI::GeneralEntry>();

		auto flyoutStyles = slg::GetStyles();

		MenuFlyout menuFlyout;

		// 选项1.1
		auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", t(L"Monitor.Menu.Unload").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) mutable -> winrt::Windows::Foundation::IAsyncAction {
			if (KernelInstance::RemoveMiniFilter(item)) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});

		// 分割线1
		MenuFlyoutSeparator separator1;

		// 选项2.1
		auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
		auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", L"IRP", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String2().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub1);
		auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Module").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String1().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub2);
		auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", t(L"Common.Base").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String3().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub3);
		auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", L"PreFilter", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String4().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub4);
		auto item2_1_sub5 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", L"PostFilter", [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String5().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub5);

		menuFlyout.Items().Append(item1_1);
		menuFlyout.Items().Append(separator1);
		menuFlyout.Items().Append(item2_1);

		slg::ShowAt(menuFlyout, listView, e);
	}

	void MonitorPage::StdFilterListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = StdFilterListView();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem()) return;

		auto item = listView.SelectedItem().as<winrt::StarlightGUI::GeneralEntry>();

		auto flyoutStyles = slg::GetStyles();

		MenuFlyout menuFlyout;

		// 选项1.1
		auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", t(L"Monitor.Menu.Unload").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) mutable -> winrt::Windows::Foundation::IAsyncAction {
			if (KernelInstance::RemoveStandardFilter(item)) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});

		// 分割线1
		MenuFlyoutSeparator separator1;

		// 选项2.1
		auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
		auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", t(L"Monitor.Menu.Driver").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String2().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub1);
		auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Module").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String3().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub2);
		auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ue97c", t(L"Common.Type").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String1().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub3);
		auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", t(L"Monitor.Menu.DeviceObj").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String4().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub4);
		auto item2_1_sub5 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", t(L"Monitor.Menu.TargetDriverObj").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String5().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub5);

		menuFlyout.Items().Append(item1_1);
		menuFlyout.Items().Append(separator1);
		menuFlyout.Items().Append(item2_1);

		slg::ShowAt(menuFlyout, listView, e);
	}

	void MonitorPage::SSDTListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = sender.as<ListView>();
		bool isSSDT = listView != SSSDTListView();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem()) return;

		auto item = listView.SelectedItem().as<winrt::StarlightGUI::GeneralEntry>();

		auto flyoutStyles = slg::GetStyles();

		MenuFlyout menuFlyout;

		// 选项1.1
		auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue75c", t(L"Monitor.Menu.Unhook").c_str(), [this, item, isSSDT](IInspectable const& sender, RoutedEventArgs const& e) mutable -> winrt::Windows::Foundation::IAsyncAction {
			if ((isSSDT && KernelInstance::UnhookSSDT(item)) || (!isSSDT && KernelInstance::UnhookSSSDT(item))) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		if (!item.ULongLong2() || (!item.Bool1() && item.ULongLong1() == item.ULongLong2())) item1_1.IsEnabled(false);

		// 选项1.2
		auto item1_2 = slg::CreateMenuItem(flyoutStyles, L"\ue72c", t(L"Monitor.Menu.ScanEPTHook").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) mutable -> winrt::Windows::Foundation::IAsyncAction {
			KernelInstance::EnableEPTScan();
			WaitAndReloadAsync(100);
			KernelInstance::DisableEPTScan();
			co_return;
			});

		// 分割线1
		MenuFlyoutSeparator separator1;

		// 选项2.1
		auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
		auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", t(L"Common.Name").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String2().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub1);
		auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Module").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String1().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub2);
		auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ue97c", t(L"Monitor.Menu.Hook").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String5().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub3);
		auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", t(L"Common.Address").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String3().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub4);
		auto item2_1_sub5 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", t(L"Monitor.Menu.SrcAddress").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String4().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub5);

		menuFlyout.Items().Append(item1_1);
		if (isSSDT) menuFlyout.Items().Append(item1_2);
		menuFlyout.Items().Append(separator1);
		menuFlyout.Items().Append(item2_1);

		slg::ShowAt(menuFlyout, listView, e);
	}

	void MonitorPage::ExCallbackListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = ExCallbackListView();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem()) return;

		auto item = listView.SelectedItem().as<winrt::StarlightGUI::GeneralEntry>();

		auto flyoutStyles = slg::GetStyles();

		MenuFlyout menuFlyout;

		// 选项1.1
		auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", t(L"Monitor.Menu.Remove").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) mutable -> winrt::Windows::Foundation::IAsyncAction {
			if (KernelInstance::RemoveExCallback(item)) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});

		// 分割线1
		MenuFlyoutSeparator separator1;

		// 选项2.1
		auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
		auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", t(L"Common.Name").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String1().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub1);
		auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Module").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String2().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub2);
		auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", t(L"Monitor.Menu.Entry").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String3().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub3);
		auto item2_1_sub4 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", t(L"Monitor.Menu.Object").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String4().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub4);
		auto item2_1_sub5 = slg::CreateMenuItem(flyoutStyles, L"\ueb1d", t(L"Common.Handle").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String5().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub5);

		menuFlyout.Items().Append(item1_1);
		menuFlyout.Items().Append(separator1);
		menuFlyout.Items().Append(item2_1);

		slg::ShowAt(menuFlyout, listView, e);
	}

	void MonitorPage::PiDDBListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = PiDDBListView();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem()) return;

		auto item = listView.SelectedItem().as<winrt::StarlightGUI::GeneralEntry>();

		auto flyoutStyles = slg::GetStyles();

		MenuFlyout menuFlyout;

		// 选项1.1
		auto item1_1 = slg::CreateMenuItem(flyoutStyles, L"\ue711", t(L"Monitor.Menu.Remove").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) mutable -> winrt::Windows::Foundation::IAsyncAction {
			if (KernelInstance::RemovePiDDBCache(item)) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.Failed", GetLastError()), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});

		// 分割线1
		MenuFlyoutSeparator separator1;

		// 选项2.1
		auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
		auto item2_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Module").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String1().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub1);
		auto item2_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uece9", t(L"Common.Status").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(std::to_wstring(item.ULong1()))) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub2);
		auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\uec92", t(L"Monitor.Timestamp").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(std::to_wstring(item.ULong2()))) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub3);

		menuFlyout.Items().Append(item1_1);
		menuFlyout.Items().Append(separator1);
		menuFlyout.Items().Append(item2_1);

		slg::ShowAt(menuFlyout, listView, e);
	}

	void MonitorPage::HALDPTListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = HALDPTListView();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem()) return;

		auto item = listView.SelectedItem().as<winrt::StarlightGUI::GeneralEntry>();

		auto flyoutStyles = slg::GetStyles();

		MenuFlyout menuFlyout;

		// 选项1.1
		auto item1_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
		auto item1_1_sub1 = slg::CreateMenuItem(flyoutStyles, L"\ue943", t(L"Common.Name").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String1().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item1_1.Items().Append(item1_1_sub1);
		auto item1_1_sub2 = slg::CreateMenuItem(flyoutStyles, L"\uec6c", t(L"Common.Module").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String2().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item1_1.Items().Append(item1_1_sub2);
		auto item1_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", t(L"Common.Address").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String3().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item1_1.Items().Append(item1_1_sub3);

		menuFlyout.Items().Append(item1_1);

		slg::ShowAt(menuFlyout, listView, e);
	}

	void MonitorPage::ObjectTreeView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e)
	{
		if (!IsLoaded() || segmentedIndex != 0) return;
		LoadObjectList();
	}

	winrt::Windows::Foundation::IAsyncAction MonitorPage::LoadItemList() {
		if (segmentedIndex != 0) co_return;
		m_itemList.Clear();

		std::sort(partitions.begin(), partitions.end(), [](auto a, auto b) {
			return LessIgnoreCase(a.Path().c_str(), b.Path().c_str());
			});

		for (const auto& partition : partitions) {
			WinUI3Package::SegmentedItem item;
			TextBlock textBlock;
			textBlock.Text(partition.Path());
			item.Content(textBlock);
			item.HorizontalContentAlignment(HorizontalAlignment::Left);
			m_itemList.Append(item);
		}
		co_return;
	}

	void MonitorPage::SearchBox_TextChanged(
        winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& e)
	{
		if (!IsLoaded()) return;

        auto searchBox = sender.try_as<AutoSuggestBox>();
        if (!searchBox) return;

        if (e.Reason() == AutoSuggestionBoxTextChangeReason::UserInput) {
            std::unordered_set<std::wstring> seen;
            auto suggestions = winrt::single_threaded_observable_vector<winrt::Windows::Foundation::IInspectable>();
            std::wstring lowerQuery = ToLowerCase(searchBox.Text().c_str());

            if (segmentedIndex == 0) {
                for (auto const& object : m_objectList) {
                    std::wstring name = object.Name().c_str();
                    if (name.empty()) continue;

                    std::wstring lowerName = ToLowerCase(name);
                    if (!lowerQuery.empty() && lowerName.find(lowerQuery) == std::wstring::npos) continue;
                    if (!seen.insert(lowerName).second) continue;

                    suggestions.Append(box_value(object.Name()));
                    if (suggestions.Size() >= 20) break;
                }
            }
            else if (segmentedIndex >= 2 && segmentedIndex <= 13) {
                for (auto const& entry : m_generalList) {
                    hstring key = GetMonitorSuggestionKey(segmentedIndex, entry);
                    std::wstring text = key.c_str();
                    if (text.empty()) continue;

                    std::wstring lowerText = ToLowerCase(text);
                    if (!lowerQuery.empty() && lowerText.find(lowerQuery) == std::wstring::npos) continue;
                    if (!seen.insert(lowerText).second) continue;

                    suggestions.Append(box_value(key));
                    if (suggestions.Size() >= 20) break;
                }
            }

            searchBox.ItemsSource(suggestions);
        }

		WaitAndReloadAsync(250);
	}

    void MonitorPage::SearchBox_SuggestionChosen(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxSuggestionChosenEventArgs const& e)
    {
        auto selected = e.SelectedItem().try_as<winrt::Windows::Foundation::IReference<winrt::hstring>>();
        hstring target = selected ? selected.Value() : unbox_value<hstring>(e.SelectedItem());
        if (target.empty()) return;

        SearchBox().Text(target);
    }

    void MonitorPage::SearchBox_QuerySubmitted(
        winrt::Windows::Foundation::IInspectable const&,
        winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& e)
    {
        (void)e;
    }

	bool MonitorPage::ApplyFilter(const hstring& target, const hstring& query) {
		return !ContainsIgnoreCase(target.c_str(), query.c_str());
	}

	slg::coroutine MonitorPage::RefreshButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		RefreshButton().IsEnabled(false);
		HandleSegmentedChange(segmentedIndex, true);
		RefreshButton().IsEnabled(true);
		co_return;
	}

	slg::coroutine MonitorPage::DbgViewButton_Click(IInspectable const&, RoutedEventArgs const&)
	{
		isDbgViewEnabled = !isDbgViewEnabled;
		DbgViewButton().Content(isDbgViewEnabled ? tbox(L"Monitor.Close") : tbox(L"Monitor.Open"));
		InitializeDbgView();
		co_return;
	}

	slg::coroutine MonitorPage::DbgViewGlobalCheckBox_Click(IInspectable const&, RoutedEventArgs const&)
	{
		isDbgViewGlobalEnabled = DbgViewGlobalCheckBox().IsChecked().GetBoolean();
		InitializeDbgView();
		co_return;
	}

	static DWORD DbgViewThread(bool global, DbgViewMonitor* m)
	{
		constexpr uint32_t DbgViewMaxLength = 200000;

		HANDLE& BufferReadyEvent = global ? m->GlobalBufferReadyEvent : m->LocalBufferReadyEvent;
		HANDLE& DataReadyEvent = global ? m->GlobalDataReadyEvent : m->LocalDataReadyEvent;
		PDBWIN_PAGE_BUFFER debugMessageBuffer = global ? m->GlobalDebugBuffer : m->LocalDebugBuffer;

		while (global ? m->GlobalCaptureEnabled : m->LocalCaptureEnabled)
		{
			SetEvent(BufferReadyEvent);

			DWORD status = WaitForSingleObject(DataReadyEvent, 1000);
			if (status != WAIT_OBJECT_0) {
				continue;
			}

			SYSTEMTIME st;
			GetLocalTime(&st);
			{
				std::lock_guard<std::mutex> guard(*m->Lock);
				wchar_t buffer[4096];
				swprintf_s(buffer, _countof(buffer), L"[%02d:%02d:%02d.%03d] [PID=%d] %hs\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, debugMessageBuffer->ProcessId, debugMessageBuffer->Buffer);
				*(m->Data) = *(m->Data) + to_hstring(buffer);
				uint32_t currentLength = static_cast<uint32_t>(m->Data->size());
				if (currentLength > DbgViewMaxLength) {
					std::wstring text = m->Data->c_str();
					text.erase(0, currentLength - DbgViewMaxLength);
					*(m->Data) = text.c_str();
				}
			}
		}

		LOG_INFO(__WFUNCTION__, L"Debug view thread exited!");

		return 0;
	}

	static DWORD WINAPI DbgViewLocalThread(PVOID param)
	{
		LOG_INFO(__WFUNCTION__, L"Thread created for local debug view!");
		return DbgViewThread(false, (DbgViewMonitor*)param);
	}

	static DWORD WINAPI DbgViewGlobalThread(PVOID param)
	{
		LOG_INFO(__WFUNCTION__, L"Thread created for global debug view!");
		return DbgViewThread(true, (DbgViewMonitor*)param);
	}

	winrt::Windows::Foundation::IAsyncAction MonitorPage::InitializeDbgView() {
		if (isDbgViewEnabled) {
			dbgViewMonitor.Init(false);
			HANDLE hThread = CreateThread(nullptr, 0, DbgViewLocalThread, &dbgViewMonitor, 0, nullptr);
			if (hThread) {
				CloseHandle(hThread);
			}
			if (isDbgViewGlobalEnabled) {
				dbgViewMonitor.Init(true);
				HANDLE hThread = CreateThread(nullptr, 0, DbgViewGlobalThread, &dbgViewMonitor, 0, nullptr);
				if (hThread) {
					CloseHandle(hThread);
				}
			}
		}
		else {
			dbgViewMonitor.UnInit(false);
			dbgViewMonitor.UnInit(true);
		}

		co_return;
	}

	winrt::Windows::Foundation::IAsyncAction MonitorPage::WaitAndReloadAsync(int interval) {
		auto lifetime = get_strong();
		auto requestVersion = ++m_reloadRequestVersion;

		co_await winrt::resume_after(std::chrono::milliseconds(interval));
		co_await wil::resume_foreground(DispatcherQueue());

		if (!IsLoaded() || requestVersion != m_reloadRequestVersion) co_return;
		RefreshButton_Click(nullptr, nullptr);

		co_return;
	}

	void MonitorPage::MainSegmented_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e)
	{
		if (!IsLoaded()) return;
		if (!MainSegmented().SelectedItem()) return;
		if (m_isLoading) {
			if (segmentedIndex != MainSegmented().SelectedIndex()) slg::CreateInfoBarAndDisplay(t(L"Common.Warning"), t(L"Monitor.Msg.WaitForLoading").c_str(), InfoBarSeverity::Warning, g_mainWindowInstance);
			MainSegmented().SelectedIndex(segmentedIndex);
			return;
		}
		HandleSegmentedChange(MainSegmented().SelectedIndex(), false);
	}

	slg::coroutine MonitorPage::HandleSegmentedChange(int index, bool force) {
		if (!IsLoaded() || m_isLoading) co_return;

		LOG_INFO(__WFUNCTION__, L"Handling segmented change: %d", index);

		LoadingRing().IsActive(true);

		auto weak_this = get_weak();
		int32_t previousObjectIndex = ObjectTreeView().SelectedIndex();
		segmentedIndex = index;

		m_itemList.Clear();
		m_objectList.Clear();
		m_generalList.Clear();
		windbgTimer.Stop();

		DefaultText().Visibility(Visibility::Collapsed);
		ObjectGrid().Visibility(Visibility::Collapsed);
		DbgViewGrid().Visibility(Visibility::Collapsed);
		CallbackGrid().Visibility(Visibility::Collapsed);
		MiniFilterGrid().Visibility(Visibility::Collapsed);
		StdFilterGrid().Visibility(Visibility::Collapsed);
		SSDTGrid().Visibility(Visibility::Collapsed);
		SSSDTGrid().Visibility(Visibility::Collapsed);
		IoTimerGrid().Visibility(Visibility::Collapsed);
		ExCallbackGrid().Visibility(Visibility::Collapsed);
		IDTGrid().Visibility(Visibility::Collapsed);
		GDTGrid().Visibility(Visibility::Collapsed);
		PiDDBGrid().Visibility(Visibility::Collapsed);
		HALDPTGrid().Visibility(Visibility::Collapsed);
		HALPDPTGrid().Visibility(Visibility::Collapsed);
		switch (index) {
		case 0: {
			ObjectGrid().Visibility(Visibility::Visible);
			if (force || partitions.empty()) {
				partitions.clear();
				winrt::StarlightGUI::ObjectEntry root = winrt::make<winrt::StarlightGUI::implementation::ObjectEntry>();
				root.Name(L"\\");
				root.Type(L"Directory");
				root.Path(L"\\");
				partitions.push_back(root);
				co_await LoadPartitionList(L"\\");
			}
			co_await LoadItemList();
			if (m_itemList.Size() > 0) {
				int32_t safeIndex = previousObjectIndex >= 0 && previousObjectIndex < static_cast<int32_t>(m_itemList.Size()) ? previousObjectIndex : 0;
				ObjectTreeView().SelectedIndex(safeIndex);
				co_await LoadObjectList();
			}
			break;
		}
		case 1: {
			DbgViewGrid().Visibility(Visibility::Visible);
			DbgViewButton().Content(isDbgViewEnabled ? tbox(L"Monitor.Close") : tbox(L"Monitor.Open"));
			DbgViewGlobalCheckBox().IsChecked(isDbgViewGlobalEnabled);
			{
				std::lock_guard<std::mutex> guard(dbgViewMutex);
				DbgViewBox().Text(dbgViewData);
				m_lastDbgViewLength = static_cast<uint32_t>(dbgViewData.size());
			}
			windbgTimer.Start();
			break;
		}
		case 2: {
			CallbackGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 3: {
			MiniFilterGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 4: {
			StdFilterGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 5: {
			SSDTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 6: {
			SSSDTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 7: {
			IoTimerGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 8: {
			ExCallbackGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 9: {
			IDTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 10: {
			GDTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 11: {
			PiDDBGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 12: {
			HALDPTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 13: {
			HALPDPTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		}

		if (auto strong_this = weak_this.get()) {
			LoadingRing().IsActive(false);
		}
		co_return;
	}

	void MonitorPage::SetupLocalization() {
		MonitorSegObjectUid().Text(t(L"Monitor.Seg.Object"));
		MonitorSegDebugUid().Text(t(L"Monitor.Seg.Debug"));
		MonitorSegCallbackUid().Text(t(L"Monitor.Seg.Callback"));
		MonitorSegMiniFilterUid().Text(t(L"Monitor.Seg.MiniFilter"));
		MonitorSegStdFilterUid().Text(t(L"Monitor.Seg.StdFilter"));
		MonitorSegSSDTUid().Text(t(L"Monitor.Seg.SSDT"));
		MonitorSegSSSDTUid().Text(t(L"Monitor.Seg.SSSDT"));
		MonitorSegIOTimerUid().Text(t(L"Monitor.Seg.IOTimer"));
		MonitorSegExCallbackUid().Text(t(L"Monitor.Seg.ExCallback"));
		MonitorSegIDTUid().Text(t(L"Monitor.Seg.IDT"));
		MonitorSegGDTUid().Text(t(L"Monitor.Seg.GDT"));
		MonitorSegPiDDBUid().Text(t(L"Monitor.Seg.PiDDB"));
		MonitorSegHALDPTUid().Text(t(L"Monitor.Seg.HALDPT"));
		MonitorSegHALPDPTUid().Text(t(L"Monitor.Seg.HALPDPT"));
		SearchBox().PlaceholderText(t(L"Monitor.Placeholder"));
		DefaultText().Text(t(L"Monitor.DefaultText"));
		RefreshButton().Label(t(L"Common.Refresh"));
		DbgViewButton().Content(tbox(L"Monitor.Open"));
		DbgViewGlobalCheckBox().Content(tbox(L"Monitor.CaptureGlobal"));
		ObjectNameHeaderButton().Content(tbox(L"Common.Name"));
		ObjectTypeHeaderButton().Content(tbox(L"Common.Type"));
		CallbackTypeModuleHeaderButton().Content(tbox(L"Monitor.Header.TypeModule"));
		CallbackEntryHeaderButton().Content(tbox(L"Monitor.Header.Entry"));
		CallbackHandleHeaderButton().Content(tbox(L"Common.Handle"));
		MiniFilterIRPModuleHeaderButton().Content(tbox(L"Monitor.Header.IRPModule"));
		MiniFilterBaseHeaderButton().Content(tbox(L"Common.Base"));
		MiniFilterPreFilterHeaderButton().Content(tbox(L"Monitor.Header.PreFilter"));
		MiniFilterPostFilterHeaderButton().Content(tbox(L"Monitor.Header.PostFilter"));
		StdFilterDriverModuleHeaderButton().Content(tbox(L"Monitor.Header.DriverModule"));
		StdFilterTypeHeaderButton().Content(tbox(L"Common.Type"));
		StdFilterDeviceObjectHeaderButton().Content(tbox(L"Monitor.Header.DeviceObject"));
		StdFilterTargetDriverObjectHeaderButton().Content(tbox(L"Monitor.Header.TargetDriverObject"));
		SSDTNameModuleHeaderButton().Content(tbox(L"Monitor.Header.NameModule"));
		SSDTHookHeaderButton().Content(tbox(L"Monitor.Header.Hook"));
		SSDTAddressHeaderButton().Content(tbox(L"Common.Address"));
		SSDTSourceAddressHeaderButton().Content(tbox(L"Monitor.Header.SourceAddress"));
		SSDTIndexHeaderButton().Content(tbox(L"Monitor.Header.Index"));
		SSSDTNameModuleHeaderButton().Content(tbox(L"Monitor.Header.NameModule"));
		SSSDTHookHeaderButton().Content(tbox(L"Monitor.Header.Hook"));
		SSSDTAddressHeaderButton().Content(tbox(L"Common.Address"));
		SSSDTSourceAddressHeaderButton().Content(tbox(L"Monitor.Header.SourceAddress"));
		SSSDTIndexHeaderButton().Content(tbox(L"Monitor.Header.Index"));
		IoTimerModuleHeaderButton().Content(tbox(L"Common.Module"));
		IoTimerAddressHeaderButton().Content(tbox(L"Common.Address"));
		IoTimerIndexHeaderButton().Content(tbox(L"Monitor.Header.Index"));
		ExCallbackNameModuleHeaderButton().Content(tbox(L"Monitor.Header.NameModule"));
		ExCallbackEntryHeaderButton().Content(tbox(L"Monitor.Header.Entry"));
		ExCallbackObjectHeaderButton().Content(tbox(L"Monitor.Header.Object"));
		ExCallbackHandleHeaderButton().Content(tbox(L"Common.Handle"));
		IDTNameHeaderButton().Content(tbox(L"Common.Name"));
		IDTBaseHeaderButton().Content(tbox(L"Common.Base"));
		IDTLimitHeaderButton().Content(tbox(L"Monitor.Header.Limit"));
		IDTPrivilegeHeaderButton().Content(tbox(L"Monitor.Header.Privilege"));
		IDTAccessHeaderButton().Content(tbox(L"Monitor.Header.Access"));
		PiDDBModuleHeaderButton().Content(tbox(L"Common.Module"));
		PiDDBStatusHeaderButton().Content(tbox(L"Common.Status"));
		PiDDBTimestampHeaderButton().Content(tbox(L"Monitor.Header.Timestamp"));
		HALDPTNameModuleHeaderButton().Content(tbox(L"Monitor.Header.NameModule"));
		HALDPTAddressHeaderButton().Content(tbox(L"Common.Address"));
		HALPDPTNameModuleHeaderButton().Content(tbox(L"Monitor.Header.NameModule"));
		HALPDPTAddressHeaderButton().Content(tbox(L"Common.Address"));
	}
}


