#include "pch.h"
#include "KernelBase.h"
#include "CppUtils.h"
#include <string>

namespace winrt::StarlightGUI::implementation {
	static HANDLE driverDevice = NULL;
	static SISTATUS lastErrorCode = SI_SUCCESS;
	static std::wstring lastErrorMessage = L"";

	SISTATUS KernelInstance::GetLastErrorCode() noexcept {
		return lastErrorCode;
	}

	std::wstring KernelInstance::GetLastErrorMessage() noexcept {
		return lastErrorMessage;
	}

	void KernelInstance::QueryError() noexcept {
		if (driverDevice == NULL) {
			lastErrorCode = SI_ERROR;
			lastErrorMessage = L"Driver device not initialized.";
			return;
		}

		SISTATUS errorCode = SI_SUCCESS;
		if (!DeviceIoControl(driverDevice, IOCTL_SIRIUS_GET_ERROR_CODE, NULL, 0, &errorCode, sizeof(SISTATUS), 0, NULL)) {
			lastErrorCode = SI_ERROR;
			lastErrorMessage = L"Failed to query error code from driver.";
			return;
		}

		lastErrorCode = errorCode;

		if (ERROR(errorCode)) {
			SI_ERROR_DETAIL errorDetail = { 0 };
			if (DeviceIoControl(driverDevice, IOCTL_SIRIUS_GET_ERROR_DETAIL, NULL, 0, &errorDetail, sizeof(SI_ERROR_DETAIL), 0, NULL)) {
				lastErrorMessage = std::wstring(errorDetail.Data);
			} else {
				lastErrorMessage = L"Failed to query error detail from driver.";
			}
		} else {
			lastErrorMessage = L"";
		}
	}

