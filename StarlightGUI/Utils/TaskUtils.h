#pragma once

#include <pch.h>
#include "Utils/ProcessInfo.h"
#include "NTBase.h"

typedef struct _SYSTEM_PROCESS_INFORMATION {
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
	LONG BasePriority;
	HANDLE UniqueProcessId;
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

using namespace winrt;

namespace winrt::StarlightGUI::implementation {
	class TaskUtils {
	public:
		struct ProcessCpuSample {
			LONGLONG ProcessTime{ 0 };
			LONGLONG CreateTime{ 0 };
		};

		struct ProcessCpuSnapshot {
			std::map<DWORD, ProcessCpuSample> Processes;
			ULONGLONG TotalTime{ 0 };
		};

		static bool EnableProcessPerformanceMode(StarlightGUI::ProcessInfo pi);
		
		static bool FetchProcessCpuUsage(ProcessCpuSnapshot const& previousSnapshot,
			ProcessCpuSnapshot& currentSnapshot, std::map<DWORD, double>& processCpuTable);

		static bool CopyToClipboard(std::wstring str);

		static bool OpenFolderAndSelectFile(std::wstring filePath);

		static bool OpenFileProperties(std::wstring filePath);

		static bool EndTaskByWindow(HWND hwnd);
	};
}
