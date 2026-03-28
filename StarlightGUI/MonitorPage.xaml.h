#pragma once

#include "MonitorPage.g.h"
#include <sddl.h>
#include <vector>
#define PAGE_SIZE 0x1000

#define DBWIN_BUFFER_READY L"DBWIN_BUFFER_READY"
#define DBWIN_DATA_READY L"DBWIN_DATA_READY"
#define DBWIN_BUFFER L"DBWIN_BUFFER"

typedef struct _DBWIN_PAGE_BUFFER
{
	ULONG ProcessId;
	CHAR Buffer[PAGE_SIZE - sizeof(ULONG)];
} DBWIN_PAGE_BUFFER, * PDBWIN_PAGE_BUFFER;

struct DbgViewMonitor
{
	DbgViewMonitor(hstring* data, std::mutex* lock)
	{
		LocalCaptureEnabled = FALSE;
		LocalBufferReadyEvent = NULL;
		LocalDataReadyEvent = NULL;
		LocalDataBufferHandle = NULL;
		LocalDebugBuffer = NULL;

		GlobalCaptureEnabled = FALSE;
		GlobalBufferReadyEvent = NULL;
		GlobalDataReadyEvent = NULL;
		GlobalDataBufferHandle = NULL;
		GlobalDebugBuffer = NULL;

		Data = data;
		Lock = lock;

		DbgCreateSecurityAttributes();
	}

	~DbgViewMonitor()
	{
		DbgCleanupSecurityAttributes();
	}

	SECURITY_ATTRIBUTES SecurityAttributes;

	BOOLEAN LocalCaptureEnabled;
	HANDLE LocalBufferReadyEvent;
	HANDLE LocalDataReadyEvent;
	HANDLE LocalDataBufferHandle;
	PDBWIN_PAGE_BUFFER LocalDebugBuffer;

	BOOLEAN GlobalCaptureEnabled;
	HANDLE GlobalBufferReadyEvent;
	HANDLE GlobalDataReadyEvent;
	HANDLE GlobalDataBufferHandle;
	PDBWIN_PAGE_BUFFER GlobalDebugBuffer;

	hstring* Data;
	std::mutex* Lock;

	BOOL DbgCreateSecurityAttributes()
	{
		SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
		SecurityAttributes.bInheritHandle = FALSE;
		return ConvertStringSecurityDescriptorToSecurityDescriptorW(
			L"D:(A;;GRGWGX;;;WD)(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGWGX;;;AN)(A;;GRGWGX;;;RC)(A;;GRGWGX;;;S-1-15-2-1)S:(ML;;NW;;;LW)",
			SDDL_REVISION, &SecurityAttributes.lpSecurityDescriptor, NULL);
	}

	void DbgCleanupSecurityAttributes()
	{
		if (SecurityAttributes.lpSecurityDescriptor)
		{
			LocalFree(SecurityAttributes.lpSecurityDescriptor);
			SecurityAttributes.lpSecurityDescriptor = NULL;
		}
	}

	void Init(bool global)
	{
		BOOLEAN& CaptureEnabled = global ? GlobalCaptureEnabled : LocalCaptureEnabled;
		HANDLE& BufferReadyEvent = global ? GlobalBufferReadyEvent : LocalBufferReadyEvent;
		HANDLE& DataReadyEvent = global ? GlobalDataReadyEvent : LocalDataReadyEvent;
		HANDLE& DataBufferHandle = global ? GlobalDataBufferHandle : LocalDataBufferHandle;
		PDBWIN_PAGE_BUFFER& DebugBuffer = global ? GlobalDebugBuffer : LocalDebugBuffer;

		SIZE_T viewSize;
		LARGE_INTEGER maximumSize;

		maximumSize.QuadPart = PAGE_SIZE;
		viewSize = sizeof(DBWIN_PAGE_BUFFER);

		if (!(BufferReadyEvent = CreateEventW(&SecurityAttributes, FALSE, FALSE, global ? L"Global\\" DBWIN_BUFFER_READY : L"Local\\" DBWIN_BUFFER_READY)) ||
			GetLastError() == ERROR_ALREADY_EXISTS)
		{
			UnInit(global);
			return;
		}

		if (!(DataReadyEvent = CreateEventW(&SecurityAttributes, FALSE, FALSE, global ? L"Global\\" DBWIN_DATA_READY : L"Local\\" DBWIN_DATA_READY)) ||
			GetLastError() == ERROR_ALREADY_EXISTS)
		{
			UnInit(global);
			return;
		}

		if (!(DataBufferHandle = CreateFileMappingW(
			INVALID_HANDLE_VALUE,
			&SecurityAttributes,
			PAGE_READWRITE,
			maximumSize.HighPart,
			maximumSize.LowPart,
			global ? L"Global\\" DBWIN_BUFFER : L"Local\\" DBWIN_BUFFER
		)) || GetLastError() == ERROR_ALREADY_EXISTS)
		{
			UnInit(global);
			return;
		}

		if (!(DebugBuffer = (PDBWIN_PAGE_BUFFER)MapViewOfFile(
			DataBufferHandle,
			FILE_MAP_READ,
			0,
			0,
			viewSize
		)))
		{
			UnInit(global);
			return;
		}

		CaptureEnabled = TRUE;
	}

