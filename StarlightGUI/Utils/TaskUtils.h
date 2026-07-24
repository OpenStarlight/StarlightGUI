#pragma once

#include <Windows.h>
#include "ProcessInfo.h"
#include "NTBase.h"
#include <map>
#include <string>
#include <unordered_map>

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

namespace winrt::StarlightGUI::implementation {
	class TaskUtils {
	public:
		struct ProcessCpuSample {
			LONGLONG processTime{ 0 };
			LONGLONG createTime{ 0 };
		};

		struct ProcessCpuSnapshot {
			std::unordered_map<DWORD, ProcessCpuSample> processes;
			ULONGLONG totalTime{ 0 };
		};

		static bool EnableProcessPerformanceMode(StarlightGUI::ProcessInfo process);
		
		static bool FetchProcessCpuUsage(ProcessCpuSnapshot const& previousSnapshot,
			ProcessCpuSnapshot& currentSnapshot, std::map<DWORD, double>& processCpuTable);

		static bool CopyToClipboard(std::wstring str);

		static bool OpenFolderAndSelectFile(std::wstring filePath);

		static bool OpenFileProperties(std::wstring filePath);

		static bool EndTaskByWindow(HWND windowHandle);
	};
}
