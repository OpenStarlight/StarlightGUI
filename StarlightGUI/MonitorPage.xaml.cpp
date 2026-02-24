#include "pch.h"
#include "MonitorPage.xaml.h"
#if __has_include("MonitorPage.g.cpp")
#include "MonitorPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace WinUI3Package;

namespace winrt::StarlightGUI::implementation
{
	static std::vector<SegmentedItem> items;
	static std::vector<winrt::StarlightGUI::ObjectEntry> partitions;

	MonitorPage::MonitorPage() {
		InitializeComponent();

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

		if (!list_animation) {
			ObjectListView().ItemContainerTransitions().Clear();
			CallbackListView().ItemContainerTransitions().Clear();
			MiniFilterListView().ItemContainerTransitions().Clear();
			StdFilterListView().ItemContainerTransitions().Clear();
			SSDTListView().ItemContainerTransitions().Clear();
			SSSDTListView().ItemContainerTransitions().Clear();
			IoTimerListView().ItemContainerTransitions().Clear();
			ExCallbackListView().ItemContainerTransitions().Clear();
			IDTListView().ItemContainerTransitions().Clear();
			GDTListView().ItemContainerTransitions().Clear();
			PiDDBListView().ItemContainerTransitions().Clear();
			HALDPTListView().ItemContainerTransitions().Clear();
			HALPDPTListView().ItemContainerTransitions().Clear();
		}

		Loaded([this](auto&&, auto&&) {
			if (!KernelInstance::IsRunningAsAdmin()) {
				CallbackSegmentedItem().IsEnabled(false);
				MiniFilterSegmentedItem().IsEnabled(false);
				StdFilterSegmentedItem().IsEnabled(false);
				SSDTSegmentedItem().IsEnabled(false);
				SSSDTSegmentedItem().IsEnabled(false);
				IoTimerSegmentedItem().IsEnabled(false);
				ExCallbackSegmentedItem().IsEnabled(false);
				IDTSegmentedItem().IsEnabled(false);
				GDTSegmentedItem().IsEnabled(false);
				PiDDBSegmentedItem().IsEnabled(false);
				HALDPTSegmentedItem().IsEnabled(false);
				HALPDPTSegmentedItem().IsEnabled(false);
			}
			});

		Unloaded([this](auto&&, auto&&) {
			windbgTimer.Stop();
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

		co_await winrt::resume_background();

		std::vector<winrt::StarlightGUI::ObjectEntry> objects;

		// 获取对象逻辑
		winrt::StarlightGUI::ObjectEntry& selectedPartition = partitions[index];
		KernelInstance::EnumObjectsByDirectory(selectedPartition.Path().c_str(), objects);

		co_await wil::resume_foreground(DispatcherQueue());

		m_objectList.Clear();
		for (const auto& object : objects) {
			bool shouldRemove = query.empty() ? false : ApplyFilter(object.Name(), query);
			if (shouldRemove) continue;

			if (object.Name().empty()) object.Name(L"(未知)");
			if (object.Type().empty()) object.Type(L"(未知)");
			if (object.CreationTime().empty()) object.CreationTime(L"(未知)");
			if (!object.Link().empty()) object.Path(object.Link());

			m_objectList.Append(object);
		}

		LOG_INFO(__WFUNCTION__, L"Loaded object list, %d entry(s) in total.", m_objectList.Size());
		m_isLoading = false;
	}

	winrt::Windows::Foundation::IAsyncAction MonitorPage::LoadGeneralList(bool force) {
		if (m_isLoading || !KernelInstance::IsRunningAsAdmin()) {
			co_return;
		}
		m_isLoading = true;

		LOG_INFO(__WFUNCTION__, L"Loading general list...");

		auto requestedIndex = segmentedIndex;
		auto lifetime = get_strong();
		hstring query = SearchBox().Text();

		co_await winrt::resume_background();

		std::vector<winrt::StarlightGUI::GeneralEntry> entries;

		static std::vector<winrt::StarlightGUI::GeneralEntry> callbackCache, minifilterCache, standardfilterCache, ssdtCache, sssdtCache, ioTimerCache, exCallbackCache, idtCache, gdtCache, piddbCache, halDptCache, halPdptCache;

		switch (requestedIndex) {
		case 2:
			if (force || callbackCache.empty()) {
				KernelInstance::EnumCallbacks(entries);
				callbackCache = entries;
			}
			else entries = callbackCache;
			break;
		case 3:
			if (force || minifilterCache.empty()) {
				KernelInstance::EnumMiniFilter(entries);
				minifilterCache = entries;
			}
			else entries = minifilterCache;
			break;
		case 4:
			if (force || standardfilterCache.empty()) {
				KernelInstance::EnumStandardFilter(entries);
				standardfilterCache = entries;
			}
			else entries = standardfilterCache;
			break;
		case 5:
			if (force || ssdtCache.empty()) {
				KernelInstance::EnumSSDT(entries);
				ssdtCache = entries;
			}
			else entries = ssdtCache;
			break;
		case 6:
			if (force || sssdtCache.empty()) {
				KernelInstance::EnumSSSDT(entries);
				sssdtCache = entries;
			}
			else entries = sssdtCache;
			break;
		case 7:
			if (force || ioTimerCache.empty()) {
				KernelInstance::EnumIoTimer(entries);
				ioTimerCache = entries;
			}
			else entries = ioTimerCache;
			break;
		case 8:
			if (force || exCallbackCache.empty()) {
				KernelInstance::EnumExCallback(entries);
				exCallbackCache = entries;
			}
			else entries = exCallbackCache;
			break;
		case 9:
			if (force || idtCache.empty()) {
				KernelInstance::EnumIDT(entries);
				idtCache = entries;
			}
			else entries = idtCache;
			break;
		case 10:
			if (force || gdtCache.empty()) {
				KernelInstance::EnumGDT(entries);
				gdtCache = entries;
			}
			else entries = gdtCache;
			break;
		case 11:
			if (force || piddbCache.empty()) {
				KernelInstance::EnumPiDDBCacheTable(entries);
				piddbCache = entries;
			}
			else entries = piddbCache;
			break;
		case 12:
			if (force || halDptCache.empty()) {
				KernelInstance::EnumHalDispatchTable(entries);
				halDptCache = entries;
			}
			else entries = halDptCache;
			break;
		case 13:
			if (force || halPdptCache.empty()) {
				KernelInstance::EnumHalPrivateDispatchTable(entries);
				halPdptCache = entries;
			}
			else entries = halPdptCache;
			break;
		}

		co_await wil::resume_foreground(DispatcherQueue());

		// 防止意外
		if (requestedIndex != segmentedIndex) {
			m_isLoading = false;
			co_return;
		}

		m_generalList.Clear();
		for (const auto& entry : entries) {
			bool shouldRemove = false;
			if (!query.empty()) {
				switch (requestedIndex) {
				case 2:
				case 3:
				case 5:
				case 6:
				case 8:
				case 12:
				case 13:
					shouldRemove = ApplyFilter(entry.String1(), query) && ApplyFilter(entry.String2(), query);
					break;
				case 4:
					shouldRemove = ApplyFilter(entry.String2(), query) && ApplyFilter(entry.String3(), query);
					break;
				case 7:
				case 9:
				case 10:
				case 11:
					shouldRemove = ApplyFilter(entry.String1(), query);
					break;
				}
			}
			if (shouldRemove) continue;

			if (entry.String1().empty()) entry.String1(L"(未知)");
			if (entry.String2().empty()) entry.String2(L"(未知)");
			if (entry.String3().empty()) entry.String3(L"(未知)");
			if (entry.String4().empty()) entry.String4(L"(未知)");
			if (entry.String5().empty()) entry.String5(L"(未知)");
			if (entry.String6().empty()) entry.String6(L"(未知)");

			m_generalList.Append(entry);
		}

		LOG_INFO(__WFUNCTION__, L"Loaded general list, %d entry(s) in total.", m_generalList.Size());
		m_isLoading = false;
	}

	void MonitorPage::ObjectListView_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
	{
		if (!ObjectListView().SelectedItem() || segmentedIndex != 0) return;
		auto item = ObjectListView().SelectedItem().as<winrt::StarlightGUI::ObjectEntry>();

		// 获取信息
		BOOL status = KernelInstance::GetObjectDetails(item.Path().c_str(), item.Type().c_str(), item);

		Flyout flyout;
		StackPanel flyoutPanel;

		// 基本信息
		GroupBox basicInfoBox;
		StackPanel basicInfoPanel;
		basicInfoBox.Header(box_value(L"基本信息"));
		basicInfoBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
		TextBlock name;
		name.Text(L"名称: " + item.Name());
		TextBlock type;
		type.Text(L"类型: " + item.Type());
		TextBlock path;
		path.Text(L"完整路径: " + item.Path());
		CheckBox permanent;
		permanent.Content(box_value(L"永久"));
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
		referencesBox.Header(box_value(L"引用信息"));
		referencesBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
		TextBlock references;
		references.Text(L"引用: " + std::to_wstring(item.References()));
		TextBlock handles;
		handles.Text(L"句柄: " + std::to_wstring(item.Handles()));
		referencesPanel.Children().Append(references);
		referencesPanel.Children().Append(handles);
		referencesBox.Content(referencesPanel);

		// 配额信息
		GroupBox quotaBox;
		StackPanel quotaPanel;
		quotaBox.Header(box_value(L"配额信息"));
		quotaBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
		TextBlock paged;
		paged.Text(L"分页池: " + FormatMemorySize(item.PagedPool()));
		TextBlock nonPaged;
		nonPaged.Text(L"非分页池: " + FormatMemorySize(item.NonPagedPool()));
		quotaPanel.Children().Append(paged);
		quotaPanel.Children().Append(nonPaged);
		quotaBox.Content(quotaPanel);

		// 详细信息
		bool flag = false;
		GroupBox detailBox;
		StackPanel detailPanel;
		detailBox.Margin(ThicknessHelper::FromLengths(0, 0, 0, 10));
		if (item.Type() == L"SymbolicLink") {
			detailBox.Header(box_value(L"符号链接"));
			TextBlock creationTime;
			creationTime.Text(L"创建时间: " + item.CreationTime());
			TextBlock linkTarget;
			linkTarget.Text(L"链接: " + item.Link());
			detailPanel.Children().Append(creationTime);
			detailPanel.Children().Append(linkTarget);
		}
		else if (item.Type() == L"Event") {
			detailBox.Header(box_value(L"事件"));
			TextBlock eventType;
			eventType.Text(L"事件类型: " + item.EventType());
			TextBlock eventSignaled;
			hstring state = item.EventSignaled() ? L"TRUE" : L"FALSE";
			eventSignaled.Text(L"触发: " + state);
			detailPanel.Children().Append(eventType);
			detailPanel.Children().Append(eventSignaled);
		}
		else if (item.Type() == L"Mutant") {
			detailBox.Header(box_value(L"互斥体"));
			TextBlock mutantHoldCount;
			mutantHoldCount.Text(L"持有数: " + to_hstring(item.MutantHoldCount()));
			TextBlock mutantAbandoned;
			hstring state = item.MutantAbandoned() ? L"TRUE" : L"FALSE";
			mutantAbandoned.Text(L"遗弃: " + state);
			detailPanel.Children().Append(mutantHoldCount);
			detailPanel.Children().Append(mutantAbandoned);
		}
		else if (item.Type() == L"Semaphore") {
			detailBox.Header(box_value(L"信号量"));
			TextBlock semaphoreCount;
			semaphoreCount.Text(L"当前量: " + to_hstring(item.SemaphoreCount()));
			TextBlock semaphoreLimit;
			semaphoreLimit.Text(L"最大量: " + to_hstring(item.SemaphoreLimit()));
			detailPanel.Children().Append(semaphoreCount);
			detailPanel.Children().Append(semaphoreLimit);
		}
		else if (item.Type() == L"Section") {
			detailBox.Header(box_value(L"区域"));
			TextBlock sectionBaseAddress;
			sectionBaseAddress.Text(L"基址: " + ULongToHexString(item.SectionBaseAddress()));
			TextBlock sectionMaximumSize;
			sectionMaximumSize.Text(L"大小: " + FormatMemorySize(item.SectionMaximumSize()));
			TextBlock sectionAttributes;
			hstring attr = item.SectionAttributes() == 0x200000 ? L"SEC_BASED" : item.SectionAttributes() == 0x800000 ? L"SEC_FILE" : item.SectionAttributes() == 0x4000000
				? L"SEC_RESERVE" : item.SectionAttributes() == 0x8000000 ? L"SEC_COMMIT" : item.SectionAttributes() == 0x1000000 ? L"SEC_IMAGE" : L"NULL";
			sectionAttributes.Text(L"属性: " + attr);
			detailPanel.Children().Append(sectionBaseAddress);
			detailPanel.Children().Append(sectionMaximumSize);
			detailPanel.Children().Append(sectionAttributes);
		}
		else if (item.Type() == L"Timer") {
			detailBox.Header(box_value(L"计时器"));
			TextBlock timerRemainingTime;
			timerRemainingTime.Text(L"剩余时间: " + to_hstring(item.TimerRemainingTime() * 100) + L"ns");
			TextBlock timerState;
			hstring state = item.TimerState() ? L"TRUE" : L"FALSE";
			timerState.Text(L"触发: " + state);
			detailPanel.Children().Append(timerRemainingTime);
			detailPanel.Children().Append(timerState);
		}
		else if (item.Type() == L"IoCompletion") {
			detailBox.Header(box_value(L"I/O 完成端口"));
			TextBlock ioCompletionDepth;
			ioCompletionDepth.Text(L"深度: " + to_hstring(item.IoCompletionDepth()));
			detailPanel.Children().Append(ioCompletionDepth);
		}
		else {
			flag = true;
		}
		detailBox.Content(detailPanel);
		detailBox.Visibility(flag ? Visibility::Collapsed : Visibility::Visible);
		if (!status && !flag) {
			TextBlock errorText;
			errorText.Text(L"获取信息时出错，部分内容可能不完整！");
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

    void MonitorPage::MonitorListView_ContainerContentChanging(
        winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
        winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args)
    {
        if (args.InRecycleQueue())
            return;

        // 将 Tag 设到容器上，便于 ListViewItemPresenter 通过 TemplatedParent 绑定
        if (auto itemContainer = args.ItemContainer())
            itemContainer.Tag(sender.Tag());
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
			std::wstring aName = a.Path().c_str();
			std::wstring bName = b.Path().c_str();
			std::transform(aName.begin(), aName.end(), aName.begin(), ::towlower);
			std::transform(bName.begin(), bName.end(), bName.begin(), ::towlower);

			return aName < bName;
			});

		for (const auto& partition : partitions) {
			SegmentedItem item;
			TextBlock textBlock;
			textBlock.Text(partition.Path());
			item.Content(textBlock);
			item.HorizontalContentAlignment(HorizontalAlignment::Left);
			items.push_back(item);
		}

		for (const auto& item : items) {
			m_itemList.Append(item);
		}
		co_return;
	}

	void MonitorPage::SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		WaitAndReloadAsync(200);
	}

	bool MonitorPage::ApplyFilter(const hstring& target, const hstring& query) {
		std::wstring name = target.c_str();
		std::wstring queryWStr = query.c_str();

		// 不比较大小写
		std::transform(name.begin(), name.end(), name.begin(), ::towlower);
		std::transform(queryWStr.begin(), queryWStr.end(), queryWStr.begin(), ::towlower);

		bool result = name.find(queryWStr) == std::wstring::npos;

		return result;
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
		DbgViewButton().Content(isDbgViewEnabled ? box_value(L"关闭") : box_value(L"开启"));
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

		reloadTimer.Stop();
		reloadTimer.Interval(std::chrono::milliseconds(interval));
		reloadTimer.Tick([this](auto&&, auto&&) {
			RefreshButton_Click(nullptr, nullptr);
			reloadTimer.Stop();
			});
		reloadTimer.Start();

		co_return;
	}

	void MonitorPage::MainSegmented_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e)
	{
		if (!IsLoaded()) return;
		if (!MainSegmented().SelectedItem()) return;
		if (m_isLoading) {
			if (segmentedIndex != MainSegmented().SelectedIndex()) CreateInfoBarAndDisplay(L"警告", L"请等待列表加载完毕后再进行切换！", InfoBarSeverity::Warning, g_mainWindowInstance);
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
		segmentedIndex = index;

		// 清除列表以防止潜在的内存占用
		items.clear();
		partitions.clear();
		items.shrink_to_fit();
		partitions.shrink_to_fit();
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
			winrt::StarlightGUI::ObjectEntry root = winrt::make<winrt::StarlightGUI::implementation::ObjectEntry>();
			root.Name(L"\\");
			root.Type(L"Directory");
			root.Path(L"\\");
			partitions.push_back(root);
			co_await LoadPartitionList(L"\\");
			co_await LoadItemList();
			co_await LoadObjectList();
			ObjectTreeView().SelectedIndex(0);
			break;
		}
		case 1: {
			DbgViewGrid().Visibility(Visibility::Visible);
			DbgViewButton().Content(isDbgViewEnabled ? box_value(L"关闭") : box_value(L"开启"));
			DbgViewGlobalCheckBox().IsChecked(isDbgViewGlobalEnabled);
			windbgTimer.Interval(std::chrono::seconds(1));
			windbgTimer.Tick([this](auto&&, auto&&) {
				std::lock_guard<std::mutex> guard(dbgViewMutex);
				DbgViewBox().Text(dbgViewData);
				});
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
}