	void UnInit(bool global)
	{
		BOOLEAN& CaptureEnabled = global ? GlobalCaptureEnabled : LocalCaptureEnabled;
		HANDLE& BufferReadyEvent = global ? GlobalBufferReadyEvent : LocalBufferReadyEvent;
		HANDLE& DataReadyEvent = global ? GlobalDataReadyEvent : LocalDataReadyEvent;
		HANDLE& DataBufferHandle = global ? GlobalDataBufferHandle : LocalDataBufferHandle;
		PDBWIN_PAGE_BUFFER& DebugBuffer = global ? GlobalDebugBuffer : LocalDebugBuffer;

		CaptureEnabled = FALSE;

		if (DebugBuffer)
		{
			UnmapViewOfFile(DebugBuffer);
			DebugBuffer = NULL;
		}

		if (DataBufferHandle)
		{
			CloseHandle(DataBufferHandle);
			DataBufferHandle = NULL;
		}

		if (BufferReadyEvent)
		{
			CloseHandle(BufferReadyEvent);
			BufferReadyEvent = NULL;
		}

		if (DataReadyEvent)
		{
			CloseHandle(DataReadyEvent);
			DataReadyEvent = NULL;
		}
	}
};

namespace winrt::StarlightGUI::implementation
{
    struct MonitorPage : MonitorPageT<MonitorPage>
    {
        MonitorPage();
		void SetupLocalization();

        void MainSegmented_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        slg::coroutine HandleSegmentedChange(int index, bool force);

        void ObjectTreeView_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void ObjectListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void CallbackListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void MiniFilterListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void StdFilterListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void SSDTListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void ExCallbackListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void PiDDBListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
		void HALDPTListView_RightTapped(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& e);
        void MonitorListView_ContainerContentChanging(
            winrt::Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            winrt::Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        slg::coroutine RefreshButton_Click(IInspectable const&, RoutedEventArgs const&);
        slg::coroutine DbgViewButton_Click(IInspectable const&, RoutedEventArgs const&);
        slg::coroutine DbgViewGlobalCheckBox_Click(IInspectable const&, RoutedEventArgs const&);

        winrt::Windows::Foundation::IAsyncAction LoadItemList();
        winrt::Windows::Foundation::IAsyncAction LoadPartitionList(std::wstring path);
        winrt::Windows::Foundation::IAsyncAction LoadObjectList();
        winrt::Windows::Foundation::IAsyncAction LoadGeneralList(bool force);
        winrt::Windows::Foundation::IAsyncAction WaitAndReloadAsync(int interval);
        winrt::Windows::Foundation::IAsyncAction InitializeDbgView();

        void ColumnHeader_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& e);
        void SearchBox_SuggestionChosen(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxSuggestionChosenEventArgs const& e);
        void SearchBox_QuerySubmitted(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& e);
        bool ApplyFilter(const hstring& target, const hstring& query);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::ObjectEntry> m_objectList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::ObjectEntry>()
        };
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::WinUI3Package::SegmentedItem> m_itemList{
            winrt::single_threaded_observable_vector<winrt::WinUI3Package::SegmentedItem>()
        };
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::StarlightGUI::GeneralEntry> m_generalList{
            winrt::single_threaded_observable_vector<winrt::StarlightGUI::GeneralEntry>()
        };

		int segmentedIndex = 0;
        bool m_isLoading = false;
        uint32_t m_lastDbgViewLength = 0;
        uint64_t m_reloadRequestVersion = 0;
		winrt::Microsoft::UI::Xaml::DispatcherTimer windbgTimer;

        struct ColumnSyncBinding
        {
            winrt::Microsoft::UI::Xaml::Controls::Grid HeaderGrid{ nullptr };
            winrt::Microsoft::UI::Xaml::Controls::Grid BodyGrid{ nullptr };
            winrt::Microsoft::UI::Xaml::Controls::ListView ListView{ nullptr };
            uint32_t RowOffset = 0;
        };

        void InitializeColumnSyncBindings();
        void AttachColumnSyncToSection(winrt::Microsoft::UI::Xaml::Controls::Grid const& sectionRoot, uint32_t rowOffset = 0);
        static void EnsureHeaderSplitters(winrt::Microsoft::UI::Xaml::Controls::Grid const& headerGrid);
        std::vector<ColumnSyncBinding> m_columnSyncBindings;

        inline static bool currentSortingOption;
        inline static hstring currentSortingType;

		inline static bool isDbgViewEnabled, isDbgViewGlobalEnabled;
		inline static hstring dbgViewData = L"";
		inline static std::mutex dbgViewMutex;
		inline static DbgViewMonitor dbgViewMonitor{ &dbgViewData, &dbgViewMutex };
    };
}

namespace winrt::StarlightGUI::factory_implementation
{
    struct MonitorPage : MonitorPageT<MonitorPage, implementation::MonitorPage>
    {
    };
}