	BOOL KernelInstance::QuerySystemEnumeration(SystemGetInformation information, SI_ENUMERATION& enumData, ULONG itemSize, ULONG argument) noexcept {
		enumData.Buffer = NULL;
		enumData.BufferSize = 0;
		enumData.Count = 0;

		BOOL result = SiQuerySystemInformation(information, &enumData, argument);
		QueryError();
		if (!result) return FALSE;
		if (enumData.Count == 0) return TRUE;
		if (itemSize == 0 || enumData.Count > static_cast<ULONG>(-1) / itemSize) {
			lastErrorCode = SI_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid enumeration size.";
			return FALSE;
		}

		enumData.BufferSize = enumData.Count * itemSize;
		ULONG capacity = enumData.Count;
		enumData.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, enumData.BufferSize);
		if (!enumData.Buffer) {
			lastErrorCode = SI_ALLOCATION_FAILED;
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
		if (itemSize == 0 || enumData.Count > static_cast<ULONG>(-1) / itemSize) {
			lastErrorCode = SI_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid enumeration size.";
			return FALSE;
		}

		enumData.BufferSize = enumData.Count * itemSize;
		ULONG capacity = enumData.Count;
		enumData.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, enumData.BufferSize);
		if (!enumData.Buffer) {
			lastErrorCode = SI_ALLOCATION_FAILED;
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
		if (itemSize == 0 || enumData.Count > static_cast<ULONG>(-1) / itemSize) {
			lastErrorCode = SI_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid enumeration size.";
			return FALSE;
		}

		enumData.BufferSize = enumData.Count * itemSize;
		ULONG capacity = enumData.Count;
		enumData.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, enumData.BufferSize);
		if (!enumData.Buffer) {
			lastErrorCode = SI_ALLOCATION_FAILED;
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

	BOOL KernelInstance::ModifyProcessToken(ULONG sourcePID, ULONG targetPID) noexcept {
		BOOL result = SiSetProcessInformation(ProcessSetInformation::Token, targetPID, &sourcePID, 0);
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
		lastErrorCode = SI_NOT_IMPLEMENTED;
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

		if (enum_file_mode == 2) {
			result = QueryFileEnumeration(FileGetInformation::DirectoryFileByNTFS, targetPath, enumData, sizeof(SI_FILE_DATA_FULL), (enum_file_mode == 3) ? 1 : 0);
		}
		else {
			result = QueryFileEnumeration(FileGetInformation::DirectoryFile, targetPath, enumData, sizeof(SI_FILE_DATA), enum_file_mode);
		}

		if (result && enumData.Count > 0 && enumData.Buffer) {
			if (enum_file_mode == 2) {
				// NTFSPARSER mode
				PSI_FILE_DATA_FULL fileData = (PSI_FILE_DATA_FULL)enumData.Buffer;
				for (ULONG i = 0; i < enumData.Count; i++) {
					auto fileInfo = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
					fileInfo.Name(fileData[i].Name);
					fileInfo.Path(path + L"\\" + std::wstring(fileData[i].Name));
					fileInfo.Directory(fileData[i].Directory);
					fileInfo.Flag(fileData[i].NtfsFlags);
					fileInfo.Size(FormatMemorySize(fileData[i].DataSize));
					fileInfo.SizeULong(fileData[i].DataSize);
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
					fileInfo.Size(FormatMemorySize(fileData[i].DataSize));
					fileInfo.SizeULong(fileData[i].DataSize);
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
		if (strengthen) {
			enumData.Arg = &strengthenFlag;
		}
	
		BOOL result = QuerySystemEnumeration(SystemGetInformation::Process, enumData, sizeof(SI_PROCESS_DATA));
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_PROCESS_DATA processData = (PSI_PROCESS_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				auto pi = winrt::make<winrt::StarlightGUI::implementation::ProcessInfo>();
				pi.Id(processData[i].Pid);
				pi.Name(to_hstring(processData[i].ImageName));
				pi.EProcess(ULongToHexString((ULONG64)processData[i].Eprocess));
				pi.EProcessULong((ULONG64)processData[i].Eprocess);
				pi.ExecutablePath(to_hstring(processData[i].ImagePath));
				pi.MemoryUsageByte(processData[i].WorkingSetPrivateSize);
				targetList.push_back(pi);
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
				threadInfo.EThread(ULongToHexString((ULONG64)threadData[i].Ethread));
				threadInfo.Address(ULongToHexString((ULONG64)threadData[i].StartAddress));
				threadInfo.Win32Address(ULongToHexString((ULONG64)threadData[i].Win32StartAddress));
				threadInfo.PreviousMode(threadData[i].PreviousMode == 0 ? L"KernelMode" : L"UserMode");
				threadInfo.Priority(threadData[i].Priority);
	
				switch (threadData[i].State) {
				case Initialized:
					threadInfo.Status(t(L"Msg.Thread.Initialized"));
					break;
				case Ready:
					threadInfo.Status(t(L"Msg.Thread.Ready"));
					break;
				case Running:
					threadInfo.Status(t(L"Msg.Thread.Running"));
					break;
				case Standby:
					threadInfo.Status(t(L"Msg.Thread.Standby"));
					break;
				case Terminated:
					threadInfo.Status(t(L"Msg.Thread.Terminated"));
					break;
				case Waiting:
					threadInfo.Status(t(L"Msg.Thread.Waiting"));
					break;
				case KTHREAD_STATE::Transition:
					threadInfo.Status(t(L"Msg.Thread.Transition"));
					break;
				case DeferredReady:
					threadInfo.Status(t(L"Msg.Thread.DeferredReady"));
					break;
				case GateWaitObsolete:
					threadInfo.Status(t(L"Msg.Thread.GateWait"));
					break;
				default:
					threadInfo.Status(t(L"Msg.Thread.Unknown"));
					break;
				}
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
				handleInfo.Object(ULongToHexString((ULONG64)handleData[i].Object));
				handleInfo.Handle(ULongToHexString((ULONG64)handleData[i].Handle));
				handleInfo.Access(ULongToHexString(handleData[i].GrantedAccess, 0, false, true));
				handleInfo.Attributes(ULongToHexString(handleData[i].Attributes, 0, false, true));
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
				moduleInfo.Address(ULongToHexString((ULONG64)moduleData[i].Base));
				moduleInfo.Size(ULongToHexString(moduleData[i].Size, 0, false, true));
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
				kctInfo.Address(ULongToHexString((ULONG64)functionData[i].Address));
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
				di.ImageBase(ULongToHexString((ULONG64)moduleData[i].Base));
				di.ImageBaseULong((ULONG64)moduleData[i].Base);
				di.Size(ULongToHexString(moduleData[i].Size, 0, false, true));
				di.SizeULong(moduleData[i].Size);
				di.DriverObject(ULongToHexString((ULONG64)moduleData[i].DriverObject));
				di.DriverObjectULong((ULONG64)moduleData[i].DriverObject);
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
				entry.String3(ULongToHexString((ULONG64)minifilterData[i].PreOperation));
				entry.String4(ULongToHexString((ULONG64)minifilterData[i].PostOperation));
				entry.String5(ULongToHexString((ULONG64)minifilterData[i].Base));
				entry.ULongLong1((ULONG64)minifilterData[i].PreOperation);
				entry.ULongLong2((ULONG64)minifilterData[i].PostOperation);
				entry.ULongLong3((ULONG64)minifilterData[i].Base);
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
				if ((!function_show_deprecated && name.rfind(L"Deprecated", 0) == 0) ||
					(!function_show_unknown && name.rfind(L"Unknown", 0) == 0)) {
					continue;
				}

				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(name);
				entry.String2(L"\\SystemRoot\\System32\\ntoskrnl.exe");
				entry.String3(ULongToHexString((ULONG64)functionData[i].Address));
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
				if ((!function_show_deprecated && name.rfind(L"Deprecated", 0) == 0) ||
					(!function_show_unknown && name.rfind(L"Unknown", 0) == 0)) {
					continue;
				}

				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(name);
				entry.String2(L"\\SystemRoot\\System32\\win32k.sys");
				entry.String3(ULongToHexString((ULONG64)functionData[i].Address));
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
				entry.String2(ULongToHexString((ULONG64)timerData[i].TimerRoutine));
				entry.String3(ULongToHexString((ULONG64)timerData[i].DeviceObject));
				entry.ULongLong1((ULONG64)timerData[i].TimerRoutine);
				entry.ULongLong2((ULONG64)timerData[i].DeviceObject);
				timerList.push_back(entry);
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
				entry.String1(ULongToHexString((ULONG64)idtData[i].Offset));
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
				entry.String2(ULongToHexString((ULONG64)gdtData[i].Base));
				entry.String3(ULongToHexString(gdtData[i].Limit));
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
			lastErrorCode = SI_NOT_AVAILABLE;
			lastErrorMessage = t(L"Common.PremiumOnly");
			return FALSE;
#endif
		default:
			break;
		}
	
		BOOL result = QuerySystemEnumeration(information, enumData, sizeof(SI_FUNCTION_DATA), function_use_document_name ? 1 : 0);
	
		if (result && enumData.Count > 0 && enumData.Buffer) {
			PSI_FUNCTION_DATA functionData = (PSI_FUNCTION_DATA)enumData.Buffer;
			for (ULONG i = 0; i < enumData.Count; i++) {
				std::wstring name = StringToWideString(functionData[i].Name);
				if ((!function_show_deprecated && name.rfind(L"Deprecated", 0) == 0) ||
					(!function_show_unknown && name.rfind(L"Unknown", 0) == 0)) {
					continue;
				}

				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(name);
				entry.String2(L"\\SystemRoot\\System32\\ntoskrnl.exe");
				entry.String3(ULongToHexString((ULONG64)functionData[i].Address));
				entry.ULongLong1((ULONG64)functionData[i].Address);
				entry.ULong1(static_cast<ULONG>(type));
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

	static hstring ObCallbackTypeToString(ULONG flag) noexcept
	{
		switch ((ObCallbackType)flag) {
		case ObCallbackType::Process: return L"Process";
		case ObCallbackType::Thread: return L"Thread";
		case ObCallbackType::Desktop: return L"Desktop";
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
				callback.String3(ULongToHexString((ULONG64)callbackData[i].Address));
				callback.String4(ULongToHexString((ULONG64)callbackData[i].Address2));
				callback.String5(ULongToHexString((ULONG64)callbackData[i].Address3));
				callback.String6(ULongToHexString((ULONG64)callbackData[i].Address4));
				if (type == CallbackType::Object) {
					callback.String6(ObCallbackTypeToString(callbackData[i].Flag));
				}
				else if (type == CallbackType::CreateProcess || type == CallbackType::CreateThread || type == CallbackType::LoadImage ||
					type == CallbackType::LogonSessionTerminated || type == CallbackType::DbgPrint) {
					callback.String4(std::to_wstring(callbackData[i].Index));
					callback.String5(ULongToHexString(callbackData[i].Flag, 0, false, true));
					callback.String6(L"");
				}
				else if (type == CallbackType::BugCheck) {
					callback.String6(ULongToHexString((ULONG64)callbackData[i].Address4, 0, false, true));
				}
				else if (type == CallbackType::BugCheckReason) {
					callback.String4(ULongToHexString((ULONG64)callbackData[i].Address3));
					callback.String5(ULongToHexString((ULONG64)callbackData[i].Address4, 0, false, true));
					callback.String6(ULongToHexString((ULONG64)callbackData[i].Address2, 0, false, true));
				}

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
		if (!fs::exists(path)) {
			return FALSE;
		}

		if (!fs::is_directory(path)) {
			return DeleteFileW(path.c_str());
		}
		else {
			for (const auto& entry : fs::directory_iterator(path)) {
				if (fs::is_directory(entry)) {
					DeleteFileAuto(entry.path().wstring());
				}
				if (fs::is_regular_file(entry)) {
					DeleteFileW(entry.path().wstring().c_str());
				}
			}
			LOG_INFO(L"KernelInstance", L"Post-deleted directory.");
			return RemoveDirectoryW(path.c_str());
		}
	}

	BOOL KernelInstance::SiLockFile(std::wstring path) noexcept {
		lastErrorCode = SI_NOT_IMPLEMENTED;
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
		if (!GetDriverDevice()) return FALSE;

		if (!DeviceIoControl(driverDevice, IOCTL_METAVERSE_CHECK_SUPPORT, NULL, 0, NULL, 0, 0, NULL)) {
			QueryError();
			return FALSE;
		}

		BOOL result = DeviceIoControl(driverDevice, IOCTL_METAVERSE_INITIALIZE, NULL, 0, NULL, 0, NULL, NULL);
		QueryError();
		return result;
#else
		lastErrorCode = SI_NOT_AVAILABLE;
		lastErrorMessage = t(L"Common.PremiumOnly");
		return FALSE;
#endif
	}

	BOOL KernelInstance::DisableHypervisor() noexcept {
#ifdef STARLIGHT_PREMIUM
		if (!GetDriverDevice()) return FALSE;

		BOOL result = DeviceIoControl(driverDevice, IOCTL_METAVERSE_EXIT, NULL, 0, NULL, 0, NULL, NULL);
		QueryError();
		return result;
#else 		
		lastErrorCode = SI_NOT_AVAILABLE;
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
		lastErrorCode = SI_NOT_IMPLEMENTED;
		lastErrorMessage = L"Not implemented";
		return FALSE;
	}

	BOOL KernelInstance::DisableModifyRegistry() noexcept {
		lastErrorCode = SI_NOT_IMPLEMENTED;
		lastErrorMessage = L"Not implemented";
		return FALSE;
	}

	BOOL KernelInstance::EnableDSE(bool hypervisor) noexcept {
#ifndef STARLIGHT_PREMIUM
		if (hypervisor) {
			lastErrorCode = SI_NOT_AVAILABLE;
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
			lastErrorCode = SI_NOT_AVAILABLE;
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
		lastErrorCode = SI_NOT_AVAILABLE;
		lastErrorMessage = t(L"Common.PremiumOnly");
		return FALSE;
#endif
	}

	BOOL KernelInstance::BlueScreen() {
		BOOL result = SiSetSystemInformation(SystemSetInformation::TriggerBugCheck, NULL, 0);
		QueryError();
		return result;
	}

	static NtQueryDirectoryObject_t NtQueryDirectoryObject = nullptr;
	static NtQuerySymbolicLinkObject_t NtQuerySymbolicLinkObject = nullptr;
	static NtQueryEvent_t NtQueryEvent = nullptr;
	static NtQueryMutant_t NtQueryMutant = nullptr;
	static NtQuerySemaphore_t NtQuerySemaphore = nullptr;
	static NtQuerySection_t NtQuerySection = nullptr;
	static NtQueryTimer_t NtQueryTimer = nullptr;
	static NtQueryIoCompletion_t NtQueryIoCompletion = nullptr;
	static NtOpenDirectoryObject_t NtOpenDirectoryObject = nullptr;
	static NtOpenSymbolicLinkObject_t NtOpenSymbolicLinkObject = nullptr;
	static NtOpenEvent_t NtOpenEvent = nullptr;
	static NtOpenMutant_t NtOpenMutant = nullptr;
	static NtOpenSemaphore_t NtOpenSemaphore = nullptr;
	static NtOpenSection_t NtOpenSection = nullptr;
	static NtOpenTimer_t NtOpenTimer = nullptr;
	static NtOpenFile_t NtOpenFile = nullptr;
	static NtOpenSession_t NtOpenSession = nullptr;
	static NtOpenCpuPartition_t NtOpenCpuPartition = nullptr;
	static NtOpenJobObject_t NtOpenJobObject = nullptr;
	static NtOpenIoCompletion_t NtOpenIoCompletion = nullptr;
	static NtOpenPartition_t NtOpenPartition = nullptr;

	BOOL KernelInstance::SiEnumObjectsByDirectory(std::wstring objectPath, std::vector<winrt::StarlightGUI::ObjectEntry>& objectList) noexcept {
		if (!NtQueryDirectoryObject || !NtQuerySymbolicLinkObject || !NtQueryEvent || !NtQueryMutant || !NtQuerySemaphore || !NtQuerySection || !NtQueryTimer || !NtQueryIoCompletion
			|| !NtOpenDirectoryObject || !NtOpenSymbolicLinkObject || !NtOpenEvent || !NtOpenMutant || !NtOpenSemaphore || !NtOpenSection || !NtOpenTimer || !NtOpenFile
			|| !NtOpenSession || !NtOpenCpuPartition || !NtOpenJobObject || !NtOpenIoCompletion || !NtOpenPartition) {
			HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
			if (!hModule) return FALSE;

			NtQueryDirectoryObject = (NtQueryDirectoryObject_t)GetProcAddress(hModule, "NtQueryDirectoryObject");
			NtQuerySymbolicLinkObject = (NtQuerySymbolicLinkObject_t)GetProcAddress(hModule, "NtQuerySymbolicLinkObject");
			NtQueryEvent = (NtQueryEvent_t)GetProcAddress(hModule, "NtQueryEvent");
			NtQueryMutant = (NtQueryMutant_t)GetProcAddress(hModule, "NtQueryMutant");
			NtQuerySemaphore = (NtQuerySemaphore_t)GetProcAddress(hModule, "NtQuerySemaphore");
			NtQuerySection = (NtQuerySection_t)GetProcAddress(hModule, "NtQuerySection");
			NtQueryTimer = (NtQueryTimer_t)GetProcAddress(hModule, "NtQueryTimer");
			NtQueryIoCompletion = (NtQueryIoCompletion_t)GetProcAddress(hModule, "NtQueryIoCompletion");
			NtOpenDirectoryObject = (NtOpenDirectoryObject_t)GetProcAddress(hModule, "NtOpenDirectoryObject");
			NtOpenSymbolicLinkObject = (NtOpenSymbolicLinkObject_t)GetProcAddress(hModule, "NtOpenSymbolicLinkObject");
			NtOpenEvent = (NtOpenEvent_t)GetProcAddress(hModule, "NtOpenEvent");
			NtOpenMutant = (NtOpenMutant_t)GetProcAddress(hModule, "NtOpenMutant");
			NtOpenSemaphore = (NtOpenSemaphore_t)GetProcAddress(hModule, "NtOpenSemaphore");
			NtOpenSection = (NtOpenSection_t)GetProcAddress(hModule, "NtOpenSection");
			NtOpenTimer = (NtOpenTimer_t)GetProcAddress(hModule, "NtOpenTimer");
			NtOpenFile = (NtOpenFile_t)GetProcAddress(hModule, "NtOpenFile");
			NtOpenSession = (NtOpenSession_t)GetProcAddress(hModule, "NtOpenSession");
			NtOpenCpuPartition = (NtOpenCpuPartition_t)GetProcAddress(hModule, "NtOpenCpuPartition");
			NtOpenJobObject = (NtOpenJobObject_t)GetProcAddress(hModule, "NtOpenJobObject");
			NtOpenIoCompletion = (NtOpenIoCompletion_t)GetProcAddress(hModule, "NtOpenIoCompletion");
			NtOpenPartition = (NtOpenPartition_t)GetProcAddress(hModule, "NtOpenPartition");

			if (!NtQueryDirectoryObject || !NtQuerySymbolicLinkObject || !NtQueryEvent || !NtQueryMutant || !NtQuerySemaphore || !NtQuerySection || !NtQueryTimer || !NtQueryIoCompletion
				|| !NtOpenDirectoryObject || !NtOpenSymbolicLinkObject || !NtOpenEvent || !NtOpenMutant || !NtOpenSemaphore || !NtOpenSection || !NtOpenTimer || !NtOpenFile
				|| !NtOpenSession || !NtOpenCpuPartition || !NtOpenJobObject || !NtOpenIoCompletion || !NtOpenPartition) return FALSE;
		}

		UNICODE_STRING objName;
		RtlInitUnicodeString(&objName, objectPath.c_str());

		OBJECT_ATTRIBUTES objAttr;
		InitializeObjectAttributes(&objAttr, &objName, OBJ_CASE_INSENSITIVE, NULL, NULL);

		HANDLE hDir = NULL;
		NTSTATUS status = NtOpenDirectoryObject(&hDir, 0x0001 /* DIRECTORY_QUERY */, &objAttr);

		if (!NT_SUCCESS(status) || !hDir) {
			return FALSE;
		}

		ULONG context = 0;
		ULONG returnLength = 0;
		std::vector<BYTE> buffer(4096);

		status = ERROR_SUCCESS;
		while (NT_SUCCESS(status)) {
			status = NtQueryDirectoryObject(hDir, buffer.data(), buffer.size(), FALSE, FALSE, &context, &returnLength);

			POBJECT_DIRECTORY_INFORMATION info =
				(POBJECT_DIRECTORY_INFORMATION)buffer.data();

			while (info->Name.Buffer) {
				winrt::StarlightGUI::ObjectEntry entry = winrt::make<winrt::StarlightGUI::implementation::ObjectEntry>();

				std::wstring name(info->Name.Buffer, info->Name.Length / sizeof(WCHAR));
				std::wstring type(info->TypeName.Buffer, info->TypeName.Length / sizeof(WCHAR));
				hstring path(objectPath + L"\\" + name);
				entry.Name(name);
				entry.Type(type);
				entry.Path(FixBackSplash(path));

				if (type == L"SymbolicLink") {
					KernelInstance::GetObjectDetails(name, type, entry);
				}

				objectList.push_back(entry);

				info++;
			}
		}

		CloseHandle(hDir);
		return TRUE;
	}

	BOOL KernelInstance::GetObjectDetails(std::wstring fullPath, std::wstring type, winrt::StarlightGUI::ObjectEntry& entry) noexcept {
		HANDLE hObject = NULL;
		NTSTATUS status = ERROR_SUCCESS;
		ULONG returnLength = 0;

		UNICODE_STRING objName;
		RtlInitUnicodeString(&objName, fullPath.c_str());

		OBJECT_ATTRIBUTES objAttr;
		InitializeObjectAttributes(&objAttr, &objName, OBJ_CASE_INSENSITIVE, NULL, NULL);

		// 根据类型尝试不同方式打开
		if (type == L"Directory") {
			status = NtOpenDirectoryObject(&hObject, 0x0001 /* DIRECTORY_QUERY */, &objAttr);
		}
		else if (type == L"SymbolicLink") {
			status = NtOpenSymbolicLinkObject(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"Event") {
			status = NtOpenEvent(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"Mutant") {
			status = NtOpenMutant(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"Semaphore") {
			status = NtOpenSemaphore(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"Section") {
			status = NtOpenSection(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"Timer") {
			status = NtOpenTimer(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"Device") {
			// Device 使用 NtOpenFile 打开
			IO_STATUS_BLOCK ioStatus = { 0 };
			status = NtOpenFile(&hObject, GENERIC_READ, &objAttr, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE);
		}
		else if (type == L"Session") {
			status = NtOpenSession(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"CpuPartition") {
			status = NtOpenCpuPartition(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"Job") {
			status = NtOpenJobObject(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"IoCompletion") {
			status = NtOpenIoCompletion(&hObject, GENERIC_READ, &objAttr);
		}
		else if (type == L"Partition") {
			status = NtOpenPartition(&hObject, GENERIC_READ, &objAttr);
		}
		else {
			// 不支持的类型
			return FALSE;
		}

		if (!NT_SUCCESS(status) || !hObject) return FALSE;

		// 获取基本信息
		OBJECT_BASIC_INFORMATION basicInfo{};
		status = NtQueryObject(hObject, ObjectBasicInformation, &basicInfo, sizeof(basicInfo), &returnLength);

		if (NT_SUCCESS(status)) {
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

				status = NtQuerySymbolicLinkObject(hObject, &target, &bufferLength);

				if (!NT_SUCCESS(status)) {
					target.Buffer = (PWSTR)HeapAlloc(GetProcessHeap(), 0, bufferLength);
					target.Length = 0;
					target.MaximumLength = (USHORT)bufferLength;

					status = NtQuerySymbolicLinkObject(hObject, &target, &bufferLength);
					if (NT_SUCCESS(status)) {
						entry.Link(std::wstring(target.Buffer, target.Length / sizeof(WCHAR)));
					}
					HeapFree(GetProcessHeap(), 0, target.Buffer);
				}
			}
			else if (type == L"Event") {
				EVENT_BASIC_INFORMATION eventInfo{};

				status = NtQueryEvent(hObject, EventBasicInformation, &eventInfo, sizeof(eventInfo), &bufferLength);
				if (NT_SUCCESS(status)) {
					entry.EventType(eventInfo.EventType == NotificationEvent ? L"Notification (Manual reset)" : L"Synchronization (Auto reset)");
					entry.EventSignaled(eventInfo.EventState == 0 ? FALSE : TRUE);
				}
			}
			else if (type == L"Mutant") {
				MUTANT_BASIC_INFORMATION mutantInfo{};

				status = NtQueryMutant(hObject, MutantBasicInformation, &mutantInfo, sizeof(mutantInfo), &bufferLength);
				if (NT_SUCCESS(status)) {
					entry.MutantHoldCount(mutantInfo.CurrentCount);
					entry.MutantAbandoned(mutantInfo.AbandonedState == 0 ? FALSE : TRUE);
				}
			}
			else if (type == L"Semaphore") {
				SEMAPHORE_BASIC_INFORMATION semaphoreInfo{};

				status = NtQuerySemaphore(hObject, SemaphoreBasicInformation, &semaphoreInfo, sizeof(semaphoreInfo), &bufferLength);
				if (NT_SUCCESS(status)) {
					entry.SemaphoreCount(semaphoreInfo.CurrentCount);
					entry.SemaphoreLimit(semaphoreInfo.MaximumCount);
				}
			}
			else if (type == L"Section") {
				SECTION_BASIC_INFORMATION sectionInfo{};

				status = NtQuerySection(hObject, SectionBasicInformation, &sectionInfo, sizeof(sectionInfo), NULL); // 这里传入长度会报错，可能是微软的问题
				if (NT_SUCCESS(status)) {
					entry.SectionBaseAddress((ULONG64)sectionInfo.BaseAddress);
					entry.SectionMaximumSize(sectionInfo.MaximumSize.QuadPart);
					entry.SectionAttributes(sectionInfo.AllocationAttributes);
				}
			}
			else if (type == L"Timer") {
				TIMER_BASIC_INFORMATION timerInfo{};
				status = NtQueryTimer(hObject, TimerBasicInformation, &timerInfo, sizeof(timerInfo), &bufferLength);
				if (NT_SUCCESS(status)) {
					entry.TimerRemainingTime(timerInfo.RemainingTime.QuadPart);
					entry.TimerState(timerInfo.TimerState);
				}
			}
			else if (type == L"IoCompletion") {
				IO_COMPLETION_BASIC_INFORMATION ioCompletionInfo{};

				status = NtQueryIoCompletion(hObject, IoCompletionBasicInformation, &ioCompletionInfo, sizeof(ioCompletionInfo), &bufferLength);
				if (NT_SUCCESS(status)) {
					entry.IoCompletionDepth(ioCompletionInfo.Depth);
				}
			}
		}

		CloseHandle(hObject);
		return NT_SUCCESS(status);
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
		lastErrorCode = SI_NOT_IMPLEMENTED;
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
		lastErrorCode = SI_NOT_AVAILABLE;
		lastErrorMessage = t(L"Common.PremiumOnly");
		return FALSE;
#endif
	}

	BOOL KernelInstance::ReadMemory(std::vector<BYTE>& data, PVOID address, ULONG size) noexcept {
		data.clear();
		if (!address || !size || size > (ULONG)-1 - FIELD_OFFSET(SI_MEMORY, Data)) {
			lastErrorCode = SI_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid memory read parameter.";
			return FALSE;
		}

		ULONG bufferSize = FIELD_OFFSET(SI_MEMORY, Data) + size;
		PSI_MEMORY input = (PSI_MEMORY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize);
		if (!input) {
			lastErrorCode = SI_ALLOCATION_FAILED;
			lastErrorMessage = L"Failed to allocate memory read buffer.";
			return FALSE;
		}

		input->Address = address;
		input->Size = size;

		BOOL result = SiQuerySystemInformation(SystemGetInformation::ReadMemory, input, 0);
		QueryError();
		if (result) {
			data.assign(input->Data, input->Data + size);
		}

		HeapFree(GetProcessHeap(), 0, input);
		return result;
	}

	BOOL KernelInstance::WriteMemory(PVOID address, PVOID data, ULONG size) noexcept {
		if (!address || !data || !size || size > (ULONG)-1 - FIELD_OFFSET(SI_MEMORY, Data)) {
			lastErrorCode = SI_INVALID_PARAMETER;
			lastErrorMessage = L"Invalid memory write parameter.";
			return FALSE;
		}

		ULONG bufferSize = FIELD_OFFSET(SI_MEMORY, Data) + size;
		PSI_MEMORY input = (PSI_MEMORY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize);
		if (!input) {
			lastErrorCode = SI_ALLOCATION_FAILED;
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

		if (device == INVALID_HANDLE_VALUE) return FALSE;

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

	std::string KernelInstance::GetMiniFilterMajorFunction(ULONG64 Index) noexcept
	{
		std::string funciton_name;
		switch (Index)
		{
		case 0:
			funciton_name = "IRP_MJ_CREATE";
			break;
		case 1:
			funciton_name = "IRP_MJ_CREATE_NAMED_PIPE";
			break;
		case 2:
			funciton_name = "IRP_MJ_CLOSE";
			break;
		case 3:
			funciton_name = "IRP_MJ_READ";
			break;
		case 4:
			funciton_name = "IRP_MJ_WRITE";
			break;
		case 5:
			funciton_name = "IRP_MJ_QUERY_INFORMATION";
			break;
		case 6:
			funciton_name = "IRP_MJ_SET_INFORMATION";
			break;
		case 7:
			funciton_name = "IRP_MJ_QUERY_EA";
			break;
		case 8:
			funciton_name = "IRP_MJ_SET_EA";
			break;
		case 9:
			funciton_name = "IRP_MJ_FLUSH_BUFFERS";
			break;
		case 10:
			funciton_name = "IRP_MJ_QUERY_VOLUME_INFORMATION";
			break;
		case 11:
			funciton_name = "IRP_MJ_SET_VOLUME_INFORMATION";
			break;
		case 12:
			funciton_name = "IRP_MJ_DIRECTORY_CONTROL";
			break;
		case 13:
			funciton_name = "IRP_MJ_FILE_SYSTEM_CONTROL";
			break;
		case 14:
			funciton_name = "IRP_MJ_DEVICE_CONTROL";
			break;
		case 15:
			funciton_name = "IRP_MJ_INTERNAL_DEVICE_CONTROL";
			break;
		case 16:
			funciton_name = "IRP_MJ_SHUTDOWN";
			break;
		case 17:
			funciton_name = "IRP_MJ_LOCK_CONTROL";
			break;
		case 18:
			funciton_name = "IRP_MJ_CLEANUP";
			break;
		case 19:
			funciton_name = "IRP_MJ_CREATE_MAILSLOT";
			break;
		case 20:
			funciton_name = "IRP_MJ_QUERY_SECURITY";
			break;
		case 21:
			funciton_name = "IRP_MJ_SET_SECURITY";
			break;
		case 22:
			funciton_name = "IRP_MJ_POWER";
			break;
		case 23:
			funciton_name = "IRP_MJ_SYSTEM_CONTROL";
			break;
		case 24:
			funciton_name = "IRP_MJ_DEVICE_CHANGE";
			break;
		case 25:
			funciton_name = "IRP_MJ_QUERY_QUOTA";
			break;
		case 26:
			funciton_name = "IRP_MJ_SET_QUOTA";
			break;
		case 27:
			funciton_name = "IRP_MJ_PNP";
			break;
		case 28:
			funciton_name = "IRP_MJ_PNP_POWER";
			break;
		case 255:
			funciton_name = "IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION";
			break;
		case 254:
			funciton_name = "IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION";
			break;
		case 253:
			funciton_name = "IRP_MJ_ACQUIRE_FOR_MOD_WRITE";
			break;
		case 252:
			funciton_name = "IRP_MJ_RELEASE_FOR_MOD_WRITE";
			break;
		case 251:
			funciton_name = "IRP_MJ_ACQUIRE_FOR_CC_FLUSH";
			break;
		case 250:
			funciton_name = "IRP_MJ_RELEASE_FOR_CC_FLUSH";
			break;
		case 249:
			funciton_name = "IRP_MJ_QUERY_OPEN";
			break;
		case 243:
			funciton_name = "IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE";
			break;
		case 242:
			funciton_name = "IRP_MJ_NETWORK_QUERY_OPEN";
			break;
		case 241:
			funciton_name = "IRP_MJ_MDL_READ";
			break;
		case 240:
			funciton_name = "IRP_MJ_MDL_READ_COMPLETE";
			break;
		case 239:
			funciton_name = "IRP_MJ_PREPARE_MDL_WRITE";
			break;
		case 238:
			funciton_name = "IRP_MJ_MDL_WRITE_COMPLETE";
			break;
		case 237:
			funciton_name = "IRP_MJ_VOLUME_MOUNT";
			break;
		case 236:
			funciton_name = "IRP_MJ_VOLUME_DISMOUN";
			break;
		case 128:
			funciton_name = "IRP_MJ_OPERATION_END";
			break;
		}
		if (funciton_name.size() > 0) {
			return funciton_name;
		}
		else {
			return "UNKNOWN(" + std::to_string(Index) + ")";
		}
	}
}
