#include "pch.h"
#include "TaskUtils.h"
#include "TlHelp32.h"
#include "shellapi.h"
#include "Psapi.h"
#include <algorithm>
#include <vector>

typedef BOOL(*EndTask_t)(HWND windowHandle, BOOL shutdown, BOOL force);

static EndTask_t endTask = NULL;

namespace winrt::StarlightGUI::implementation {

	bool TaskUtils::EnableProcessPerformanceMode(StarlightGUI::ProcessInfo process) {
		int pid = process.Id();
		if (pid != 0) {
			HANDLE processHandle = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);

			if (processHandle) {
				PROCESS_POWER_THROTTLING_STATE throttling;
				ZeroMemory(&throttling, sizeof(throttling));

				throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
				throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
				throttling.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;

				BOOL result = SetProcessInformation(processHandle, ProcessPowerThrottling, &throttling, sizeof(throttling));
				CloseHandle(processHandle);

				return result;
			}
		}
		return false;
	}

	bool TaskUtils::FetchProcessCpuUsage(ProcessCpuSnapshot const& previousSnapshot,
		ProcessCpuSnapshot& currentSnapshot, std::map<DWORD, double>& processCpuTable) {
		ULONG length = 0;
		std::vector<BYTE> buffer(0x10000);
		LONG status = NtQuerySystemInformation(5, buffer.data(), (ULONG)buffer.size(), &length);
		while (status < 0 && length > buffer.size()) {
			buffer.resize((size_t)length + 0x10000);
			status = NtQuerySystemInformation(5, buffer.data(), (ULONG)buffer.size(), &length);
		}
		if (status < 0) return false;

		FILETIME idleTime{}, kernelTime{}, userTime{};
		if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return false;

		ULARGE_INTEGER kernel{}, user{};
		kernel.LowPart = kernelTime.dwLowDateTime;
		kernel.HighPart = kernelTime.dwHighDateTime;
		user.LowPart = userTime.dwLowDateTime;
		user.HighPart = userTime.dwHighDateTime;

		currentSnapshot.processes.clear();
		currentSnapshot.totalTime = kernel.QuadPart + user.QuadPart;
		processCpuTable.clear();

		PSYSTEM_PROCESS_INFORMATION process = (PSYSTEM_PROCESS_INFORMATION)buffer.data();
		while (process) {
			DWORD pid = (DWORD)(ULONG_PTR)process->UniqueProcessId;
			ProcessCpuSample sample{
				process->UserTime.QuadPart + process->KernelTime.QuadPart,
				process->CreateTime.QuadPart
			};
			currentSnapshot.processes[pid] = sample;

			double usage = 0.0;
			auto previous = previousSnapshot.processes.find(pid);
			if (previous != previousSnapshot.processes.end() &&
				previous->second.createTime == sample.createTime &&
				sample.processTime >= previous->second.processTime &&
				currentSnapshot.totalTime > previousSnapshot.totalTime) {
				ULONGLONG processDelta = (ULONGLONG)(sample.processTime - previous->second.processTime);
				ULONGLONG totalDelta = currentSnapshot.totalTime - previousSnapshot.totalTime;
				usage = (double)processDelta * 100.0 / (double)totalDelta;
			}
			processCpuTable[pid] = std::min(100.0, std::max(0.0, usage));

			if (process->NextEntryOffset == 0) break;
			process = (PSYSTEM_PROCESS_INFORMATION)((BYTE*)process + process->NextEntryOffset);
		}

		return true;
	}

	bool TaskUtils::CopyToClipboard(std::wstring str) {
		if (str.empty())
			return false;

		if (!OpenClipboard(nullptr))
			return false;

		if (!EmptyClipboard()) {
			CloseClipboard();
			return false;
		}

		size_t sizeInBytes = (str.size() + 1) * sizeof(wchar_t);
		HGLOBAL globalMemory = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
		if (!globalMemory) {
			CloseClipboard();
			return false;
		}

		void* globalBuffer = GlobalLock(globalMemory);
		if (!globalBuffer) {
			GlobalFree(globalMemory);
			CloseClipboard();
			return false;
		}
		memcpy(globalBuffer, str.c_str(), sizeInBytes);
		GlobalUnlock(globalMemory);

		if (!SetClipboardData(CF_UNICODETEXT, globalMemory)) {
			GlobalFree(globalMemory);
			CloseClipboard();
			return false;
		}

		CloseClipboard();
		return true;
	}
	bool TaskUtils::OpenFolderAndSelectFile(std::wstring filePath) {
		DWORD attrs = GetFileAttributesW(filePath.c_str());
		if (attrs == INVALID_FILE_ATTRIBUTES)
			return false;

		std::wstring command = L"explorer.exe";
		std::wstring arguments = L"/select,\"" + filePath + L"\"";

		SHELLEXECUTEINFOW shellExecuteInfo{ sizeof(shellExecuteInfo) };
		shellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		shellExecuteInfo.lpVerb = L"open";
		shellExecuteInfo.lpFile = command.c_str();
		shellExecuteInfo.lpParameters = arguments.c_str();
		shellExecuteInfo.nShow = SW_SHOWNORMAL;

		BOOL result = ShellExecuteExW(&shellExecuteInfo);

		if (shellExecuteInfo.hProcess)
			CloseHandle(shellExecuteInfo.hProcess);

		return result;
	}

	bool TaskUtils::OpenFileProperties(std::wstring filePath) {
		SHELLEXECUTEINFOW shellExecuteInfo{};
		shellExecuteInfo.cbSize = sizeof(shellExecuteInfo);
		shellExecuteInfo.fMask = SEE_MASK_INVOKEIDLIST;
		shellExecuteInfo.hwnd = NULL;
		shellExecuteInfo.lpVerb = L"properties";
		shellExecuteInfo.lpFile = filePath.c_str();
		shellExecuteInfo.nShow = SW_SHOW;

		return ShellExecuteExW(&shellExecuteInfo) != FALSE;
	}

	bool TaskUtils::EndTaskByWindow(HWND windowHandle) {
		if (!endTask) endTask = (EndTask_t)GetProcAddress(GetModuleHandleW(L"user32.dll"), "EndTask");
		if (!endTask) return FALSE;

		return endTask(windowHandle, FALSE, TRUE);
	}
}
