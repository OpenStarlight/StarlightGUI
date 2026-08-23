#include "pch.h"
#include "KernelBase.h"
#include "Config.h"
#include "CppUtils.h"
#include <cwctype>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace winrt::StarlightGUI::implementation {
	namespace fs = std::filesystem;

	static HANDLE driverDevice = NULL;
	static DWORD lastErrorCode = ERROR_SUCCESS;
	static std::wstring lastErrorMessage = L"";

	DWORD KernelInstance::GetLastErrorCode() noexcept {
		return lastErrorCode;
	}

	std::wstring KernelInstance::GetLastErrorMessage() noexcept {
		if (lastErrorCode == ERROR_SUCCESS) return lastErrorMessage;

		LPWSTR messageBuffer = nullptr;
		DWORD length = FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			lastErrorCode,
			0,
			reinterpret_cast<LPWSTR>(&messageBuffer),
			0,
			nullptr);

		std::wstring message;
		if (length != 0 && messageBuffer != nullptr) {
			message.assign(messageBuffer, length);
			LocalFree(messageBuffer);
			while (!message.empty() && iswspace(message.back())) message.pop_back();
		}
		else {
			message = lastErrorMessage;
		}

		wchar_t errorCode[16];
		swprintf_s(errorCode, L" (%lu)", lastErrorCode);
		message += errorCode;
		return message;
	}

	void KernelInstance::QueryError() noexcept {
		if (driverDevice == NULL) {
			lastErrorCode = ERROR_INVALID_HANDLE;
			lastErrorMessage = L"Driver device not initialized.";
			return;
		}

		lastErrorCode = GetLastError();
		lastErrorMessage = lastErrorCode == ERROR_SUCCESS ? L"" : L"The driver operation failed.";
	}

	BOOL KernelInstance::QuerySystemEnumeration(SystemGetInformation information, SI_ENUMERATION& enumData, ULONG itemSize, ULONG argument) noexcept {
		enumData.Buffer = NULL;
		enumData.BufferSize = 0;
		enumData.Count = 0;

		BOOL result = SiQuerySystemInformation(information, &enumData, argument);
		QueryError();
		if (!result) return FALSE;
		if (enumData.Count == 0) return TRUE;
		if (itemSize == 0 || enumData.Count > (ULONG)-1 / itemSize) {
			lastErrorCode = ERROR_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid enumeration size.";
			return FALSE;
		}

		enumData.BufferSize = enumData.Count * itemSize;
		ULONG capacity = enumData.Count;
		enumData.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, enumData.BufferSize);
		if (!enumData.Buffer) {
			lastErrorCode = ERROR_NOT_ENOUGH_MEMORY;
			lastErrorMessage = L"Failed to allocate enumeration buffer.";
			return FALSE;
		}

		enumData.Count = 0;
		result = SiQuerySystemInformation(information, &enumData, argument);
		QueryError();
		if (result && enumData.Count > capacity) enumData.Count = capacity;
		return result;
	}

	BOOL KernelInstance::QueryProcessEnumeration(ProcessGetInformation information, ULONG pid, SI_ENUMERATION& enumData, ULONG itemSize, ULONG argument) noexcept {
		enumData.Buffer = NULL;
		enumData.BufferSize = 0;
		enumData.Count = 0;

		BOOL result = SiQueryProcessInformation(information, pid, &enumData, argument);
		QueryError();
		if (!result) return FALSE;
		if (enumData.Count == 0) return TRUE;
		if (itemSize == 0 || enumData.Count > (ULONG)-1 / itemSize) {
			lastErrorCode = ERROR_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid enumeration size.";
			return FALSE;
		}

		enumData.BufferSize = enumData.Count * itemSize;
		ULONG capacity = enumData.Count;
		enumData.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, enumData.BufferSize);
		if (!enumData.Buffer) {
			lastErrorCode = ERROR_NOT_ENOUGH_MEMORY;
			lastErrorMessage = L"Failed to allocate enumeration buffer.";
			return FALSE;
		}

		enumData.Count = 0;
		result = SiQueryProcessInformation(information, pid, &enumData, argument);
		QueryError();
		if (result && enumData.Count > capacity) enumData.Count = capacity;
		return result;
	}

	BOOL KernelInstance::QueryFileEnumeration(FileGetInformation information, LPCWSTR path, SI_ENUMERATION& enumData, ULONG itemSize, ULONG argument) noexcept {
		enumData.Buffer = NULL;
		enumData.BufferSize = 0;
		enumData.Count = 0;

		BOOL result = SiQueryFileInformation(information, path, &enumData, argument);
		QueryError();
		if (!result) return FALSE;
		if (enumData.Count == 0) return TRUE;
		if (itemSize == 0 || enumData.Count > (ULONG)-1 / itemSize) {
			lastErrorCode = ERROR_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid enumeration size.";
			return FALSE;
		}

		enumData.BufferSize = enumData.Count * itemSize;
		ULONG capacity = enumData.Count;
		enumData.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, enumData.BufferSize);
		if (!enumData.Buffer) {
			lastErrorCode = ERROR_NOT_ENOUGH_MEMORY;
			lastErrorMessage = L"Failed to allocate enumeration buffer.";
			return FALSE;
		}

		enumData.Count = 0;
		result = SiQueryFileInformation(information, path, &enumData, argument);
		QueryError();
		if (result && enumData.Count > capacity) enumData.Count = capacity;
		return result;
	}

	BOOL KernelInstance::SiTerminateProcess(ULONG pid) noexcept {
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Terminate, pid, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiTerminateProcessEx(ULONG pid) noexcept {
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Terminate, pid, NULL, 2);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiSuspendProcess(ULONG pid) noexcept {
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Suspend, pid, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiResumeProcess(ULONG pid) noexcept {
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Resume, pid, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiHideProcess(ULONG pid) noexcept {
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Hide, pid, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SetPPL(ULONG pid, int level) noexcept {
		SI_PROCESS_PROTECTION in = { PsProtectedTypeProtectedLight, level };
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Protection, pid, &in, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SetCriticalProcess(ULONG pid) noexcept {
		BOOLEAN state = TRUE;
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Critical, pid, &state, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::InjectDLLToProcess(ULONG pid, PWCHAR dllPath, ULONG size) noexcept {
		SI_INJECT_DLL in = { 0 };
		RtlCopyMemory(in.DllPath, dllPath, size < RTL_NUMBER_OF(in.DllPath) ? size : RTL_NUMBER_OF(in.DllPath));
		in.Method = 0;
		BOOL result = SiSetProcessInformation(ProcessSetInformation::InjectDll, pid, &in, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::ModifyProcessToken(ULONG sourcePid, ULONG targetPid) noexcept {
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Token, targetPid, &sourcePid, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiTerminateThread(ULONG tid) noexcept {
		BOOL result = SiSetThreadInformation(ThreadSetInformation::Terminate, tid, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiTerminateThreadEx(ULONG tid) noexcept {
		BOOL result = SiSetThreadInformation(ThreadSetInformation::Terminate, tid, NULL, 1);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiSuspendThread(ULONG tid) noexcept {
		BOOL result = SiSetThreadInformation(ThreadSetInformation::Suspend, tid, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiResumeThread(ULONG tid) noexcept {
		BOOL result = SiSetThreadInformation(ThreadSetInformation::Resume, tid, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiUnloadDriver(ULONG64 driverObj) noexcept {
		if (driverObj == 0) return FALSE;

		SI_UNLOAD_IMAGE input = { 0 };
		input.Base = (PVOID)driverObj;
		input.UnloadAsDriver = TRUE;

		BOOL result = SiSetSystemInformation(SystemSetInformation::UnloadImage, &input, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiHideDriver(ULONG64 driverObj) noexcept {
		lastErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
		lastErrorMessage = L"Not implemented.";
		return FALSE;
	}

	BOOL KernelInstance::QueryFile(std::wstring path, std::vector<winrt::StarlightGUI::FileInfo>& files) noexcept
	{
		if (!GetDriverDevice()) return FALSE;

		WCHAR targetPath[512];
		wcscpy_s(targetPath, L"\\??\\");
		wcscat_s(targetPath, path.c_str());

		PWCHAR pathPtr = targetPath;
		SI_ENUMERATION enumData = { 0 };
		enumData.Arg = (PVOID)&pathPtr;

		BOOL result = FALSE;

		if (enumFileMode == 2)
			result = QueryFileEnumeration(FileGetInformation::DirectoryFileByNTFS, targetPath, enumData, sizeof(SI_FILE_DATA_FULL), (enumFileMode == 3) ? 1 : 0);
		else
			result = QueryFileEnumeration(FileGetInformation::DirectoryFile, targetPath, enumData, sizeof(SI_FILE_DATA), enumFileMode);

		if (result && enumData.Count > 0 && enumData.Buffer) {
			if (enumFileMode == 2) {
				// NTFSPARSER mode
				PSI_FILE_DATA_FULL fileData = (PSI_FILE_DATA_FULL)enumData.Buffer;
				for (ULONG i = 0; i < enumData.Count; i++) {
					auto fileInfo = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
					fileInfo.Name(fileData[i].Name);
					fileInfo.Path(path + L"\\" + std::wstring(fileData[i].Name));
					fileInfo.Directory(fileData[i].Directory);
					fileInfo.Flag(fileData[i].NtfsFlags);
					fileInfo.Size(fileData[i].DataSize);
					fileInfo.MFTID(fileData[i].FileReference);
					files.push_back(fileInfo);
				}
			}
			else {
				// NTAPI or NTFSIO mode
				PSI_FILE_DATA fileData = (PSI_FILE_DATA)enumData.Buffer;
				for (ULONG i = 0; i < enumData.Count; i++) {
					auto fileInfo = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
					fileInfo.Name(fileData[i].Name);
					fileInfo.Path(path + L"\\" + std::wstring(fileData[i].Name));
					fileInfo.Directory(fileData[i].Directory);
					fileInfo.Flag(0);
					fileInfo.Size(fileData[i].DataSize);
					files.push_back(fileInfo);
				}
			}
		}

		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}

	BOOL KernelInstance::SiEnumProcesses(std::vector<winrt::StarlightGUI::ProcessInfo>& targetList, bool strengthen) noexcept {
		SI_ENUMERATION enumData = { 0 };
		ULONG strengthenFlag = 1;
		if (strengthen)
			enumData.Arg = &strengthenFlag;
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::Process, enumData, sizeof(SI_PROCESS_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_PROCESS_DATA processData = (PSI_PROCESS_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto processInfo = winrt::make<winrt::StarlightGUI::implementation::ProcessInfo>();
				processInfo.Id(processData[i].Pid);
				processInfo.Name(to_hstring(processData[i].ImageName));
				processInfo.EProcess((ULONG64)processData[i].Eprocess);
				processInfo.ExecutablePath(to_hstring(processData[i].ImagePath));
				processInfo.MemoryUsage(processData[i].WorkingSetPrivateSize);
				targetList.push_back(processInfo);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumProcessThreads(ULONG pid, std::vector<winrt::StarlightGUI::ThreadInfo>& threads) noexcept {
		SI_ENUMERATION enumData = { 0 };
		enumData.Arg = (PVOID)&pid;
	
		BOOL result = QueryProcessEnumeration(ProcessGetInformation::Thread, pid, enumData, sizeof(SI_THREAD_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_THREAD_DATA threadData = (PSI_THREAD_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto threadInfo = winrt::make<winrt::StarlightGUI::implementation::ThreadInfo>();
				threadInfo.Id(threadData[i].Tid);
				threadInfo.EThread((ULONG64)threadData[i].Ethread);
				threadInfo.Address((ULONG64)threadData[i].StartAddress);
				threadInfo.Win32Address((ULONG64)threadData[i].Win32StartAddress);
				threadInfo.PreviousMode(threadData[i].PreviousMode);
				threadInfo.Priority(threadData[i].Priority);
				threadInfo.Status((ULONG)threadData[i].State);
				threads.push_back(threadInfo);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumProcessHandles(ULONG pid, std::vector<winrt::StarlightGUI::HandleInfo>& handles) noexcept {
		SI_ENUMERATION enumData = { 0 };
		enumData.Arg = (PVOID)&pid;
	
		BOOL result = QueryProcessEnumeration(ProcessGetInformation::Handle, pid, enumData, sizeof(SI_HANDLE_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_HANDLE_DATA handleData = (PSI_HANDLE_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto handleInfo = winrt::make<winrt::StarlightGUI::implementation::HandleInfo>();
				handleInfo.Type(to_hstring(handleData[i].TypeName));
				handleInfo.Object((ULONG64)handleData[i].Object);
				handleInfo.Handle((ULONG64)handleData[i].Handle);
				handleInfo.Access(handleData[i].GrantedAccess);
				handleInfo.Attributes(handleData[i].Attributes);
				handles.push_back(handleInfo);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumProcessModules(ULONG pid, std::vector<winrt::StarlightGUI::MokuaiInfo>& modules) noexcept {
		SI_ENUMERATION enumData = { 0 };
		enumData.Arg = (PVOID)&pid;
	
		BOOL result = QueryProcessEnumeration(ProcessGetInformation::Module, pid, enumData, sizeof(SI_MODULE_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_MODULE_DATA moduleData = (PSI_MODULE_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto moduleInfo = winrt::make<winrt::StarlightGUI::implementation::MokuaiInfo>();
				moduleInfo.Name(to_hstring(moduleData[i].Name));
				moduleInfo.Address((ULONG64)moduleData[i].Base);
				moduleInfo.Size(moduleData[i].Size);
				moduleInfo.Path(to_hstring(moduleData[i].Path));
				modules.push_back(moduleInfo);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumProcessKernelCallbackTable(ULONG pid, std::vector<winrt::StarlightGUI::KCTInfo>& kcts) noexcept {
		SI_ENUMERATION enumData = { 0 };
		enumData.Arg = (PVOID)&pid;
	
		BOOL result = QueryProcessEnumeration(ProcessGetInformation::KernelCallbackTable, pid, enumData, sizeof(SI_FUNCTION_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_FUNCTION_DATA functionData = (PSI_FUNCTION_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto kctInfo = winrt::make<winrt::StarlightGUI::implementation::KCTInfo>();
				kctInfo.Name(to_hstring(functionData[i].Name));
				kctInfo.Address((ULONG64)functionData[i].Address);
				kcts.push_back(kctInfo);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumDrivers(std::vector<winrt::StarlightGUI::KernelModuleInfo>& kernelModules) noexcept {
		SI_ENUMERATION enumData = { 0 };
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::Module, enumData, sizeof(SI_MODULE_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_MODULE_DATA moduleData = (PSI_MODULE_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto di = winrt::make<winrt::StarlightGUI::implementation::KernelModuleInfo>();
				di.Name(to_hstring(moduleData[i].Name));
				di.Path(to_hstring(moduleData[i].Path));
				di.ImageBase((ULONG64)moduleData[i].Base);
				di.Size(moduleData[i].Size);
				di.DriverObject((ULONG64)moduleData[i].DriverObject);
				kernelModules.push_back(di);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumMiniFilter(std::vector<winrt::StarlightGUI::GeneralEntry>& filterList) noexcept {
		SI_ENUMERATION enumData = { 0 };
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::Minifilter, enumData, sizeof(SI_MINIFILTER_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_MINIFILTER_DATA minifilterData = (PSI_MINIFILTER_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(minifilterData[i].Name));
				entry.String2(to_hstring(GetMiniFilterMajorFunction(minifilterData[i].MajorFunction)));
				entry.ULongLong1((ULONG64)minifilterData[i].Base);
				entry.ULongLong2((ULONG64)minifilterData[i].PreOperation);
				entry.ULongLong3((ULONG64)minifilterData[i].PostOperation);
				filterList.push_back(entry);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumSSDT(std::vector<winrt::StarlightGUI::GeneralEntry>& ssdtList) noexcept {
		SI_ENUMERATION enumData = { 0 };
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::SSDT, enumData, sizeof(SI_FUNCTION_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_FUNCTION_DATA functionData = (PSI_FUNCTION_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				std::wstring name = StringToWideString(functionData[i].Name);
				if ((!functionShowDeprecated && name.rfind(L"Deprecated", 0) == 0) ||
					(!functionShowUnknown && name.rfind(L"Unknown", 0) == 0)) {
					continue;
				}

				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(name);
				entry.String2(L"\\SystemRoot\\System32\\ntoskrnl.exe");
				entry.ULongLong1((ULONG64)functionData[i].Address);
				ssdtList.push_back(entry);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumSSSDT(std::vector<winrt::StarlightGUI::GeneralEntry>& sssdtList) noexcept {
		SI_ENUMERATION enumData = { 0 };
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::ShadowSSDT, enumData, sizeof(SI_FUNCTION_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_FUNCTION_DATA functionData = (PSI_FUNCTION_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				std::wstring name = StringToWideString(functionData[i].Name);
				if ((!functionShowDeprecated && name.rfind(L"Deprecated", 0) == 0) ||
					(!functionShowUnknown && name.rfind(L"Unknown", 0) == 0)) {
					continue;
				}

				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(name);
				entry.String2(L"\\SystemRoot\\System32\\win32k.sys");
				entry.ULongLong1((ULONG64)functionData[i].Address);
				sssdtList.push_back(entry);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumIoTimer(std::vector<winrt::StarlightGUI::GeneralEntry>& timerList) noexcept {
		SI_ENUMERATION enumData = { 0 };
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::IOTimer, enumData, sizeof(SI_IO_TIMER_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_IO_TIMER_DATA timerData = (PSI_IO_TIMER_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(timerData[i].Path));
				entry.ULongLong1((ULONG64)timerData[i].TimerRoutine);
				entry.ULongLong2((ULONG64)timerData[i].DeviceObject);
				timerList.push_back(entry);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}

	BOOL KernelInstance::SiEnumDPCTimers(std::vector<winrt::StarlightGUI::GeneralEntry>& timerList) noexcept {
		SI_ENUMERATION enumData = { 0 };

		BOOL result = QuerySystemEnumeration(SystemGetInformation::DPCTimer, enumData, sizeof(SI_DPC_TIMER_DATA));

		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_DPC_TIMER_DATA timerData = (PSI_DPC_TIMER_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(timerData[i].Path));
				entry.ULongLong1((ULONG64)timerData[i].Timer);
				entry.ULongLong2((ULONG64)timerData[i].DPC);
				entry.ULongLong3((ULONG64)timerData[i].DeferredRoutine);
				entry.ULongLong4((ULONG64)timerData[i].DeferredContext);
				entry.Long1(timerData[i].Period);
				timerList.push_back(entry);
			}
		}

		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}

	BOOL KernelInstance::SiEnumEResources(std::vector<winrt::StarlightGUI::GeneralEntry>& resourceList) noexcept {
		SI_ENUMERATION enumData = { 0 };

		BOOL result = QuerySystemEnumeration(SystemGetInformation::Resource, enumData, sizeof(SI_ERESOURCE_DATA));

		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_ERESOURCE_DATA resourceData = (PSI_ERESOURCE_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.ULongLong1((ULONG64)resourceData[i].Resource);
				entry.Long1(resourceData[i].ActiveCount);
				entry.ULong1(resourceData[i].ContentionCount);
				entry.ULong2(resourceData[i].NumberOfSharedWaiters);
				entry.ULong3(resourceData[i].NumberOfExclusiveWaiters);
				entry.ULong4(resourceData[i].Flag);
				resourceList.push_back(entry);
			}
		}

		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumIDT(std::vector<winrt::StarlightGUI::GeneralEntry>& idtList) noexcept {
		SI_ENUMERATION enumData = { 0 };
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::IDT, enumData, sizeof(SI_IDT_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_IDT_DATA idtData = (PSI_IDT_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.ULongLong1((ULONG64)idtData[i].Offset);
				entry.ULong1(i);
				entry.ULong2(idtData[i].Selector);
				entry.ULong3(idtData[i].Type);
				entry.ULong4(idtData[i].Dpl);
				idtList.push_back(entry);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumGDT(std::vector<winrt::StarlightGUI::GeneralEntry>& gdtList) noexcept {
		SI_ENUMERATION enumData = { 0 };
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::GDT, enumData, sizeof(SI_GDT_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_GDT_DATA gdtData = (PSI_GDT_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.ULongLong1((ULONG64)gdtData[i].Base);
				entry.ULongLong2(gdtData[i].Limit);
				entry.ULong1(i);
				entry.ULong2(gdtData[i].Type);
				entry.ULong3(gdtData[i].Dpl);
				entry.ULong4(gdtData[i].Granularity);
				gdtList.push_back(entry);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumPiDDBCacheTable(std::vector<winrt::StarlightGUI::GeneralEntry>& piddbList) noexcept {
		SI_ENUMERATION enumData = { 0 };
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::PiDDBCacheTable, enumData, sizeof(SI_PIDDB_CACHE_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_PIDDB_CACHE_DATA piddbData = (PSI_PIDDB_CACHE_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(piddbData[i].Name));
				entry.ULong1(piddbData[i].LoadStatus);
				entry.ULong2(piddbData[i].Timestamp);
				piddbList.push_back(entry);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	BOOL KernelInstance::SiEnumHalDispatchTable(std::vector<winrt::StarlightGUI::GeneralEntry>& halList, HalTableType type) noexcept {
		SI_ENUMERATION enumData = { 0 };

		SystemGetInformation information = SystemGetInformation::HalDispatchTable;
		switch (type) {
		case HalTableType::HalPrivateDispatchTable:
			information = SystemGetInformation::HalPrivateDispatchTable;
			break;
#ifdef STARLIGHT_PREMIUM
		case HalTableType::HalIommuDispatchTable:
			information = SystemGetInformation::HalIommuDispatchTable;
			break;
		case HalTableType::HalAcpiDispatchTable:
			information = SystemGetInformation::HalAcpiDispatchTable;
			break;
		case HalTableType::HalSubComponents:
			information = SystemGetInformation::HalSubComponents;
			break;
#else
		case HalTableType::HalIommuDispatchTable:
		case HalTableType::HalAcpiDispatchTable:
		case HalTableType::HalSubComponents:
			lastErrorCode = ERROR_NOT_SUPPORTED;
			lastErrorMessage = t(L"Common.PremiumOnly");
			return FALSE;
#endif
		default:
			break;
		}
	
		BOOL result = QuerySystemEnumeration(information, enumData, sizeof(SI_FUNCTION_DATA), functionUseDocumentName ? 1 : 0);
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_FUNCTION_DATA functionData = (PSI_FUNCTION_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				std::wstring name = StringToWideString(functionData[i].Name);
				if ((!functionShowDeprecated && name.rfind(L"Deprecated", 0) == 0) ||
					(!functionShowUnknown && name.rfind(L"Unknown", 0) == 0)) {
					continue;
				}

				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(name);
				entry.String2(L"\\SystemRoot\\System32\\ntoskrnl.exe");
				entry.ULongLong1((ULONG64)functionData[i].Address);
				entry.ULong1((ULONG)type);
				halList.push_back(entry);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}
	
	static hstring CallbackTypeToString(CallbackType type) noexcept
	{
		switch (type) {
		case CallbackType::CreateProcess: return L"CreateProcess";
		case CallbackType::CreateThread: return L"CreateThread";
		case CallbackType::LoadImage: return L"LoadImage";
		case CallbackType::Object: return L"Object";
		case CallbackType::Registry: return L"Registry";
		case CallbackType::PowerSetting: return L"PowerSetting";
		case CallbackType::PlugPlay: return L"PlugPlay";
		case CallbackType::Shutdown: return L"Shutdown";
		case CallbackType::LastChanceShutdown: return L"LastChanceShutdown";
		case CallbackType::FileSystemChange: return L"FileSystemChange";
		case CallbackType::BugCheck: return L"BugCheck";
		case CallbackType::BugCheckReason: return L"BugCheckReason";
		case CallbackType::ExCallback: return L"ExCallback";
		case CallbackType::LogonSessionTerminated: return L"LogonSessionTerminated";
		case CallbackType::LogonSessionTerminatedEx: return L"LogonSessionTerminatedEx";
		case CallbackType::DbgPrint: return L"DbgPrint";
		case CallbackType::IoPriority: return L"IoPriority";
		case CallbackType::Coalescing: return L"Coalescing";
		case CallbackType::ImageVerification: return L"ImageVerification";
		case CallbackType::Nmi: return L"Nmi";
		default: return L"Unknown";
		}
	}

	BOOL KernelInstance::SiEnumCallbacks(std::vector<winrt::StarlightGUI::GeneralEntry>& callbackList, CallbackType type) noexcept {
		SI_ENUMERATION enumData = { 0 };
		ULONG callbackType = (ULONG)type;
		enumData.Arg = &callbackType;
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::Callback, enumData, sizeof(SI_CALLBACK_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_CALLBACK_DATA callbackData = (PSI_CALLBACK_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto callback = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				callback.String1(CallbackTypeToString(type));
				callback.String2(to_hstring(callbackData[i].Path));

				callback.ULong1((ULONG)type);
				callback.ULong2(callbackData[i].Index);
				callback.ULong3(callbackData[i].Flag);
				callback.ULongLong1((ULONG64)callbackData[i].Address);
				callback.ULongLong2((ULONG64)callbackData[i].Address2);
				callback.ULongLong3((ULONG64)callbackData[i].Address3);
				callback.ULongLong4((ULONG64)callbackData[i].Address4);
				callbackList.push_back(callback);
			}
		}
	
		HeapFree(GetProcessHeap(), 0, enumData.Buffer);
		return result;
	}

	BOOL KernelInstance::SiDeleteFile(std::wstring path) noexcept {
		WCHAR targetPath[512];
		wcscpy_s(targetPath, L"\\??\\");
		wcscat_s(targetPath, path.c_str());

		BOOL result = SiSetFileInformation(FileSetInformation::Delete, targetPath, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiDeleteFileEx(std::wstring path) noexcept {
		WCHAR targetPath[512];
		wcscpy_s(targetPath, L"\\??\\");
		wcscat_s(targetPath, path.c_str());

		BOOL status = SiSetFileInformation(FileSetInformation::Delete, targetPath, NULL, 1);
		QueryError();
		return status;
	}

	BOOL KernelInstance::DeleteFileAuto(std::wstring path) noexcept {
		if (!fs::exists(path))
			return FALSE;

		if (!fs::is_directory(path))
			return DeleteFileW(path.c_str());

		for (const auto& entry : fs::directory_iterator(path)) {
			if (fs::is_directory(entry))
				DeleteFileAuto(entry.path().wstring());
			if (fs::is_regular_file(entry))
				DeleteFileW(entry.path().wstring().c_str());
		}
		LOG_INFO(L"KernelInstance", L"Post-deleted directory.");
		return RemoveDirectoryW(path.c_str());
	}

	BOOL KernelInstance::SiLockFile(std::wstring path) noexcept {
		lastErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
		lastErrorMessage = L"Not implemented.";
		return FALSE;
	}

	BOOL KernelInstance::SiCopyFile(std::wstring from, std::wstring to) noexcept {
		WCHAR sourcePath[512];
		wcscpy_s(sourcePath, L"\\??\\");
		wcscat_s(sourcePath, from.c_str());

		WCHAR targetPath[512];
		wcscpy_s(targetPath, L"\\??\\");
		wcscat_s(targetPath, to.c_str());
		PWCHAR pathPtr = targetPath;

		BOOL result = SiSetFileInformation(FileSetInformation::Copy, sourcePath, pathPtr, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiRenameFile(std::wstring from, std::wstring to) noexcept {
		WCHAR sourcePath[512];
		wcscpy_s(sourcePath, L"\\??\\");
		wcscat_s(sourcePath, from.c_str());

		WCHAR targetPath[512];
		wcscpy_s(targetPath, L"\\??\\");
		wcscat_s(targetPath, to.c_str());
		PWCHAR pathPtr = targetPath;

		BOOL result = SiSetFileInformation(FileSetInformation::Rename, sourcePath, pathPtr, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::EnableHypervisor() noexcept {
#ifdef STARLIGHT_PREMIUM
		if (!SiFeatureCollection(FeatureCollection::Virtualization,
			(COLLECTION_ENUM)VirtualizationCollection::CheckSupport, NULL, 0)) {
			QueryError();
			return FALSE;
		}

		METAVERSE_CONFIGURATION configuration = { MetaverseMode::Normal };
		BOOL result = SiFeatureCollection(FeatureCollection::Virtualization,
			(COLLECTION_ENUM)VirtualizationCollection::StartMetaverse, &configuration, 0);
		QueryError();
		return result;
#else
		lastErrorCode = ERROR_NOT_SUPPORTED;
		lastErrorMessage = t(L"Common.PremiumOnly");
		return FALSE;
#endif
	}

	BOOL KernelInstance::DisableHypervisor() noexcept {
#ifdef STARLIGHT_PREMIUM
		BOOL result = SiFeatureCollection(FeatureCollection::Virtualization,
			(COLLECTION_ENUM)VirtualizationCollection::StopMetaverse, NULL, 0);
		QueryError();
		return result;
#else 		
		lastErrorCode = ERROR_NOT_SUPPORTED;
		lastErrorMessage = t(L"Common.PremiumOnly");
		return FALSE;
#endif
	}

	BOOL KernelInstance::EnableCreateProcess() noexcept {
		BOOLEAN state = TRUE;
		BOOL result = SiSetSystemInformation(SystemSetInformation::CreateProcessState, &state, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::DisableCreateProcess() noexcept {
		BOOLEAN state = FALSE;
		BOOL result = SiSetSystemInformation(SystemSetInformation::CreateProcessState, &state, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::EnableCreateFile() noexcept {
		BOOLEAN state = TRUE;
		BOOL result = SiSetSystemInformation(SystemSetInformation::CreateFileState, &state, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::DisableCreateFile() noexcept {
		BOOLEAN state = FALSE;
		BOOL result = SiSetSystemInformation(SystemSetInformation::CreateFileState, &state, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::EnableModifyRegistry() noexcept {
		lastErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
		lastErrorMessage = L"Not implemented";
		return FALSE;
	}

	BOOL KernelInstance::DisableModifyRegistry() noexcept {
		lastErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
		lastErrorMessage = L"Not implemented";
		return FALSE;
	}

	BOOL KernelInstance::EnableDSE(bool hypervisor) noexcept {
#ifndef STARLIGHT_PREMIUM
		if (hypervisor) {
			lastErrorCode = ERROR_NOT_SUPPORTED;
			lastErrorMessage = t(L"Common.PremiumOnly");
			return FALSE;
		}
#endif
		BOOLEAN state = TRUE;
		BOOL result = SiSetSystemInformation(SystemSetInformation::DSEState, &state, hypervisor ? 1 : 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::DisableDSE(bool hypervisor) noexcept {
#ifndef STARLIGHT_PREMIUM
		if (hypervisor) {
			lastErrorCode = ERROR_NOT_SUPPORTED;
			lastErrorMessage = t(L"Common.PremiumOnly");
			return FALSE;
		}
#endif
		BOOLEAN state = FALSE;
		BOOL result = SiSetSystemInformation(SystemSetInformation::DSEState, &state, hypervisor ? 1 : 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::EnableLKD() noexcept {
		BOOLEAN state = TRUE;
		BOOL result = SiSetSystemInformation(SystemSetInformation::LKDState, &state, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::DisablePatchGuard(bool hypervisor) noexcept {
#ifdef STARLIGHT_PREMIUM
		BOOL result = SiSetSystemInformation(SystemSetInformation::DisablePatchGuard, NULL, hypervisor ? 1 : 0);
		QueryError();
		return result;
#else
		lastErrorCode = ERROR_NOT_SUPPORTED;
		lastErrorMessage = t(L"Common.PremiumOnly");
		return FALSE;
#endif
	}

	BOOL KernelInstance::BlueScreen() {
		BOOL result = SiSetSystemInformation(SystemSetInformation::TriggerBugCheck, NULL, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::SiEnumObjectsByDirectory(std::wstring objectPath, std::vector<winrt::StarlightGUI::ObjectEntry>& objectList) noexcept {
		UNICODE_STRING objectName;
		RtlInitUnicodeString(&objectName, objectPath.c_str());

		OBJECT_ATTRIBUTES objectAttributes;
		InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

		HANDLE directoryHandle = NULL;
		LONG status = NtOpenDirectoryObject(&directoryHandle, 0x0001, &objectAttributes);
		if (status < 0 || !directoryHandle) return FALSE;

		ULONG context = 0;
		ULONG returnLength = 0;
		std::vector<BYTE> buffer(0x1000);
		objectList.clear();

		for (;;) {
			status = NtQueryDirectoryObject(directoryHandle, buffer.data(), (ULONG)buffer.size(), FALSE, FALSE, &context, &returnLength);
			if (status < 0) {
				if (returnLength > buffer.size()) {
					buffer.resize(returnLength);
					continue;
				}
				break;
			}

			POBJECT_DIRECTORY_INFORMATION info = (POBJECT_DIRECTORY_INFORMATION)buffer.data();
			while (info->Name.Buffer) {
				winrt::StarlightGUI::ObjectEntry entry = winrt::make<winrt::StarlightGUI::implementation::ObjectEntry>();
				std::wstring name(info->Name.Buffer, info->Name.Length / sizeof(WCHAR));
				std::wstring type(info->TypeName.Buffer, info->TypeName.Length / sizeof(WCHAR));
				hstring path(objectPath + L"\\" + name);

				entry.Name(name);
				entry.Type(type);
				entry.Path(FixBackSplash(path));

				if (type == L"SymbolicLink")
					KernelInstance::GetObjectDetails(entry.Path().c_str(), type, entry);

				objectList.push_back(entry);
				info++;
			}
		}

		CloseHandle(directoryHandle);
		return status == (LONG)0x8000001A;
	}

	BOOL KernelInstance::GetObjectDetails(std::wstring fullPath, std::wstring type, winrt::StarlightGUI::ObjectEntry& entry) noexcept {
		HANDLE objectHandle = NULL;
		LONG status = 0L;
		ULONG returnLength = 0;

		UNICODE_STRING objectName;
		RtlInitUnicodeString(&objectName, fullPath.c_str());

		OBJECT_ATTRIBUTES objectAttributes;
		InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

		if (type == L"Directory")
			status = NtOpenDirectoryObject(&objectHandle, 0x0001, &objectAttributes);
		else if (type == L"SymbolicLink")
			status = NtOpenSymbolicLinkObject(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"Event")
			status = NtOpenEvent(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"Mutant")
			status = NtOpenMutant(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"Semaphore")
			status = NtOpenSemaphore(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"Section")
			status = NtOpenSection(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"Timer")
			status = NtOpenTimer(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"Device") {
			IO_STATUS_BLOCK ioStatus = { 0 };
			status = NtOpenFile(&objectHandle, GENERIC_READ, &objectAttributes, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE, 0x00000040);
		}
		else if (type == L"Session")
			status = NtOpenSession(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"CpuPartition")
			status = NtOpenCpuPartition(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"Job")
			status = NtOpenJobObject(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"IoCompletion")
			status = NtOpenIoCompletion(&objectHandle, GENERIC_READ, &objectAttributes);
		else if (type == L"Partition")
			status = NtOpenPartition(&objectHandle, GENERIC_READ, &objectAttributes);
		else
			return FALSE;

		if (status < 0 || !objectHandle) return FALSE;

		OBJECT_BASIC_INFORMATION basicInfo{};
		status = NtQueryObject(objectHandle, 0, &basicInfo, sizeof(basicInfo), &returnLength);

		if (status >= 0) {
			entry.Permanent((basicInfo.Attributes & OBJ_PERMANENT) != 0);
			entry.References(basicInfo.PointerCount);
			entry.Handles(basicInfo.HandleCount);
			entry.PagedPool(basicInfo.PagedPoolCharge);
			entry.NonPagedPool(basicInfo.NonPagedPoolCharge);
			FILETIME ft = { basicInfo.CreationTime.LowPart, basicInfo.CreationTime.HighPart };
			SYSTEMTIME st;
			if (FileTimeToSystemTime(&ft, &st))
			{
				std::wstringstream ss;
				ss << std::setw(4) << std::setfill(L'0') << st.wYear << L"/"
					<< std::setw(2) << std::setfill(L'0') << st.wMonth << L"/"
					<< std::setw(2) << std::setfill(L'0') << st.wDay << L" "
					<< std::setw(2) << std::setfill(L'0') << st.wHour << L":"
					<< std::setw(2) << std::setfill(L'0') << st.wMinute << L":"
					<< std::setw(2) << std::setfill(L'0') << st.wSecond;
				entry.CreationTime(ss.str());
			}
			else
			{
				entry.CreationTime(t(L"Common.Unknown"));
			}

			ULONG bufferLength = 0;
			if (type == L"SymbolicLink") {
				UNICODE_STRING target{};

				status = NtQuerySymbolicLinkObject(objectHandle, &target, &bufferLength);

				if (status < 0) {
					target.Buffer = (PWSTR)HeapAlloc(GetProcessHeap(), 0, bufferLength);
					target.Length = 0;
					target.MaximumLength = (USHORT)bufferLength;

					status = NtQuerySymbolicLinkObject(objectHandle, &target, &bufferLength);
					if (status >= 0)
						entry.Link(std::wstring(target.Buffer, target.Length / sizeof(WCHAR)));
					HeapFree(GetProcessHeap(), 0, target.Buffer);
				}
			}
			else if (type == L"Event") {
				EVENT_BASIC_INFORMATION eventInfo{};

				status = NtQueryEvent(objectHandle, EventBasicInformation, &eventInfo, sizeof(eventInfo), &bufferLength);
				if (status >= 0) {
					entry.EventType(eventInfo.EventType == NotificationEvent ? L"Notification (Manual reset)" : L"Synchronization (Auto reset)");
					entry.EventSignaled(eventInfo.EventState == 0 ? FALSE : TRUE);
				}
			}
			else if (type == L"Mutant") {
				MUTANT_BASIC_INFORMATION mutantInfo{};

				status = NtQueryMutant(objectHandle, MutantBasicInformation, &mutantInfo, sizeof(mutantInfo), &bufferLength);
				if (status >= 0) {
					entry.MutantHoldCount(mutantInfo.CurrentCount);
					entry.MutantAbandoned(mutantInfo.AbandonedState == 0 ? FALSE : TRUE);
				}
			}
			else if (type == L"Semaphore") {
				SEMAPHORE_BASIC_INFORMATION semaphoreInfo{};

				status = NtQuerySemaphore(objectHandle, SemaphoreBasicInformation, &semaphoreInfo, sizeof(semaphoreInfo), &bufferLength);
				if (status >= 0) {
					entry.SemaphoreCount(semaphoreInfo.CurrentCount);
					entry.SemaphoreLimit(semaphoreInfo.MaximumCount);
				}
			}
			else if (type == L"Section") {
				SECTION_BASIC_INFORMATION sectionInfo{};

				status = NtQuerySection(objectHandle, SectionBasicInformation, &sectionInfo, sizeof(sectionInfo), NULL);
				if (status >= 0) {
					entry.SectionBaseAddress((ULONG64)sectionInfo.BaseAddress);
					entry.SectionMaximumSize(sectionInfo.MaximumSize.QuadPart);
					entry.SectionAttributes(sectionInfo.AllocationAttributes);
				}
			}
			else if (type == L"Timer") {
				TIMER_BASIC_INFORMATION timerInfo{};
				status = NtQueryTimer(objectHandle, TimerBasicInformation, &timerInfo, sizeof(timerInfo), &bufferLength);
				if (status >= 0) {
					entry.TimerRemainingTime(timerInfo.RemainingTime.QuadPart);
					entry.TimerState(timerInfo.TimerState);
				}
			}
			else if (type == L"IoCompletion") {
				IO_COMPLETION_BASIC_INFORMATION ioCompletionInfo{};

				status = NtQueryIoCompletion(objectHandle, IoCompletionBasicInformation, &ioCompletionInfo, sizeof(ioCompletionInfo), &bufferLength);
				if (status >= 0)
					entry.IoCompletionDepth(ioCompletionInfo.Depth);
			}
		}

		CloseHandle(objectHandle);
		return status >= 0;
	}

	BOOL KernelInstance::RemoveCallback(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		SI_REMOVE_CALLBACK input = { 0 };
		input.Type = entry.ULong1();
		input.Address = (PVOID)entry.ULongLong1();
		input.Address2 = (PVOID)entry.ULongLong2();

		BOOL result = SiSetSystemInformation(SystemSetInformation::RemoveCallback, &input, 0);
		QueryError();
		return result;
	}

	BOOL KernelInstance::RemoveMiniFilter(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		lastErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
		lastErrorMessage = L"Not implemented";
		return FALSE;
	}

	BOOL KernelInstance::RemovePiDDBCache(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
#ifdef STARLIGHT_PREMIUM
		SI_REMOVE_PIDDB_CACHE input = { 0 };
		wcsncpy_s(input.Name, entry.String1().c_str(), _TRUNCATE);
		input.Timestamp = entry.ULong2();

		BOOL result = SiSetSystemInformation(SystemSetInformation::RemoveFromPiDDBCacheTable, &input, 0);
		QueryError();
		return result;
#else
		lastErrorCode = ERROR_NOT_SUPPORTED;
		lastErrorMessage = t(L"Common.PremiumOnly");
		return FALSE;
#endif
	}

	BOOL KernelInstance::ReadMemory(std::vector<BYTE>& data, PVOID address, ULONG size) noexcept {
		data.clear();
		if (!address || !size || size > (ULONG)-1 - FIELD_OFFSET(SI_MEMORY, Data)) {
			lastErrorCode = ERROR_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid memory read parameter.";
			return FALSE;
		}

		ULONG bufferSize = FIELD_OFFSET(SI_MEMORY, Data) + size;
		PSI_MEMORY input = (PSI_MEMORY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize);
		if (!input) {
			lastErrorCode = ERROR_NOT_ENOUGH_MEMORY;
			lastErrorMessage = L"Failed to allocate memory read buffer.";
			return FALSE;
		}

		input->Address = address;
		input->Size = size;

		BOOL result = SiQuerySystemInformation(SystemGetInformation::ReadMemory, input, 0);
		QueryError();
		if (result)
			data.assign(input->Data, input->Data + size);

		HeapFree(GetProcessHeap(), 0, input);
		return result;
	}

	BOOL KernelInstance::WriteMemory(PVOID address, PVOID data, ULONG size) noexcept {
		if (!address || !data || !size || size > (ULONG)-1 - FIELD_OFFSET(SI_MEMORY, Data)) {
			lastErrorCode = ERROR_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid memory write parameter.";
			return FALSE;
		}

		ULONG bufferSize = FIELD_OFFSET(SI_MEMORY, Data) + size;
		PSI_MEMORY input = (PSI_MEMORY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize);
		if (!input) {
			lastErrorCode = ERROR_NOT_ENOUGH_MEMORY;
			lastErrorMessage = L"Failed to allocate memory write buffer.";
			return FALSE;
		}

		input->Address = address;
		input->Size = size;
		RtlCopyMemory(input->Data, data, size);

		BOOL result = SiSetSystemInformation(SystemSetInformation::WriteMemory, input, 0);
		QueryError();

		HeapFree(GetProcessHeap(), 0, input);
		return result;
	}

	// =================================
	//				PRIVATE
	// =================================

	/*
	* 获取驱动设备位置
	*/
	BOOL KernelInstance::GetDriverDevice() noexcept {
		if (driverDevice != NULL) return TRUE;
		if (!DriverUtils::LoadKernelDriver(siriusPath.c_str())) return FALSE;

		HANDLE device = CreateFile(L"\\\\.\\Sirius", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (device == INVALID_HANDLE_VALUE) {
			LOG_ERROR(L"Sirius", L"Failed to create Sirius device handle!");
			return FALSE;
		}

		driverDevice = device;
		return TRUE;
	}

	BOOL KernelInstance::SiSetProcessInformation(ProcessSetInformation processInformation, ULONG pid, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_PROCESS_INFORMATION request = { (ULONG)processInformation, pid, buffer, argument };
		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_SET_PROCESS_INFORMATION, &request, sizeof(SI_PROCESS_INFORMATION), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::SiQueryProcessInformation(ProcessGetInformation processInformation, ULONG pid, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_PROCESS_INFORMATION request = { (ULONG)processInformation, pid, buffer, argument };
		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_QUERY_PROCESS_INFORMATION, &request, sizeof(SI_PROCESS_INFORMATION), &request, sizeof(SI_PROCESS_INFORMATION), 0, NULL);

		return status;
	}

	BOOL KernelInstance::SiSetThreadInformation(ThreadSetInformation threadInformation, ULONG tid, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_THREAD_INFORMATION request = { (ULONG)threadInformation, tid, buffer, argument };
		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_SET_THREAD_INFORMATION, &request, sizeof(SI_THREAD_INFORMATION), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::SiQueryThreadInformation(ThreadGetInformation threadInformation, ULONG tid, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_THREAD_INFORMATION request = { (ULONG)threadInformation, tid, buffer, argument };
		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_QUERY_THREAD_INFORMATION, &request, sizeof(SI_THREAD_INFORMATION), &request, sizeof(SI_THREAD_INFORMATION), 0, NULL);

		return status;
	}

	BOOL KernelInstance::SiSetFileInformation(FileSetInformation fileInformation, LPCWSTR filePath, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_FILE_INFORMATION request = { 0 };
		request.FileInformation = (ULONG)fileInformation;
		wcsncpy_s(request.File, filePath, _TRUNCATE);
		request.Buffer = buffer;
		request.Argument = argument;

		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_SET_FILE_INFORMATION, &request, sizeof(SI_FILE_INFORMATION), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::SiQueryFileInformation(FileGetInformation fileInformation, LPCWSTR filePath, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_FILE_INFORMATION request = { 0 };
		request.FileInformation = (ULONG)fileInformation;
		wcsncpy_s(request.File, filePath, _TRUNCATE);
		request.Buffer = buffer;
		request.Argument = argument;

		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_QUERY_FILE_INFORMATION, &request, sizeof(SI_FILE_INFORMATION), &request, sizeof(SI_FILE_INFORMATION), 0, NULL);

		return status;
	}

	BOOL KernelInstance::SiSetSystemInformation(SystemSetInformation systemInformation, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_SYSTEM_INFORMATION request = { 0 };
		request.SystemInformation = (ULONG)systemInformation;
		request.Buffer = buffer;
		request.Argument = argument;

		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_SET_SYSTEM_INFORMATION, &request, sizeof(SI_SYSTEM_INFORMATION), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::SiQuerySystemInformation(SystemGetInformation systemInformation, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_SYSTEM_INFORMATION request = { 0 };
		request.SystemInformation = (ULONG)systemInformation;
		request.Buffer = buffer;
		request.Argument = argument;

		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_QUERY_SYSTEM_INFORMATION, &request, sizeof(SI_SYSTEM_INFORMATION), &request, sizeof(SI_SYSTEM_INFORMATION), 0, NULL);

		return status;
	}

	BOOL KernelInstance::SiFeatureCollection(FeatureCollection collection, COLLECTION_ENUM subCollection, PVOID buffer, ULONG argument) noexcept {
		if (!GetDriverDevice()) return FALSE;

		SI_COLLECTION_INFORMATION request = { 0 };
		request.CollectionInformation = (FEATURE_ENUM)collection;
		request.SubCollectionInformation = subCollection;
		request.Buffer = buffer;
		request.Argument = argument;

		BOOL status = DeviceIoControl(driverDevice, IOCTL_SIRIUS_FEATURE_COLLECTION,
			&request, sizeof(SI_COLLECTION_INFORMATION), &request, sizeof(SI_COLLECTION_INFORMATION), 0, NULL);

		return status;
	}

	std::string KernelInstance::GetMiniFilterMajorFunction(ULONG64 index) noexcept {
		switch (index) {
		case 0: return "IRP_MJ_CREATE";
		case 1: return "IRP_MJ_CREATE_NAMED_PIPE";
		case 2: return "IRP_MJ_CLOSE";
		case 3: return "IRP_MJ_READ";
		case 4: return "IRP_MJ_WRITE";
		case 5: return "IRP_MJ_QUERY_INFORMATION";
		case 6: return "IRP_MJ_SET_INFORMATION";
		case 7: return "IRP_MJ_QUERY_EA";
		case 8: return "IRP_MJ_SET_EA";
		case 9: return "IRP_MJ_FLUSH_BUFFERS";
		case 10: return "IRP_MJ_QUERY_VOLUME_INFORMATION";
		case 11: return "IRP_MJ_SET_VOLUME_INFORMATION";
		case 12: return "IRP_MJ_DIRECTORY_CONTROL";
		case 13: return "IRP_MJ_FILE_SYSTEM_CONTROL";
		case 14: return "IRP_MJ_DEVICE_CONTROL";
		case 15: return "IRP_MJ_INTERNAL_DEVICE_CONTROL";
		case 16: return "IRP_MJ_SHUTDOWN";
		case 17: return "IRP_MJ_LOCK_CONTROL";
		case 18: return "IRP_MJ_CLEANUP";
		case 19: return "IRP_MJ_CREATE_MAILSLOT";
		case 20: return "IRP_MJ_QUERY_SECURITY";
		case 21: return "IRP_MJ_SET_SECURITY";
		case 22: return "IRP_MJ_POWER";
		case 23: return "IRP_MJ_SYSTEM_CONTROL";
		case 24: return "IRP_MJ_DEVICE_CHANGE";
		case 25: return "IRP_MJ_QUERY_QUOTA";
		case 26: return "IRP_MJ_SET_QUOTA";
		case 27: return "IRP_MJ_PNP";
		case 28: return "IRP_MJ_PNP_POWER";
		case 128: return "IRP_MJ_OPERATION_END";
		case 236: return "IRP_MJ_VOLUME_DISMOUNT";
		case 237: return "IRP_MJ_VOLUME_MOUNT";
		case 238: return "IRP_MJ_MDL_WRITE_COMPLETE";
		case 239: return "IRP_MJ_PREPARE_MDL_WRITE";
		case 240: return "IRP_MJ_MDL_READ_COMPLETE";
		case 241: return "IRP_MJ_MDL_READ";
		case 242: return "IRP_MJ_NETWORK_QUERY_OPEN";
		case 243: return "IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE";
		case 249: return "IRP_MJ_QUERY_OPEN";
		case 250: return "IRP_MJ_RELEASE_FOR_CC_FLUSH";
		case 251: return "IRP_MJ_ACQUIRE_FOR_CC_FLUSH";
		case 252: return "IRP_MJ_RELEASE_FOR_MOD_WRITE";
		case 253: return "IRP_MJ_ACQUIRE_FOR_MOD_WRITE";
		case 254: return "IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION";
		case 255: return "IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION";
		default: return "UNKNOWN(" + std::to_string(index) + ")";
		}
	}
}
