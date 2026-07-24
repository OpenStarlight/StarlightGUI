#include "pch.h"
#include "TaskUtils.h"
#include "TlHelp32.h"
#include "shellapi.h"
#include "Psapi.h"

typedef BOOL(*EndTask_t)(HWND hwnd, BOOL fShutdown, BOOL fForce);
typedef LONG(*NtQuerySystemInformation_t)(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

EndTask_t EndTask = NULL;
NtQuerySystemInformation_t NtQuerySystemInformation = NULL;

namespace winrt::StarlightGUI::implementation {

	/*
	* 开启进程效能模式
	*/
	bool TaskUtils::EnableProcessPerformanceMode(StarlightGUI::ProcessInfo pi) {
		int pid = pi.Id();
		if (pid != 0) {
			HANDLE hProc = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);

			if (hProc) {
				PROCESS_POWER_THROTTLING_STATE throttling;
				ZeroMemory(&throttling, sizeof(throttling));

				throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
				throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
				throttling.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;

				BOOL result = SetProcessInformation(hProc, ProcessPowerThrottling, &throttling, sizeof(throttling));
				CloseHandle(hProc);

				return result;
			}
		}
		return false;
	}

	/*
	 * 获取进程CPU占用
	 */
	bool TaskUtils::FetchProcessCpuUsage(ProcessCpuSnapshot const& previousSnapshot,
		ProcessCpuSnapshot& currentSnapshot, std::map<DWORD, double>& processCpuTable) {
		if (!NtQuerySystemInformation) {
			HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
			if (!hNtdll) return false;
			NtQuerySystemInformation = (NtQuerySystemInformation_t)GetProcAddress(hNtdll, "NtQuerySystemInformation");
		}
		if (!NtQuerySystemInformation) return false;

		ULONG length = 0;
		std::vector<BYTE> buffer(0x10000);
		LONG status = NtQuerySystemInformation(5, buffer.data(), (ULONG)buffer.size(), &length);
		while (status < 0 && length > buffer.size()) {
			buffer.resize(static_cast<size_t>(length) + 0x10000);
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

		currentSnapshot.Processes.clear();
		currentSnapshot.TotalTime = kernel.QuadPart + user.QuadPart;
		processCpuTable.clear();

		PSYSTEM_PROCESS_INFORMATION process = (PSYSTEM_PROCESS_INFORMATION)buffer.data();
		while (process) {
			DWORD pid = (DWORD)(ULONG_PTR)process->UniqueProcessId;
			ProcessCpuSample sample{
				process->UserTime.QuadPart + process->KernelTime.QuadPart,
				process->CreateTime.QuadPart
			};
			currentSnapshot.Processes[pid] = sample;

			double usage = 0.0;
			auto previous = previousSnapshot.Processes.find(pid);
			if (previous != previousSnapshot.Processes.end() &&
				previous->second.CreateTime == sample.CreateTime &&
				sample.ProcessTime >= previous->second.ProcessTime &&
				currentSnapshot.TotalTime > previousSnapshot.TotalTime) {
				ULONGLONG processDelta = static_cast<ULONGLONG>(sample.ProcessTime - previous->second.ProcessTime);
				ULONGLONG totalDelta = currentSnapshot.TotalTime - previousSnapshot.TotalTime;
				usage = static_cast<double>(processDelta) * 100.0 / static_cast<double>(totalDelta);
			}
			processCpuTable[pid] = std::min(100.0, std::max(0.0, usage));

			if (process->NextEntryOffset == 0) break;
			process = (PSYSTEM_PROCESS_INFORMATION)((BYTE*)process + process->NextEntryOffset);
		}

		return true;
	}

	/*
	 * 复制至剪贴板
	 */
	bool TaskUtils::CopyToClipboard(std::wstring str) {
		if (str.empty()) {
			return false;
		}

		if (!OpenClipboard(nullptr)) {
			return false;
		}

		if (!EmptyClipboard()) {
			CloseClipboard();
			return false;
		}

		size_t sizeInBytes = (str.size() + 1) * sizeof(wchar_t);
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
		if (!hGlobal) {
			CloseClipboard();
			return false;
		}

		void* pGlobal = GlobalLock(hGlobal);
		if (!pGlobal) {
			GlobalFree(hGlobal);
			CloseClipboard();
			return false;
		}
		memcpy(pGlobal, str.c_str(), sizeInBytes);
		GlobalUnlock(hGlobal);

		if (!SetClipboardData(CF_UNICODETEXT, hGlobal)) {
			GlobalFree(hGlobal);
			CloseClipboard();
			return false;
		}

		CloseClipboard();
		return true;
	}
	/*
	 * 打开文件所在位置并选中文件
	 */
	bool TaskUtils::OpenFolderAndSelectFile(std::wstring filePath) {
		DWORD attrs = GetFileAttributesW(filePath.c_str());
		if (attrs == INVALID_FILE_ATTRIBUTES) {
			return false;
		}

		std::wstring cmd = L"explorer.exe";
		std::wstring args = L"/select,\"" + filePath + L"\"";

		SHELLEXECUTEINFOW sei = { sizeof(sei) };
		sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		sei.lpVerb = L"open";
		sei.lpFile = cmd.c_str();
		sei.lpParameters = args.c_str();
		sei.nShow = SW_SHOWNORMAL;

		BOOL result = ShellExecuteExW(&sei);

		if (sei.hProcess) {
			CloseHandle(sei.hProcess);
		}

		return result;
	}

	/*
	 * 打开文件属性
	 */
	bool TaskUtils::OpenFileProperties(std::wstring filePath) {
		SHELLEXECUTEINFOW sei = { 0 };
		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_INVOKEIDLIST;
		sei.hwnd = NULL;
		sei.lpVerb = L"properties";
		sei.lpFile = filePath.c_str();
		sei.nShow = SW_SHOW;

		return ShellExecuteExW(&sei) != FALSE;
	}

	bool TaskUtils::EndTaskByWindow(HWND hwnd) {
		if (!EndTask) EndTask = (EndTask_t)GetProcAddress(GetModuleHandleW(L"user32.dll"), "EndTask");
		if (!EndTask) return FALSE;

		return EndTask(hwnd, FALSE, TRUE);
	}
}
