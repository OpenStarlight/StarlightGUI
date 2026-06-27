#pragma once

#include <pch.h>
#include <winternl.h>
#include <Utils/ProcessInfo.h>

typedef struct MY_SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	LARGE_INTEGER WorkingSetPrivateSize;
	ULONG HardFaultCount;
	ULONG NumberOfThreadsHighWatermark;
	ULONGLONG CycleTime;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ImageName;
	KPRIORITY BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
	ULONG HandleCount;
	ULONG SessionId;
	ULONG_PTR UniqueProcessKey;
	SIZE_T PeakVirtualSize;
	SIZE_T VirtualSize;
	ULONG PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER ReadOperationCount;
	LARGE_INTEGER WriteOperationCount;
	LARGE_INTEGER OtherOperationCount;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
} MY_SYSTEM_PROCESS_INFORMATION, * PMY_SYSTEM_PROCESS_INFORMATION;

using namespace winrt;

namespace winrt::StarlightGUI::implementation {
	class TaskUtils {
	public:
		static void EnsurePrivileges();

		static bool _TerminateThread(DWORD tid);

		static bool _TerminateProcess(DWORD pid);

		static bool EnableProcessPerformanceMode(StarlightGUI::ProcessInfo pi);

		static SIZE_T GetProcessWorkingSet(HANDLE hProc);
		
		static winrt::Windows::Foundation::IAsyncAction FetchProcessCpuUsage(std::map<DWORD, hstring>& processCpuTable);

		static bool CopyToClipboard(std::wstring str);

		static bool OpenFolderAndSelectFile(std::wstring filePath);

		static bool OpenFileProperties(std::wstring filePath);

		static DWORD GetWindowsBuildNumber();

		static BOOL CALLBACK EndTaskByWindow(HWND hwnd);
	private:

		static bool EnableDebugPrivilege();
	};
}