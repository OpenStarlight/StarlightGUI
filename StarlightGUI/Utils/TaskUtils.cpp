#include "pch.h"
#include "TaskUtils.h"
#include "TlHelp32.h"
#include "shellapi.h"
#include "Psapi.h"

typedef BOOL(WINAPI* P_EndTask)(HWND hwnd, BOOL fShutdown, BOOL fForce);
typedef NTSTATUS(NTAPI* P_NtQuerySystemInformation)(_SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
typedef LONG(WINAPI* P_RtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);

P_EndTask _EndTask{ nullptr };
P_NtQuerySystemInformation _NtQuerySystemInformation{ nullptr };
P_RtlGetVersion _RtlGetVersion{ nullptr };

namespace winrt::StarlightGUI::implementation {
	void TaskUtils::EnsurePrivileges() {
		TaskUtils::EnableDebugPrivilege();
	}

	bool TaskUtils::_TerminateProcess(DWORD pid) {
		if (pid != 0) {
            HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
			if (hProc) {
				return TerminateProcess(hProc, 0);
			}
		}
		return false;
	}

	bool TaskUtils::_TerminateThread(DWORD tid) {
		if (tid != 0) {
			HANDLE hThread = OpenThread(THREAD_TERMINATE, FALSE, tid);
			if (hThread) {
				return TerminateThread(hThread, 0);
			}
		}
		return false;
	}

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
	* 获取进程私有工作集
	*/
	SIZE_T TaskUtils::GetProcessWorkingSet(HANDLE hProc) {
		std::vector<BYTE> buffer(sizeof(PSAPI_WORKING_SET_INFORMATION) + sizeof(PSAPI_WORKING_SET_BLOCK) * 1024);
		PSAPI_WORKING_SET_INFORMATION* workSetInfo = nullptr;
		PSAPI_WORKING_SET_BLOCK* pWorkSetBlock = nullptr;

		for (int attempt = 0; attempt < 6; ++attempt) {
			if (K32QueryWorkingSet(hProc, buffer.data(), static_cast<DWORD>(buffer.size()))) {
				workSetInfo = reinterpret_cast<PSAPI_WORKING_SET_INFORMATION*>(buffer.data());
				pWorkSetBlock = workSetInfo->WorkingSetInfo;
				break;
			}
			if (GetLastError() != ERROR_BAD_LENGTH) {
				return 0;
			}
			buffer.resize(buffer.size() * 2);
		}

		if (!workSetInfo || !pWorkSetBlock) {
			return 0;
		}
		PERFORMANCE_INFORMATION performanceInfo{};
		if (!K32GetPerformanceInfo(&performanceInfo, sizeof(performanceInfo))) return 0;
		SIZE_T pageSize = performanceInfo.PageSize;
		SIZE_T privateWorkingSet = 0;
		for (ULONG_PTR i = 0; i < workSetInfo->NumberOfEntries; ++i)
		{
			if (!pWorkSetBlock[i].Shared) // Remove shared pages
				privateWorkingSet += pageSize;
		}
		return privateWorkingSet;
	}

	/*
	* 获取进程CPU占用
	*/
	winrt::Windows::Foundation::IAsyncAction TaskUtils::FetchProcessCpuUsage(std::map<DWORD, hstring>& processCpuTable) {
		co_await winrt::resume_background();

		if (!_NtQuerySystemInformation) {
			HMODULE hNtdll = LoadLibraryW(L"ntdll.dll");
			_NtQuerySystemInformation = (P_NtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");
		}

		if (!_NtQuerySystemInformation) co_return; // Check again to ensure safety

		ULONG len = 0;
		std::vector<BYTE> buffer(0x10000);
		NTSTATUS status = _NtQuerySystemInformation(SystemProcessInformation, buffer.data(),
			static_cast<ULONG>(buffer.size()), &len);

		if (NT_ERROR(status) && len > 0) {
			buffer.resize(len);
			status = _NtQuerySystemInformation(SystemProcessInformation, buffer.data(),
				static_cast<ULONG>(buffer.size()), &len);
		}

		if (!NT_SUCCESS(status)) co_return;

		// Loop through the processes and fill the map with PID and CPU usage
		PMY_SYSTEM_PROCESS_INFORMATION spi = (PMY_SYSTEM_PROCESS_INFORMATION)buffer.data();
		while (spi) {
			LONG64 cpuUsage = spi->UserTime.QuadPart + spi->KernelTime.QuadPart;
			if (cpuUsage > 0) {
				processCpuTable[(DWORD)(ULONG_PTR)spi->UniqueProcessId] = to_hstring(std::round(cpuUsage / 1000000000.0) / 10.0) + L"s";
			}
			else processCpuTable[(DWORD)(ULONG_PTR)spi->UniqueProcessId] = L"0s";

			if (spi->NextEntryOffset == 0)
				break;
			spi = (PMY_SYSTEM_PROCESS_INFORMATION)((BYTE*)spi + spi->NextEntryOffset);
		}

		co_return;
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

	/*
	* 获取Windows版本
	*/
	DWORD TaskUtils::GetWindowsBuildNumber() {
		if (!_RtlGetVersion) {
			HMODULE hNtdll = LoadLibraryW(L"ntdll.dll");
			_RtlGetVersion = (P_RtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
		}
		if (!_RtlGetVersion) return 0;
		RTL_OSVERSIONINFOW osInfo = { 0 };
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		if (_RtlGetVersion(&osInfo) == 0) {
			return osInfo.dwBuildNumber;
		}
		return 0;
	}

	BOOL CALLBACK TaskUtils::EndTaskByWindow(HWND hwnd) {
		if (_EndTask == nullptr) {
			_EndTask = (P_EndTask)GetProcAddress(GetModuleHandleW(L"user32.dll"), "EndTask");
		}

		_EndTask(hwnd, FALSE, TRUE);
		return TRUE;
	}

	// =====================================================
// Private --- Starting here
// =====================================================

	bool TaskUtils::EnableDebugPrivilege() {
		HANDLE hToken;
		TOKEN_PRIVILEGES tkp;

		if (!OpenProcessToken(GetCurrentProcess(),
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			return false;
		}

		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);

		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
		CloseHandle(hToken);

		return result != FALSE;
	}
}
