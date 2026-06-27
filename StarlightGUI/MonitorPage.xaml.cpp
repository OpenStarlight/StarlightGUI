#include "pch.h"
#include "MonitorPage.xaml.h"
#if __has_include("MonitorPage.g.cpp")
#include "MonitorPage.g.cpp"
#endif
#include <array>
#include <string_view>
#include <utility>
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

	static hstring GetDriverErrorMessage()
	{
		auto errorMsg = KernelInstance::GetLastErrorMessage();
		if (errorMsg.empty()) {
			auto errorCode = KernelInstance::GetLastErrorCode();
			wchar_t hexCode[32];
			swprintf_s(hexCode, L"0x%X", errorCode);
			return t(L"Msg.DriverError.Code", hexCode);
		}
		return t(L"Msg.DriverError.Detail", errorMsg.c_str());
	}

	static constexpr std::array<std::pair<CallbackType, std::wstring_view>, 19> CallbackTypeOptions = {
		std::pair{ CallbackType::CreateProcess, L"CreateProcess" },
		std::pair{ CallbackType::CreateThread, L"CreateThread" },
		std::pair{ CallbackType::LoadImage, L"LoadImage" },
		std::pair{ CallbackType::Object, L"Object" },
		std::pair{ CallbackType::Registry, L"Registry" },
		std::pair{ CallbackType::PowerSetting, L"PowerSetting" },
		std::pair{ CallbackType::PlugPlay, L"PlugPlay" },
		std::pair{ CallbackType::Shutdown, L"Shutdown" },
		std::pair{ CallbackType::LastChanceShutdown, L"LastChanceShutdown" },
		std::pair{ CallbackType::FileSystemChange, L"FileSystemChange" },
		std::pair{ CallbackType::BugCheck, L"BugCheck" },
		std::pair{ CallbackType::BugCheckReason, L"BugCheckReason" },
		std::pair{ CallbackType::ExCallback, L"ExCallback" },
		std::pair{ CallbackType::LogonSessionTerminated, L"LogonSessionTerminated" },
		std::pair{ CallbackType::LogonSessionTerminatedEx, L"LogonSessionTerminatedEx" },
		std::pair{ CallbackType::DbgPrint, L"DbgPrint" },
		std::pair{ CallbackType::IoPriority, L"IoPriority" },
		std::pair{ CallbackType::Coalescing, L"Coalescing" },
		std::pair{ CallbackType::Nmi, L"Nmi" }
	};

	static constexpr std::array<std::wstring_view, 5> HALTableNames = {
		L"HalDispatchTable", L"HalPrivateDispatchTable", L"HalIommuDispatchTable",
		L"HalAcpiDispatchTable", L"HalSubComponents"
	};

	MonitorPage::MonitorPage() {
		InitializeComponent();
		SetupLocalization();
		InitializeFlyout();

		// 初始化所有列表
		{
			ObjectTreeView().ItemsSource(m_itemList);
			ObjectListView().ItemsSource(m_objectList);
			CallbackListView().ItemsSource(m_generalList);
			MiniFilterListView().ItemsSource(m_generalList);
			SSDTListView().ItemsSource(m_generalList);
			SSSDTListView().ItemsSource(m_generalList);
			IoTimerListView().ItemsSource(m_generalList);
			IDTListView().ItemsSource(m_generalList);
			GDTListView().ItemsSource(m_generalList);
			PiDDBListView().ItemsSource(m_generalList);
			HALTableListView().ItemsSource(m_generalList);
		}
		UpdateCallbackColumns();
        InitializeColumnSyncBindings();

		Unloaded([this](auto&&, auto&&) {
			++m_reloadRequestVersion;
			});

		LOG_INFO(L"MonitorPage", L"MonitorPage initialized.");
	}

	winrt::Windows::Foundation::IAsyncAction MonitorPage::LoadPartitionList(std::wstring path, bool reportError) {
		if (segmentedIndex != 0) co_return;

		std::vector<winrt::StarlightGUI::ObjectEntry> partitionsInPath;

		if (!KernelInstance::SiEnumObjectsByDirectory(path, partitionsInPath)) {
			if (reportError) slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
		}

		for (const auto& object : partitionsInPath) {
			if (object.Type() == L"Directory") {
				partitions.push_back(object);
				co_await LoadPartitionList(object.Path().c_str(), false);
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
		if (!KernelInstance::SiEnumObjectsByDirectory(selectedPartition.Path().c_str(), objects)) {
			co_await wil::resume_foreground(DispatcherQueue());
			slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
			m_isLoading = false;
			co_return;
		}

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

		if (m_callbackType < 0 || m_callbackType > static_cast<int>(CallbackType::Nmi) || (CallbackType)m_callbackType == CallbackType::ImageVerification) {
			m_callbackType = static_cast<int>(CallbackType::CreateProcess);
			UpdateCallbackColumns();
		}

		auto requestedIndex = segmentedIndex;
		auto requestedCallbackType = m_callbackType;
		auto requestedHALTableType = m_halTableType;
		auto lifetime = get_strong();
		hstring query = SearchBox().Text();
		std::wstring lowerQuery;
		if (!query.empty()) lowerQuery = ToLowerCase(query.c_str());

		co_await winrt::resume_background();

		std::vector<winrt::StarlightGUI::GeneralEntry> entries;
		std::vector<winrt::StarlightGUI::GeneralEntry> const* entriesSource = &entries;

		static std::array<std::vector<winrt::StarlightGUI::GeneralEntry>, static_cast<size_t>(CallbackType::Nmi) + 1> callbackCache;
		static std::array<std::vector<winrt::StarlightGUI::GeneralEntry>, HALTableNames.size()> halCache;
		static std::vector<winrt::StarlightGUI::GeneralEntry> minifilterCache, ssdtCache, sssdtCache, ioTimerCache, idtCache, gdtCache, piddbCache;

		switch (requestedIndex) {
		case 1:
			if (requestedCallbackType < 0 || requestedCallbackType >= static_cast<int>(callbackCache.size()) || (CallbackType)requestedCallbackType == CallbackType::ImageVerification) {
				requestedCallbackType = 0;
			}
			if (force || callbackCache[requestedCallbackType].empty()) {
				if (!KernelInstance::SiEnumCallbacks(entries, (CallbackType)requestedCallbackType)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				callbackCache[requestedCallbackType] = entries;
				entriesSource = &entries;
			}
			else entriesSource = &callbackCache[requestedCallbackType];
			break;
		case 2:
			if (force || minifilterCache.empty()) {
				if (!KernelInstance::SiEnumMiniFilter(entries)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				minifilterCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &minifilterCache;
			break;
		case 3:
			if (force || ssdtCache.empty()) {
				if (!KernelInstance::SiEnumSSDT(entries)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				ssdtCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &ssdtCache;
			break;
		case 4:
			if (force || sssdtCache.empty()) {
				if (!KernelInstance::SiEnumSSSDT(entries)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				sssdtCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &sssdtCache;
			break;
		case 5:
			if (force || ioTimerCache.empty()) {
				if (!KernelInstance::SiEnumIoTimer(entries)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				ioTimerCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &ioTimerCache;
			break;
		case 6:
			if (force || idtCache.empty()) {
				if (!KernelInstance::SiEnumIDT(entries)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				idtCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &idtCache;
			break;
		case 7:
			if (force || gdtCache.empty()) {
				if (!KernelInstance::SiEnumGDT(entries)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				gdtCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &gdtCache;
			break;
		case 8:
			if (force || piddbCache.empty()) {
				if (!KernelInstance::SiEnumPiDDBCacheTable(entries)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				piddbCache = entries;
				entriesSource = &entries;
			}
			else entriesSource = &piddbCache;
			break;
		case 9:
			if (requestedHALTableType < 0 ||
				requestedHALTableType >= static_cast<int>(halCache.size())) {
				requestedHALTableType = 0;
			}
			if (force || halCache[requestedHALTableType].empty()) {
				if (!KernelInstance::SiEnumHalDispatchTable(entries, (HalTableType)requestedHALTableType)) {
					co_await wil::resume_foreground(DispatcherQueue());
					slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
					m_isLoading = false;
					co_return;
				}
				halCache[requestedHALTableType] = entries;
				entriesSource = &entries;
			}
			else entriesSource = &halCache[requestedHALTableType];
			break;
		}

		co_await wil::resume_foreground(DispatcherQueue());

		// 防止意外
		if (requestedIndex != segmentedIndex ||
			(requestedIndex == 1 && requestedCallbackType != m_callbackType) ||
			(requestedIndex == 9 && requestedHALTableType != m_halTableType)) {
			m_isLoading = false;
			co_return;
		}

		m_generalList.Clear();
		for (const auto& entry : *entriesSource) {
			bool shouldRemove = false;
			if (!lowerQuery.empty()) {
				shouldRemove = !ContainsIgnoreCaseLowerQuery(entry.String1().c_str(), lowerQuery) &&
					!ContainsIgnoreCaseLowerQuery(entry.String2().c_str(), lowerQuery) &&
					!ContainsIgnoreCaseLowerQuery(entry.String3().c_str(), lowerQuery) &&
					!ContainsIgnoreCaseLowerQuery(entry.String4().c_str(), lowerQuery) &&
					!ContainsIgnoreCaseLowerQuery(entry.String5().c_str(), lowerQuery) &&
					!ContainsIgnoreCaseLowerQuery(entry.String6().c_str(), lowerQuery);
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
			auto errorText = createSelectableTextBlock(GetDriverErrorMessage());
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
			auto lifetime = get_strong();
			auto xamlRoot = XamlRoot();
			auto target = item;
			target.ULong1(m_callbackType);
			if (dangerous_confirm && !(co_await slg::ShowConfirmDialog(t(L"Common.Warning"), t(L"Utility.Msg.ConfirmAction"), t(L"Common.Continue"), t(L"Common.Cancel"), xamlRoot))) {
				co_return;
			}
			if (KernelInstance::RemoveCallback(target)) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				lifetime->WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});

		// 分割线1
		MenuFlyoutSeparator separator1;

		// 选项2.1
		auto item2_1 = slg::CreateMenuSubItem(flyoutStyles, L"\ue8c8", t(L"Common.CopyInfo").c_str());
		std::vector<std::pair<hstring, hstring>> copyItems = {
			{ t(L"Common.Type"), item.String1() },
			{ t(L"Common.Module"), item.String2() }
		};
		std::array<Button, 4> callbackHeaderButtons = { CallbackEntryHeaderButton(), CallbackHandleHeaderButton(), CallbackAddress3HeaderButton(), CallbackAddress4HeaderButton() };
		std::array<hstring, 4> callbackValues = { item.String3(), item.String4(), item.String5(), item.String6() };
		for (uint32_t i = 0; i < callbackHeaderButtons.size(); ++i) {
			if (callbackHeaderButtons[i].Visibility() != Visibility::Visible) continue;

			auto content = callbackHeaderButtons[i].Content();
			hstring label;
			if (auto text = content.try_as<TextBlock>()) {
				label = text.Text();
			}
			else {
				label = unbox_value_or<hstring>(content, L"");
			}
			if (!label.empty()) copyItems.push_back({ label, callbackValues[i] });
		}
		for (auto const& copyItem : copyItems) {
			hstring label = copyItem.first;
			hstring value = copyItem.second;
			auto menuItem = slg::CreateMenuItem(flyoutStyles, label.c_str(), [this, value](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
				if (TaskUtils::CopyToClipboard(value.c_str())) {
					slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				}
				else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
				co_return;
				});
			item2_1.Items().Append(menuItem);
		}

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
			auto lifetime = get_strong();
			auto xamlRoot = XamlRoot();
			auto target = item;
			if (dangerous_confirm && !(co_await slg::ShowConfirmDialog(t(L"Common.Warning"), t(L"Utility.Msg.ConfirmAction"), t(L"Common.Continue"), t(L"Common.Cancel"), xamlRoot))) {
				co_return;
			}
			if (KernelInstance::RemoveMiniFilter(target)) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				lifetime->WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
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

	void MonitorPage::SSDTListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = sender.as<ListView>();

		slg::SelectItemOnRightTapped(listView, e);

		if (!listView.SelectedItem()) return;

		auto item = listView.SelectedItem().as<winrt::StarlightGUI::GeneralEntry>();

		auto flyoutStyles = slg::GetStyles();

		MenuFlyout menuFlyout;

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
		auto item2_1_sub3 = slg::CreateMenuItem(flyoutStyles, L"\ueb19", t(L"Common.Address").c_str(), [this, item](IInspectable const& sender, RoutedEventArgs const& e) -> winrt::Windows::Foundation::IAsyncAction {
			if (TaskUtils::CopyToClipboard(item.String3().c_str())) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.CopyToClipboard.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), t(L"Msg.CopyToClipboard.Failed"), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
		item2_1.Items().Append(item2_1_sub3);

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
			auto lifetime = get_strong();
			auto xamlRoot = XamlRoot();
			auto target = item;
			if (dangerous_confirm && !(co_await slg::ShowConfirmDialog(t(L"Common.Warning"), t(L"Utility.Msg.ConfirmAction"), t(L"Common.Continue"), t(L"Common.Cancel"), xamlRoot))) {
				co_return;
			}
			if (KernelInstance::RemovePiDDBCache(target)) {
				slg::CreateInfoBarAndDisplay(t(L"Common.Success"), t(L"Msg.Success"), InfoBarSeverity::Success, g_mainWindowInstance);
				lifetime->WaitAndReloadAsync(1000);
			}
			else slg::CreateInfoBarAndDisplay(t(L"Common.Failed"), GetDriverErrorMessage(), InfoBarSeverity::Error, g_mainWindowInstance);
			co_return;
			});
#ifndef STARLIGHT_PREMIUM
		item1_1.Opacity(0.45);
		ToolTipService::SetToolTip(item1_1, tbox(L"Common.PremiumOnly"));
#endif

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

	void MonitorPage::HALTableListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e)
	{
		auto listView = HALTableListView();

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

	void MonitorPage::InitializeFlyout()
	{
		MenuFlyout flyout;

		for (auto const& option : CallbackTypeOptions) {
			ToggleMenuFlyoutItem item;
			item.Text(hstring(option.second));
			item.Tag(box_value(static_cast<int32_t>(option.first)));
			item.IsChecked(static_cast<int>(option.first) == m_callbackType);
			item.Click({ this, &MonitorPage::CallbackTypeMenuItem_Click });
			flyout.Items().Append(item);
		}

		CallbackTypeButton().Flyout(flyout);

		flyout = MenuFlyout();
		for (uint32_t i = 0; i < HALTableNames.size(); ++i) {
			ToggleMenuFlyoutItem item;
			item.Text(hstring(HALTableNames[i]));
			item.Tag(box_value(static_cast<int32_t>(i)));
			item.IsChecked(static_cast<int>(i) == m_halTableType);
			item.Click({ this, &MonitorPage::HALTableMenuItem_Click });
#ifndef STARLIGHT_PREMIUM
			if (i > 1) {
				item.Opacity(0.45);
				ToolTipService::SetToolTip(item, box_value(t(L"Common.PremiumOnly")));
			}
#endif
			flyout.Items().Append(item);
		}
		HALTableButton().Flyout(flyout);
	}

	void MonitorPage::UpdateCallbackColumns()
	{
		std::array<hstring, 4> labels = { t(L"Common.Address"), t(L"Monitor.Header.Address2"), t(L"Monitor.Header.Address3"), t(L"Monitor.Header.Address4") };
		std::array<double, 4> widths = { 150, 150, 150, 150 };
		uint32_t visibleColumns = 4;

		switch ((CallbackType)m_callbackType) {
		case CallbackType::CreateProcess:
		case CallbackType::CreateThread:
		case CallbackType::LoadImage:
		case CallbackType::LogonSessionTerminated:
		case CallbackType::DbgPrint:
			labels = { t(L"Monitor.Header.Routine"), t(L"Common.Index"), t(L"Monitor.Header.Flag"), L"" };
			widths = { 150, 80, 80, 0 };
			visibleColumns = 3;
			break;
		case CallbackType::Object:
			labels = { t(L"Common.Handle"), L"PreCall", L"PostCall", t(L"Monitor.Header.Object") };
			widths = { 150, 150, 150, 80 };
			break;
		case CallbackType::Registry:
			labels = { L"Cookie", t(L"Monitor.Header.Context"), t(L"Monitor.Header.Function"), L"" };
			visibleColumns = 3;
			break;
		case CallbackType::PowerSetting:
			labels = { t(L"Monitor.Header.Routine"), t(L"Monitor.Header.Context"), t(L"Monitor.Header.Configuration"), t(L"Monitor.Header.DeviceObject") };
			break;
		case CallbackType::PlugPlay:
			labels = { t(L"Monitor.Header.Routine"), t(L"Monitor.Header.Context"), t(L"Monitor.Header.DeviceObject"), t(L"Monitor.Header.DriverObject") };
			break;
		case CallbackType::Shutdown:
		case CallbackType::LastChanceShutdown:
			labels = { t(L"Monitor.Header.DeviceObject"), t(L"Monitor.Header.DriverObject"), L"", L"" };
			visibleColumns = 2;
			break;
		case CallbackType::FileSystemChange:
			labels = { t(L"Monitor.Header.Callback"), t(L"Monitor.Header.DeviceObject"), t(L"Monitor.Header.DriverObject"), L"" };
			visibleColumns = 3;
			break;
		case CallbackType::BugCheck:
			labels = { t(L"Monitor.Header.Routine"), t(L"Monitor.Header.Buffer"), t(L"Monitor.Header.Component"), t(L"Monitor.Header.Length") };
			widths = { 150, 150, 150, 80 };
			break;
		case CallbackType::BugCheckReason:
			labels = { t(L"Monitor.Header.Routine"), t(L"Monitor.Header.Component"), t(L"Monitor.Header.State"), t(L"Monitor.Header.Reason") };
			widths = { 150, 150, 80, 80 };
			break;
		case CallbackType::ExCallback:
			labels = { t(L"Monitor.Header.Routine"), t(L"Monitor.Header.Context"), t(L"Monitor.Header.Object"), t(L"Monitor.Header.Entry") };
			break;
		case CallbackType::LogonSessionTerminatedEx:
			labels = { t(L"Monitor.Header.Routine"), t(L"Monitor.Header.Context"), L"", L"" };
			visibleColumns = 2;
			break;
		case CallbackType::IoPriority:
			labels = { L"UserCallback", L"SelfReference", t(L"Monitor.Header.DeviceObject"), t(L"Monitor.Header.DriverObject") };
			break;
		case CallbackType::Coalescing:
			labels = { L"CallbackContext", L"SelfReference", t(L"Monitor.Header.DeviceObject"), t(L"Monitor.Header.DriverObject") };
			break;
		case CallbackType::Nmi:
			labels = { t(L"Monitor.Header.Routine"), t(L"Monitor.Header.Context"), L"SelfReference", L"" };
			visibleColumns = 3;
			break;
		default:
			break;
		}

		CallbackTypeModuleHeaderButton().Content(tbox(L"Monitor.Header.TypeModule"));
		CallbackEntryHeaderButton().Content(box_value(labels[0]));
		CallbackHandleHeaderButton().Content(box_value(labels[1]));
		CallbackAddress3HeaderButton().Content(box_value(labels[2]));
		CallbackAddress4HeaderButton().Content(box_value(labels[3]));

		std::array<ColumnDefinition, 4> headerColumns = { CallbackHeaderColumn1(), CallbackHeaderColumn2(), CallbackHeaderColumn3(), CallbackHeaderColumn4() };
		std::array<ColumnDefinition, 4> bodyColumns = { CallbackBodyColumn1(), CallbackBodyColumn2(), CallbackBodyColumn3(), CallbackBodyColumn4() };
		std::array<Button, 4> headerButtons = { CallbackEntryHeaderButton(), CallbackHandleHeaderButton(), CallbackAddress3HeaderButton(), CallbackAddress4HeaderButton() };

		for (uint32_t i = 0; i < headerColumns.size(); ++i) {
			bool visible = i < visibleColumns;
			GridLength width = visible ? GridLengthHelper::FromValueAndType(widths[i], GridUnitType::Pixel) : GridLengthHelper::FromPixels(0);
			headerColumns[i].Width(width);
			bodyColumns[i].Width(width);
			headerButtons[i].Visibility(visible ? Visibility::Visible : Visibility::Collapsed);
		}

		std::wstring_view callbackTypeName = CallbackTypeOptions[0].second;
		for (auto const& option : CallbackTypeOptions) {
			if (static_cast<int>(option.first) == m_callbackType) {
				callbackTypeName = option.second;
				break;
			}
		}
		CallbackTypeButton().Label(t(L"Monitor.CallbackTypeLabel", std::wstring(callbackTypeName).c_str()));

		auto flyout = CallbackTypeButton().Flyout().try_as<MenuFlyout>();
		if (!flyout) return;

		for (auto const& itemBase : flyout.Items()) {
			auto item = itemBase.try_as<ToggleMenuFlyoutItem>();
			if (!item) continue;

			int32_t index = unbox_value<int32_t>(item.Tag());
			item.IsChecked(index == m_callbackType);
		}
	}

	void MonitorPage::CallbackTypeMenuItem_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		if (!IsLoaded()) return;

		auto item = sender.try_as<ToggleMenuFlyoutItem>();
		if (!item) return;

		int selectedIndex = unbox_value<int32_t>(item.Tag());
		bool validCallbackType = false;
		for (auto const& option : CallbackTypeOptions) {
			if (static_cast<int>(option.first) == selectedIndex) {
				validCallbackType = true;
				break;
			}
		}
		if (!validCallbackType) return;
		if (m_isLoading) {
			UpdateCallbackColumns();
			return;
		}
		if (selectedIndex == m_callbackType) {
			UpdateCallbackColumns();
			return;
		}

		m_callbackType = selectedIndex;
		UpdateCallbackColumns();
		if (segmentedIndex == 1) {
			HandleSegmentedChange(segmentedIndex, true);
		}
	}

	void MonitorPage::HALTableMenuItem_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		if (!IsLoaded()) return;

		auto item = sender.try_as<ToggleMenuFlyoutItem>();
		if (!item) return;

		int selectedIndex = unbox_value<int32_t>(item.Tag());
		if (selectedIndex < 0 || selectedIndex >= static_cast<int>(HALTableNames.size())) return;

		bool reload = false;
		if (!m_isLoading && selectedIndex != m_halTableType) {
			m_halTableType = selectedIndex;
			reload = segmentedIndex == 9;
		}

		HALTableButton().Label(t(L"Monitor.HALTableLabel", std::wstring(HALTableNames[m_halTableType]).c_str()));
		if (auto flyout = HALTableButton().Flyout().try_as<MenuFlyout>()) {
			for (auto const& itemBase : flyout.Items()) {
				auto flyoutItem = itemBase.try_as<ToggleMenuFlyoutItem>();
				if (!flyoutItem) continue;

				int32_t index = unbox_value<int32_t>(flyoutItem.Tag());
				flyoutItem.IsChecked(index == m_halTableType);
			}
		}

		if (reload) {
			HandleSegmentedChange(segmentedIndex, true);
		}
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
            else if (segmentedIndex >= 1 && segmentedIndex <= 9) {
                for (auto const& entry : m_generalList) {
					std::array<hstring, 6> fields = {
						entry.String1(), entry.String2(), entry.String3(), entry.String4(), entry.String5(), entry.String6()
					};
					hstring key;
					for (auto const& field : fields) {
						if (field.empty()) continue;
						std::wstring lowerField = ToLowerCase(field.c_str());
						if (lowerQuery.empty() || lowerField.find(lowerQuery) != std::wstring::npos) {
							key = field;
							break;
						}
					}
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
			if (segmentedIndex != MainSegmented().SelectedIndex()) slg::CreateInfoBarAndDisplay(t(L"Common.Warning"), t(L"Monitor.Msg.WaitForLoading"), InfoBarSeverity::Warning, g_mainWindowInstance);
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

		DefaultText().Visibility(Visibility::Collapsed);
		CallbackTypeButton().Visibility(Visibility::Collapsed);
		HALTableButton().Visibility(Visibility::Collapsed);
		ObjectGrid().Visibility(Visibility::Collapsed);
		CallbackGrid().Visibility(Visibility::Collapsed);
		MiniFilterGrid().Visibility(Visibility::Collapsed);
		SSDTGrid().Visibility(Visibility::Collapsed);
		SSSDTGrid().Visibility(Visibility::Collapsed);
		IoTimerGrid().Visibility(Visibility::Collapsed);
		IDTGrid().Visibility(Visibility::Collapsed);
		GDTGrid().Visibility(Visibility::Collapsed);
		PiDDBGrid().Visibility(Visibility::Collapsed);
		HALTableGrid().Visibility(Visibility::Collapsed);
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
			CallbackTypeButton().Visibility(Visibility::Visible);
			UpdateCallbackColumns();
			CallbackGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 2: {
			MiniFilterGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 3: {
			SSDTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 4: {
			SSSDTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 5: {
			IoTimerGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 6: {
			IDTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 7: {
			GDTGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 8: {
			PiDDBGrid().Visibility(Visibility::Visible);
			co_await LoadGeneralList(force);
			break;
		}
		case 9: {
			HALTableButton().Visibility(Visibility::Visible);
			HALTableButton().Label(t(L"Monitor.HALTableLabel", std::wstring(HALTableNames[m_halTableType]).c_str()));
			if (auto flyout = HALTableButton().Flyout().try_as<MenuFlyout>()) {
				for (auto const& itemBase : flyout.Items()) {
					auto item = itemBase.try_as<ToggleMenuFlyoutItem>();
					if (!item) continue;

					int32_t itemIndex = unbox_value<int32_t>(item.Tag());
					item.IsChecked(itemIndex == m_halTableType);
				}
			}
			HALTableGrid().Visibility(Visibility::Visible);
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
		MonitorSegCallbackUid().Text(t(L"Monitor.Seg.Callback"));
		MonitorSegMiniFilterUid().Text(t(L"Monitor.Seg.MiniFilter"));
		MonitorSegSSDTUid().Text(t(L"Monitor.Seg.SSDT"));
		MonitorSegSSSDTUid().Text(t(L"Monitor.Seg.SSSDT"));
		MonitorSegIOTimerUid().Text(t(L"Monitor.Seg.IOTimer"));
		MonitorSegIDTUid().Text(t(L"Monitor.Seg.IDT"));
		MonitorSegGDTUid().Text(t(L"Monitor.Seg.GDT"));
		MonitorSegPiDDBUid().Text(t(L"Monitor.Seg.PiDDB"));
		MonitorSegHALTableUid().Text(t(L"Monitor.Seg.HALTable"));
		SearchBox().PlaceholderText(t(L"Monitor.Placeholder"));
		DefaultText().Text(t(L"Monitor.DefaultText"));
		RefreshButton().Label(t(L"Common.Refresh"));
		ObjectNameHeaderButton().Content(tbox(L"Common.Name"));
		ObjectTypeHeaderButton().Content(tbox(L"Common.Type"));
		CallbackTypeModuleHeaderButton().Content(tbox(L"Monitor.Header.TypeModule"));
		CallbackEntryHeaderButton().Content(tbox(L"Monitor.Header.Entry"));
		CallbackHandleHeaderButton().Content(tbox(L"Common.Handle"));
		MiniFilterIRPModuleHeaderButton().Content(tbox(L"Monitor.Header.IRPModule"));
		MiniFilterBaseHeaderButton().Content(tbox(L"Common.Base"));
		MiniFilterPreFilterHeaderButton().Content(tbox(L"Monitor.Header.PreFilter"));
		MiniFilterPostFilterHeaderButton().Content(tbox(L"Monitor.Header.PostFilter"));
		SSDTNameModuleHeaderButton().Content(tbox(L"Monitor.Header.NameModule"));
		SSDTAddressHeaderButton().Content(tbox(L"Common.Address"));
		SSSDTNameModuleHeaderButton().Content(tbox(L"Monitor.Header.NameModule"));
		SSSDTAddressHeaderButton().Content(tbox(L"Common.Address"));
		IoTimerModuleHeaderButton().Content(tbox(L"Common.Module"));
		IoTimerAddressHeaderButton().Content(tbox(L"Common.Address"));
		IoTimerDeviceObjHeaderButton().Content(tbox(L"Monitor.Header.DeviceObject"));
		IDTOffsetHeaderButton().Content(tbox(L"Offset"));
		IDTSelectorHeaderButton().Content(tbox(L"Selector"));
		IDTTypeHeaderButton().Content(tbox(L"Common.Type"));
		IDTDplHeaderButton().Content(tbox(L"DPL"));
		IDTIndexHeaderButton().Content(tbox(L"Common.Index"));
		GDTIndexHeaderButton().Content(tbox(L"Common.Index"));
		GDTBaseHeaderButton().Content(tbox(L"Common.Base"));
		GDTLimitHeaderButton().Content(tbox(L"Monitor.Header.Limit"));
		GDTTypeHeaderButton().Content(tbox(L"Common.Type"));
		GDTDplHeaderButton().Content(tbox(L"DPL"));
		GDTGranularityHeaderButton().Content(tbox(L"Granularity"));
		PiDDBModuleHeaderButton().Content(tbox(L"Common.Module"));
		PiDDBStatusHeaderButton().Content(tbox(L"Common.Status"));
		PiDDBTimestampHeaderButton().Content(tbox(L"Monitor.Header.Timestamp"));
		HALTableNameModuleHeaderButton().Content(tbox(L"Monitor.Header.NameModule"));
		HALTableAddressHeaderButton().Content(tbox(L"Common.Address"));
		HALTableButton().Label(t(L"Monitor.HALTableLabel", std::wstring(HALTableNames[m_halTableType]).c_str()));
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

			std::vector<std::pair<int, Grid>> grids;
			for (auto const& child : container.Children()) {
				auto border = child.try_as<Border>();
				if (!border) continue;

				auto grid = border.Child().try_as<Grid>();
				if (!grid) continue;

				grids.push_back({ Grid::GetRow(border), grid });
			}

			if (grids.size() >= 2) {
				std::sort(grids.begin(), grids.end(), [](auto const& a, auto const& b) {
					return a.first < b.first;
					});
				headerGrid = grids[0].second;
				bodyGrid = grids[1].second;
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
		AttachColumnSyncToSection(SSDTGrid(), 0);
		AttachColumnSyncToSection(SSSDTGrid(), 0);
		AttachColumnSyncToSection(IoTimerGrid(), 0);
		AttachColumnSyncToSection(IDTGrid(), 0);
		AttachColumnSyncToSection(GDTGrid(), 0);
		AttachColumnSyncToSection(PiDDBGrid(), 0);
		AttachColumnSyncToSection(HALTableGrid(), 0);

		for (auto const& binding : m_columnSyncBindings) {
			slg::SyncListViewColumnWidths(binding.HeaderGrid, binding.BodyGrid, binding.ListView, binding.RowOffset);
		}
	}
}
