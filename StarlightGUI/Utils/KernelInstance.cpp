#include "pch.h"
#include "KernelBase.h"
#include "CppUtils.h"

typedef struct _PROCESS_INPUT {
	ULONG PID;
} PROCESS_INPUT, * PPROCESS_INPUT;

typedef struct _DRIVER_INPUT {
	PVOID DriverObj;
} DRIVER_INPUT, * PDRIVER_INPUT;

namespace winrt::StarlightGUI::implementation {
	static HANDLE driverDevice = NULL;
	static HANDLE driverDevice2 = NULL;

	BOOL KernelInstance::_ZwTerminateProcess(ULONG pid) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { pid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_TERMINATE_PROCESS, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_TERMINATE_PROCESS, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::MurderProcess(ULONG pid) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { pid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_FORCE_TERMINATE_PROCESS, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_FORCE_TERMINATE_PROCESS, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::_SuspendProcess(ULONG pid) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { pid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_SUSPEND_PROCESS, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_SUSPEND_PROCESS, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::_ResumeProcess(ULONG pid) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { pid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_RESUME_PROCESS, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_RESUME_PROCESS, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::HideProcess(ULONG pid) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { pid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_HIDE_PROCESS, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_HIDE_PROCESS, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::SetPPL(ULONG pid, int level) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG PID;
			int level;
		};

		INPUT in = { pid, level };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_SET_PPL, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_SET_PPL, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::SetCriticalProcess(ULONG pid) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { pid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_SET_CRITICAL_PROCESS, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_SET_CRITICAL_PROCESS, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::InjectDLLToProcess(ULONG pid, PWCHAR dllPath) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG PID;
			UNICODE_STRING DllPath[MAX_PATH];
		};

		INPUT in = { 0 };
		in.PID = pid;
		RtlInitUnicodeString(in.DllPath, dllPath);

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_SHELLCODE_INJECT_DLL, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_SHELLCODE_INJECT_DLL, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::ModifyProcessToken(ULONG pid, ULONG type) noexcept {
		if (pid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG PID;
			ULONG Type;
		};

		INPUT in = { 0 };
		in.PID = pid;
		in.Type = type;

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_MODIFY_PROCESS_TOKEN, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_MODIFY_PROCESS_TOKEN, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::_ZwTerminateThread(ULONG tid) noexcept {
		if (tid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { tid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_TERMINATE_THREAD, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_TERMINATE_THREAD, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::MurderThread(ULONG tid) noexcept {
		if (tid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { tid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_FORCE_TERMINATE_THREAD, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_FORCE_TERMINATE_THREAD, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::_SuspendThread(ULONG tid) noexcept {
		if (tid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { tid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_SUSPEND_THREAD, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_SUSPEND_THREAD, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::_ResumeThread(ULONG tid) noexcept {
		if (tid == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		PROCESS_INPUT in = { tid };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_RESUME_THREAD, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_RESUME_THREAD, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::UnloadDriver(ULONG64 driverObj) noexcept {
		if (driverObj == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		DRIVER_INPUT in = { (PVOID)driverObj };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNLOAD_DRIVER, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_UNLOAD_DRIVER, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::HideDriver(ULONG64 driverObj) noexcept {
		if (driverObj == 0) return FALSE;
		if (!GetDriverDevice()) return FALSE;

		DRIVER_INPUT in = { (PVOID)driverObj };

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_HIDE_DRIVER, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_HIDE_DRIVER, &in, sizeof(in), 0, 0, 0, NULL);
	}

	BOOL KernelInstance::EnumProcesses(std::vector<winrt::StarlightGUI::ProcessInfo>& targetList) noexcept {
		if (!GetDriverDevice2()) return FALSE;

		BOOL bRet = FALSE;
		ENUM_PROCESS input = { 0 };

		PPROCESS_DATA pProcessInfo = NULL;

		input.BufferSize = sizeof(PROCESS_DATA) * 500;
		input.Buffer = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.BufferSize);
		input.ProcessCount = 0;

		BOOL status;

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_AX_ENUM_PROCESSES, __WFUNCTION__.c_str());
		status = DeviceIoControl(driverDevice2, IOCTL_AX_ENUM_PROCESSES, &input, sizeof(ENUM_PROCESS), &input, sizeof(ENUM_PROCESS), 0, NULL);

		if (status)
		{
			pProcessInfo = (PPROCESS_DATA)input.Buffer;
			for (ULONG i = 0; i < input.ProcessCount; i++)
			{
				PROCESS_DATA data = pProcessInfo[i];
				auto pi = winrt::make<winrt::StarlightGUI::implementation::ProcessInfo>();
				pi.Id(data.Pid);
				pi.Name(to_hstring(data.ImageName));
				pi.EProcess(ULongToHexString((ULONG64)data.Eprocess));
				pi.EProcessULong((ULONG64)data.Eprocess);
				pi.ExecutablePath(to_hstring(data.ImagePath));
				pi.MemoryUsageByte(data.WorkingSetPrivateSize);
				targetList.push_back(pi);
			}
		}
		bRet = HeapFree(GetProcessHeap(), 0, input.Buffer);
		return status && bRet;
	}

	BOOL KernelInstance::EnumProcesses2(std::vector<winrt::StarlightGUI::ProcessInfo>& targetList) noexcept {
		if (!GetDriverDevice()) return FALSE;
		struct INPUT
		{
			ULONG_PTR nSize;
			PVOID ProcessInfo;
		};

		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 500;
		input.ProcessInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		BOOL status;
		ULONG nRet = 0;

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_PROCESS, __WFUNCTION__.c_str());

		status = DeviceIoControl(driverDevice, IOCTL_ENUM_PROCESS, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);
		if (status && input.ProcessInfo)
		{
			pProcessInfo = (PDATA_INFO)input.ProcessInfo;
			for (ULONG i = 0; i < nRet; i++)
			{
				DATA_INFO data = pProcessInfo[i];
				auto pi = winrt::make<winrt::StarlightGUI::implementation::ProcessInfo>();
				pi.Id(data.ulongdata1);
				pi.Name(to_hstring(data.Module));
				pi.EProcess(ULongToHexString((ULONG64)data.pvoidaddressdata1));
				pi.EProcessULong((ULONG64)data.pvoidaddressdata1);
				pi.ExecutablePath(to_hstring(data.Module1));
				targetList.push_back(pi);
			}
		}
		status = HeapFree(GetProcessHeap(), 0, input.ProcessInfo);
		return status;
	}

	BOOL KernelInstance::EnumProcessThreads(ULONG64 eprocess, std::vector<winrt::StarlightGUI::ThreadInfo>& threads) noexcept
	{
		if (!GetDriverDevice()) return FALSE;
		BOOL status = FALSE;

		struct INPUT
		{
			ULONG nSize;
			ULONG64 pEprocess;
			PDATA_INFO pBuffer;
		};

		INPUT inputs = { 0 };

		ULONG nRet = 0;
		inputs.pEprocess = eprocess;
		inputs.nSize = sizeof(DATA_INFO) * 1000;
		inputs.pBuffer = (PDATA_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_PROCESS_THREAD_CIDTABLE, __WFUNCTION__.c_str());
		status = DeviceIoControl(driverDevice, IOCTL_ENUM_PROCESS_THREAD_CIDTABLE, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && inputs.pBuffer)
		{
			for (ULONG i = 0; i < nRet; i++)
			{
				DATA_INFO data = inputs.pBuffer[i];
				auto threadInfo = winrt::make<winrt::StarlightGUI::implementation::ThreadInfo>();
				threadInfo.Id(data.ulongdata3);
				threadInfo.EThread(ULongToHexString(data.ulong64data1));
				threadInfo.Address(ULongToHexString(data.ulong64data2));
				threadInfo.Priority(data.ulongdata2);
				threadInfo.ModuleInfo(winrt::to_hstring(data.Module));
				switch (data.ulongdata1)
				{
				case ThreadState_Initialized:
					threadInfo.Status(t(L"Msg.Thread.Initialized"));
					break;

				case ThreadState_Ready:
					threadInfo.Status(t(L"Msg.Thread.Ready"));
					break;

				case ThreadState_Running:
					threadInfo.Status(t(L"Msg.Thread.Running"));
					break;

				case ThreadState_Standby:
					threadInfo.Status(t(L"Msg.Thread.Standby"));
					break;

				case ThreadState_Terminated:
					threadInfo.Status(t(L"Msg.Thread.Terminated"));
					break;

				case ThreadState_Waiting:
					threadInfo.Status(t(L"Msg.Thread.Waiting"));
					break;

				case ThreadState_Transition:
					threadInfo.Status(L"Msg.Thread.Transition");
					break;

				case ThreadState_DeferredReady:
					threadInfo.Status(L"Msg.Thread.DeferredReady");
					break;

				case ThreadState_GateWait:
					threadInfo.Status(L"Msg.Thread.GateWait");
					break;

				default:
					threadInfo.Status(t(L"Msg.Thread.Unknown"));
					break;
				}
				threads.push_back(threadInfo);
			}
		}

		status = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		return status;
	}

	BOOL KernelInstance::EnumProcessHandles(ULONG pid, std::vector<winrt::StarlightGUI::HandleInfo>& handles) noexcept
	{
		if (!GetDriverDevice()) return FALSE;
		BOOL bRet = FALSE;
		ULONG nRet = 0;

		struct INPUT
		{
			ULONG nSize;
			ULONG PID;
			PDATA_INFO pBuffer;
		};
		PDATA_INFO pProcessInfo = NULL;
		INPUT inputs = { 0 };

		inputs.nSize = sizeof(DATA_INFO) * 1000;
		inputs.pBuffer = (PDATA_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);
		inputs.PID = pid;

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_PROCESS_EXIST_HANDLE, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_PROCESS_EXIST_HANDLE, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && inputs.pBuffer) {
			pProcessInfo = (PDATA_INFO)inputs.pBuffer;
			for (ULONG i = 0; i < nRet; i++)
			{
				DATA_INFO data = pProcessInfo[i];
				auto handleInfo = winrt::make<winrt::StarlightGUI::implementation::HandleInfo>();
				handleInfo.Type(to_hstring(data.Module));
				handleInfo.Object(ULongToHexString((ULONG64)data.pvoidaddressdata1));
				handleInfo.Handle(ULongToHexString(data.ulong64data3));
				handleInfo.Access(ULongToHexString(data.ulong64data1, 0, false, true));
				handleInfo.Attributes(ULongToHexString(data.ulong64data2, 0, false, true));
				handles.push_back(handleInfo);
			}
		}

		OutputDebugString(std::to_wstring(nRet).c_str());

		bRet = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		return bRet;
	}

	BOOL KernelInstance::EnumProcessModules(ULONG64 eprocess, std::vector<winrt::StarlightGUI::MokuaiInfo>& modules) noexcept
	{
		if (!GetDriverDevice()) return FALSE;
		BOOL bRet = FALSE;
		ULONG nRet = 0;

		struct INPUT
		{
			ULONG nSize;
			PVOID eproc;
			PDATA_INFO pBuffer;
		};
		PDATA_INFO pProcessInfo = NULL;
		INPUT inputs = { 0 };

		inputs.nSize = sizeof(DATA_INFO) * 1000;
		inputs.pBuffer = (PDATA_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);
		inputs.eproc = (PVOID)eprocess;

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_PROCESS_MODULE, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_PROCESS_MODULE, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && inputs.pBuffer) {
			pProcessInfo = (PDATA_INFO)inputs.pBuffer;
			for (ULONG i = 0; i < nRet; i++)
			{
				DATA_INFO data = pProcessInfo[i];
				auto moduleInfo = winrt::make<winrt::StarlightGUI::implementation::MokuaiInfo>();
				moduleInfo.Name(to_hstring(data.Module));
				moduleInfo.Address(ULongToHexString(data.ulong64data1));
				moduleInfo.Size(ULongToHexString(data.ulong64data2, 0, false, true));
				moduleInfo.Path(to_hstring(data.Module1));
				modules.push_back(moduleInfo);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		return bRet;
	}

	BOOL KernelInstance::EnumProcessKernelCallbackTable(ULONG64 eprocess, std::vector<winrt::StarlightGUI::KCTInfo>& modules) noexcept
	{
		if (!GetDriverDevice()) return FALSE;
		BOOL bRet = FALSE;
		ULONG nRet = 0;

		struct INPUT
		{
			ULONG nSize;
			PVOID eproc;
			PDATA_INFO pBuffer;
		};
		PDATA_INFO pProcessInfo = NULL;
		INPUT inputs = { 0 };

		inputs.nSize = sizeof(DATA_INFO) * 1000;
		inputs.pBuffer = (PDATA_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);
		inputs.eproc = (PVOID)eprocess;

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_KERNELCALLBACKTABLE, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_KERNELCALLBACKTABLE, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && inputs.pBuffer) {
			pProcessInfo = (PDATA_INFO)inputs.pBuffer;
			for (ULONG i = 0; i < nRet; i++)
			{
				DATA_INFO data = pProcessInfo[i];
				auto kctInfo = winrt::make<winrt::StarlightGUI::implementation::KCTInfo>();
				kctInfo.Name(to_hstring(data.Module));
				kctInfo.Address(ULongToHexString(data.ulong64data1));
				modules.push_back(kctInfo);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		return bRet;
	}

	BOOL KernelInstance::EnumDrivers(std::vector<winrt::StarlightGUI::KernelModuleInfo>& kernelModules) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG nSize;
			PALL_DRIVERS pBuffer;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };

		PPROCESS_DATA pProcessInfo = NULL;

		input.nSize = sizeof(ALL_DRIVERS) * 1000;
		input.pBuffer = (PALL_DRIVERS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_DRIVERS, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_DRIVERS, &input, sizeof(INPUT), 0, 0, 0, NULL);

		if (status && input.pBuffer && input.pBuffer->nCnt > 0)
		{
			for (ULONG i = 0; i < input.pBuffer->nCnt; i++)
			{
				DRIVER_INFO data = input.pBuffer->Drivers[i];
				auto di = winrt::make<winrt::StarlightGUI::implementation::KernelModuleInfo>();
				di.Name(to_hstring(data.szDriverName));
				di.Path(to_hstring(data.szDriverPath));
				di.ImageBase(ULongToHexString(data.nBase));
				di.ImageBaseULong(data.nBase);
				di.Size(ULongToHexString(data.nSize, 0, false, true));
				di.SizeULong(data.nSize);
				di.LoadOrder(ULongToHexString(data.nLoadOrder, 0, false, true));
				di.LoadOrderULong(data.nLoadOrder);
				di.DriverObject(ULongToHexString(data.nDriverObject));
				di.DriverObjectULong(data.nDriverObject);
				kernelModules.push_back(di);
			}
		}
		bRet = HeapFree(GetProcessHeap(), 0, input.pBuffer);
		return status && bRet;
	}

	BOOL KernelInstance::QueryFile(std::wstring path, std::vector<winrt::StarlightGUI::FileInfo>& files) noexcept
	{
		if (!GetDriverDevice()) return FALSE;
		BOOL bRet = FALSE;
		ULONG nRet = 0;

		struct INPUT
		{
			ULONG_PTR nSize;
			PDATA_INFO pBuffer;
			UNICODE_STRING path[MAX_PATH];
		};

		LOG_WARNING(L"KernelInstance", L"Enum file mode: %d", enum_file_mode);

		INPUT inputs = { 0 };

		if (enum_file_mode == 1)
		{
			RtlInitUnicodeString(inputs.path, path.c_str());
		}
		else
		{
			WCHAR targetPath[MAX_PATH];
			wcscpy_s(targetPath, L"\\??\\");
			wcscat_s(targetPath, path.c_str());
			RtlInitUnicodeString(inputs.path, targetPath);
		}

		inputs.nSize = sizeof(DATA_INFO) * 10000;
		inputs.pBuffer = (DATA_INFO*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);
		if (enum_file_mode == 2)
		{
            LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_NTFS_PARSER_ENUM_FILE2, __WFUNCTION__.c_str());
			bRet = DeviceIoControl(driverDevice, IOCTL_NTFS_PARSER_ENUM_FILE2, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, 0);
			if (bRet && nRet > 0 && nRet < 10000)
			{
				std::vector<winrt::StarlightGUI::FileInfo> result;
				for (ULONG i = 0; i < nRet; i++)
				{
					DATA_INFO data = inputs.pBuffer[i];
					auto fileInfo = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
					fileInfo.Name(data.wcstr);
					fileInfo.Path(path + L"\\" + data.wcstr);
					fileInfo.Flag(data.ulongdata1);
					fileInfo.Directory(data.ulongdata1 != MFT_RECORD_FLAG_FILE);
					fileInfo.Size(FormatMemorySize(data.ulong64data2));
					fileInfo.SizeULong(data.ulong64data2);
					fileInfo.MFTID(data.ulong64data1);
					result.push_back(fileInfo);
				}
				// Remove duplicated MFT indexes
				std::unordered_map<ULONG64, size_t> keep;
				for (size_t i = 0; i < result.size(); ++i) {
					ULONG64 mft = result[i].MFTID();
					auto it = keep.find(mft);

					if (it == keep.end()) keep[mft] = i;
					else {
						std::wstring_view curr = result[i].Name(), kept = result[it->second].Name();
						bool cHas = curr.find(L'~') != std::wstring_view::npos;
						bool kHas = kept.find(L'~') != std::wstring_view::npos;

						if ((!cHas && kHas) || (cHas == kHas && curr.length() < kept.length())) {
							it->second = i;
						}
					}
				}

				result.reserve(keep.size());
				for (auto& [mft, idx] : keep) files.push_back(std::move(result[idx]));
			}
		}
		else if (enum_file_mode == 0)
		{
            LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_QUERY_FILE2, __WFUNCTION__.c_str());
			bRet = DeviceIoControl(driverDevice, IOCTL_QUERY_FILE2, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, 0);
			if (bRet && nRet > 0 && nRet < 10000)
			{
				for (ULONG i = 0; i < nRet; i++)
				{
					DATA_INFO data = inputs.pBuffer[i];
					auto fileInfo = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
					fileInfo.Name(data.wcstr);
					fileInfo.Path(path + L"\\" + data.wcstr);
					if (data.ulongdata4 == FALSE)
					{
						fileInfo.Flag(MFT_RECORD_FLAG_FILE);
						fileInfo.Directory(false);
					}
					else
					{
						fileInfo.Flag(MFT_RECORD_FLAG_DIRECTORY);
						fileInfo.Directory(true);
					}
					files.push_back(fileInfo);
				}
			}
		}
		else if (enum_file_mode == 1)
		{
            LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_QUERY_FILE_IRP, __WFUNCTION__.c_str());
			bRet = DeviceIoControl(driverDevice, IOCTL_QUERY_FILE_IRP, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, 0);
			if (bRet && nRet > 0 && nRet < 10000)
			{
				for (ULONG i = 0; i < nRet; i++)
				{
					DATA_INFO data = inputs.pBuffer[i];
					auto fileInfo = winrt::make<winrt::StarlightGUI::implementation::FileInfo>();
					fileInfo.Name(data.wcstr);
					fileInfo.Path(path + L"\\" + data.wcstr);
					if (data.ulongdata4 == FALSE)
					{
						fileInfo.Flag(MFT_RECORD_FLAG_FILE);
						fileInfo.Directory(false);
					}
					else
					{
						fileInfo.Flag(MFT_RECORD_FLAG_DIRECTORY);
						fileInfo.Directory(true);
					}
					files.push_back(fileInfo);
				}
			}
		}
		bRet = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		return bRet;
	}

	BOOL KernelInstance::_DeleteFile(std::wstring path) noexcept {
		if (!GetDriverDevice()) return FALSE;

		WCHAR targetPath[MAX_PATH];
		wcscpy_s(targetPath, L"\\??\\");
		wcscat_s(targetPath, path.c_str());

		UNICODE_STRING filePath[MAX_PATH];
		RtlInitUnicodeString(filePath, targetPath);

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_DELETE_FILE_UNICODE, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_DELETE_FILE_UNICODE, filePath, sizeof(filePath), NULL, 0, 0, NULL);
	}

	BOOL KernelInstance::MurderFile(std::wstring path) noexcept {
		if (!GetDriverDevice()) return FALSE;

		WCHAR targetPath[MAX_PATH];
		wcscpy_s(targetPath, L"\\??\\");
		wcscat_s(targetPath, path.c_str());

		UNICODE_STRING filePath[MAX_PATH];
		RtlInitUnicodeString(filePath, targetPath);

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_FORCE_DELETE_UNICODE, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_FORCE_DELETE_UNICODE, filePath, sizeof(filePath), NULL, 0, 0, NULL);

		if (status) {
			status = DeleteFileW(path.c_str());
			LOG_INFO(L"KernelInstance", L"Post-deleted file.");
		}

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

	BOOL KernelInstance::_DeleteFileAuto(std::wstring path) noexcept {
		if (!fs::exists(path)) {
			return FALSE;
		}

		if (!fs::is_directory(path)) {
			return _DeleteFile(path);
		}
		else {
			for (const auto& entry : fs::directory_iterator(path)) {
				if (fs::is_directory(entry)) {
					_DeleteFileAuto(entry.path().wstring());
				}
				if (fs::is_regular_file(entry)) {
					_DeleteFile(entry.path().wstring());
				}
			}
			LOG_INFO(L"KernelInstance", L"Post-deleted directory.");
			return RemoveDirectoryW(path.c_str());
		}
	}

	BOOL KernelInstance::MurderFileAuto(std::wstring path) noexcept {
		if (!fs::exists(path)) {
			return FALSE;
		}

		if (!fs::is_directory(path)) {
			return MurderFile(path);
		}
		else {
			for (const auto& entry : fs::directory_iterator(path)) {
				if (fs::is_directory(entry)) {
					MurderFileAuto(entry.path().wstring());
				}
				if (fs::is_regular_file(entry)) {
					MurderFile(entry.path().wstring());
				}
			}
			LOG_INFO(L"KernelInstance", L"Post-deleted directory.");
			return RemoveDirectoryW(path.c_str());
		}
	}

	BOOL KernelInstance::LockFile(std::wstring path) noexcept {
		if (!GetDriverDevice()) return FALSE;

		WCHAR targetPath[MAX_PATH];
		wcscpy_s(targetPath, L"\\??\\");
		wcscat_s(targetPath, path.c_str());

		UNICODE_STRING filePath[MAX_PATH];
		RtlInitUnicodeString(filePath, targetPath);

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\", parameters: [path=%s]", IOCTL_LOCK_FILE_UNICODE, __WFUNCTION__.c_str(), path.c_str());
		return DeviceIoControl(driverDevice, IOCTL_LOCK_FILE_UNICODE, filePath, sizeof(filePath), NULL, 0, 0, NULL);
	}

	BOOL KernelInstance::_CopyFile(std::wstring from, std::wstring to, std::wstring name) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT
		{
			UNICODE_STRING FilePath[MAX_PATH];
			UNICODE_STRING FileName[MAX_PATH];
			UNICODE_STRING TargetFilePath[MAX_PATH];
		};

		WCHAR fromPath[MAX_PATH];
		wcscpy_s(fromPath, from.c_str());
		WCHAR toPath[MAX_PATH];
		wcscpy_s(toPath, to.c_str());

		INPUT input = { 0 };
		RtlInitUnicodeString(input.FilePath, fromPath);
		RtlInitUnicodeString(input.FileName, name.c_str());
		RtlInitUnicodeString(input.TargetFilePath, toPath);

        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_NTFS_COPY_FILE, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_NTFS_COPY_FILE, &input, sizeof(INPUT), NULL, 0, NULL, 0);
	}

	BOOL KernelInstance::_RenameFile(std::wstring from, std::wstring to) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT
		{
			UNICODE_STRING FilePath[MAX_PATH];
			UNICODE_STRING TargetName[MAX_PATH];
		};

		INPUT input = { 0 };
		RtlInitUnicodeString(input.FilePath, from.c_str());
		RtlInitUnicodeString(input.TargetName, to.c_str());

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_IRP_RENAME_FILE, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_IRP_RENAME_FILE, &input, sizeof(INPUT), NULL, 0, NULL, 0);
	}

	BOOL KernelInstance::EnableHVM() noexcept {
		if (!GetDriverDevice()) return FALSE;

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_CHECK_HVM, __WFUNCTION__.c_str());
		BOOL result = DeviceIoControl(driverDevice, IOCTL_CHECK_HVM, NULL, 0, NULL, 0, NULL, NULL);
		
		if (DeviceIoControl(driverDevice, IOCTL_CHECK_HVM, NULL, 0, NULL, 0, NULL, NULL)) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_INIT_HVM, __WFUNCTION__.c_str());
			result = DeviceIoControl(driverDevice, IOCTL_INIT_HVM, NULL, 0, NULL, 0, NULL, NULL);
		}

		return result;
	}

	BOOL KernelInstance::EnableCreateProcess() noexcept {
		if (!GetDriverDevice()) return FALSE;
        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNPROHIBIT_CREATEPROCESS, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_UNPROHIBIT_CREATEPROCESS, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisableCreateProcess() noexcept {
		if (!GetDriverDevice()) return FALSE;
        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_PROHIBIT_CREATEPROCESS, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_PROHIBIT_CREATEPROCESS, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::EnableCreateFile() noexcept {
		if (!GetDriverDevice()) return FALSE;
        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNPROHIBIT_CREATEFILE, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_UNPROHIBIT_CREATEFILE, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisableCreateFile() noexcept {
		if (!GetDriverDevice()) return FALSE;
        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_PROHIBIT_CREATEFILE, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_PROHIBIT_CREATEFILE, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::EnableLoadDriver() noexcept {
		if (!GetDriverDevice()) return FALSE;
        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNPROHIBIT_LOADDRIVER, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_UNPROHIBIT_LOADDRIVER, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisableLoadDriver() noexcept {
		if (!GetDriverDevice()) return FALSE;
        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_PROHIBIT_LOADDRIVER, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_PROHIBIT_LOADDRIVER, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::EnableUnloadDriver() noexcept {
		if (!GetDriverDevice()) return FALSE;
        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNPROHIBIT_UNLOADDRIVER, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_UNPROHIBIT_UNLOADDRIVER, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisableUnloadDriver() noexcept {
		if (!GetDriverDevice()) return FALSE;
        LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_PROHIBIT_UNLOADDRIVER, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_PROHIBIT_UNLOADDRIVER, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::EnableModifyRegistry() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNPROHIBIT_MODIFY_REGISTRY, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_UNPROHIBIT_MODIFY_REGISTRY, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisableModifyRegistry() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_PROHIBIT_MODIFY_REGISTRY, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_PROHIBIT_MODIFY_REGISTRY, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::ProtectDisk() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_PROTECT_DISK, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_PROTECT_DISK, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::UnprotectDisk() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNPROTECT_DISK, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_UNPROTECT_DISK, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::EnableObCallback() noexcept {
		if (!GetDriverDevice()) return FALSE;

		if (hypervisor_mode) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENABLE_OBCALLBACK_HVM, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_ENABLE_OBCALLBACK_HVM, NULL, 0, NULL, 0, NULL, NULL);
		}
		else {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENABLE_OBCALLBACK, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_ENABLE_OBCALLBACK, NULL, 0, NULL, 0, NULL, NULL);
		}
	}

	BOOL KernelInstance::DisableObCallback() noexcept {
		if (!GetDriverDevice()) return FALSE;

		if (hypervisor_mode) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_DISABLE_OBCALLBACK_HVM, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_DISABLE_OBCALLBACK_HVM, NULL, 0, NULL, 0, NULL, NULL);
		}
		else {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_DISABLE_OBCALLBACK, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_DISABLE_OBCALLBACK, NULL, 0, NULL, 0, NULL, NULL);
		}
	}

	BOOL KernelInstance::EnableDSE() noexcept {
		if (!GetDriverDevice()) return FALSE;

		if (hypervisor_mode) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENABLE_DSE_HVM, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_ENABLE_DSE, NULL, 0, NULL, 0, NULL, NULL);
		}
		else {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENABLE_DSE, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_ENABLE_DSE_HVM, NULL, 0, NULL, 0, NULL, NULL);
		}
	}

	BOOL KernelInstance::DisableDSE() noexcept {
		if (!GetDriverDevice()) return FALSE;

		if (hypervisor_mode) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_DISABLE_DSE_HVM, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_DISABLE_DSE_HVM, NULL, 0, NULL, 0, NULL, NULL);
		}
		else {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_DISABLE_DSE, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_DISABLE_DSE, NULL, 0, NULL, 0, NULL, NULL);
		}
	}

	BOOL KernelInstance::EnableCmpCallback() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENABLE_CMPCALLBACK, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_ENABLE_CMPCALLBACK, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisableCmpCallback() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_DISABLE_CMPCALLBACK, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_DISABLE_CMPCALLBACK, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::EnableLKD() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENABLE_LKD, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_ENABLE_LKD, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisableLKD() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_DISABLE_LKD, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_DISABLE_LKD, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::EnableEPTScan() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENABLE_SCAN_EPT_HOOK, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_ENABLE_SCAN_EPT_HOOK, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisableEPTScan() noexcept {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_DISABLE_SCAN_EPT_HOOK, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_DISABLE_SCAN_EPT_HOOK, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::DisablePatchGuard(int type) noexcept {
		if (!GetDriverDevice()) return FALSE;

		if (type == 0) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\", parameters: [type=%d]", IOCTL_DISABLE_PATCHGUARD_EFI, __WFUNCTION__, type);
			return DeviceIoControl(driverDevice, IOCTL_DISABLE_PATCHGUARD_EFI, NULL, 0, NULL, 0, NULL, NULL);
		}
		else if (type == 1) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\", parameters: [type=%d]", IOCTL_DISABLE_PATCHGUARD_BIOS, __WFUNCTION__, type);
			return DeviceIoControl(driverDevice, IOCTL_DISABLE_PATCHGUARD_BIOS, NULL, 0, NULL, 0, NULL, NULL);
		}
		else if (type == 2) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\", parameters: [type=%d]", IOCTL_DISABLE_PATCHGUARD_DYNAMIC, __WFUNCTION__, type);
			return DeviceIoControl(driverDevice, IOCTL_DISABLE_PATCHGUARD_DYNAMIC, NULL, 0, NULL, 0, NULL, NULL);
		}
		return FALSE;
	}

	BOOL KernelInstance::Shutdown() {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_SHUTDOWN, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_SHUTDOWN, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::Reboot() {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_REBOOT, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_REBOOT, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::RebootForce() {
		if (!GetDriverDevice()) return FALSE;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_FORCE_REBOOT, __WFUNCTION__.c_str());
		return DeviceIoControl(driverDevice, IOCTL_FORCE_REBOOT, NULL, 0, NULL, 0, NULL, NULL);
	}

	BOOL KernelInstance::BlueScreen(int color) {
		if (!GetDriverDevice()) return FALSE;

		if (color == -1) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\", parameters: [color=-1]", IOCTL_BLUESCREEN, __WFUNCTION__.c_str());
			return DeviceIoControl(driverDevice, IOCTL_BLUESCREEN, NULL, 0, NULL, 0, NULL, NULL);
		}
		else {
			struct INPUT {
				ULONG color;
			};
			INPUT input = { color };

			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\", parameters: [color=%d]", IOCTL_CRASH_SYSTEM_SET_COLOR, __WFUNCTION__, color);
			return DeviceIoControl(driverDevice, IOCTL_CRASH_SYSTEM_SET_COLOR, &input, sizeof(input), NULL, 0, NULL, NULL);
		}
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

	BOOL KernelInstance::EnumObjectsByDirectory(std::wstring objectPath, std::vector<winrt::StarlightGUI::ObjectEntry>& objectList) noexcept {
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

		// 枚举对象
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

				// 只获取符号链接的详细信息，其他类型获取详细信息会很慢
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

	BOOL KernelInstance::RemoveNotify(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		if (!GetDriverDevice()) return FALSE;
		struct INPUT {
			PVOID Address;
			PVOID Handle;
			ULONG Type;
		};

		INPUT input = { 0 };
		input.Address = (PVOID)entry.ULongLong1();
		input.Handle = (PVOID)entry.ULongLong2();
		input.Type = entry.ULong1();

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_REMOVE_NOTIFY, __WFUNCTION__.c_str());

		BOOL status = DeviceIoControl(driverDevice, IOCTL_REMOVE_NOTIFY, &input, sizeof(INPUT), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::RemoveMiniFilter(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		if (!GetDriverDevice()) return FALSE;
		struct INPUT {
			PVOID Address;
		};

		INPUT input = { 0 };
		input.Address = (PVOID)entry.ULongLong1();

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_REMOVE_MINIFILTER2, __WFUNCTION__.c_str());

		BOOL status = DeviceIoControl(driverDevice, IOCTL_REMOVE_MINIFILTER2, &input, sizeof(INPUT), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::RemoveStandardFilter(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		if (!GetDriverDevice()) return FALSE;
		struct INPUT {
			ULONG64 TargetDriverObject;
			ULONG64 DeviceObject;
		};

		INPUT input = { 0 };
		input.DeviceObject = entry.ULongLong1();
		input.TargetDriverObject = entry.ULongLong2();

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_REMOVE_MINIFILTER2, __WFUNCTION__.c_str());

		BOOL status = DeviceIoControl(driverDevice, IOCTL_REMOVE_MINIFILTER2, &input, sizeof(INPUT), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::UnhookSSDT(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		if (!GetDriverDevice()) return FALSE;

		BOOL status = FALSE;

		if (entry.Bool1()) {
			struct INPUT {
				ULONG Index;
			};
			INPUT input = { entry.ULong1() };

			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNHOOK_SSDT_INLINEHOOK, __WFUNCTION__.c_str());

			status = DeviceIoControl(driverDevice, IOCTL_UNHOOK_SSDT_INLINEHOOK, &input, sizeof(INPUT), 0, 0, 0, NULL);
		}
		else {
			struct INPUT {
				PVOID OriginalAddress;
				ULONG Index;
			};

			INPUT input = { (PVOID)entry.ULongLong2(), entry.ULong1() };

			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_REMOVE_SSDTHOOK, __WFUNCTION__.c_str());

			status = DeviceIoControl(driverDevice, IOCTL_REMOVE_SSDTHOOK, &input, sizeof(INPUT), 0, 0, 0, NULL);
		}

		return status;
	}

	BOOL KernelInstance::UnhookSSSDT(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG Index;
			ULONG64 OriginalAddress;
			ULONG IsInlineHook;
		};
		INPUT input = { entry.ULong1(), entry.ULongLong2(), entry.Bool1() ? 1 : 0 };

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_UNHOOK_SSSDT, __WFUNCTION__.c_str());

		BOOL status = DeviceIoControl(driverDevice, IOCTL_UNHOOK_SSSDT, &input, sizeof(INPUT), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::RemoveExCallback(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			PVOID Address;
		};
		INPUT input = { (PVOID)entry.ULongLong3() };

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_REMOVE_EXCALLBACK, __WFUNCTION__.c_str());

		BOOL status = DeviceIoControl(driverDevice, IOCTL_REMOVE_EXCALLBACK, &input, sizeof(INPUT), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::RemovePiDDBCache(winrt::StarlightGUI::GeneralEntry& entry) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG Time;
		};
		INPUT input = { entry.ULong2() };

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_REMOVE_PIDDBCACHE, __WFUNCTION__.c_str());

		BOOL status = DeviceIoControl(driverDevice, IOCTL_REMOVE_PIDDBCACHE, &input, sizeof(INPUT), 0, 0, 0, NULL);

		return status;
	}

	BOOL KernelInstance::EnumNotifies(std::vector<winrt::StarlightGUI::GeneralEntry>& callbackList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID pBuffer;
		};

		BOOL bRet = FALSE;
		ULONG nRet = 0;
		PDATA_INFO pProcessInfo = NULL;

		// 定义所有回调类型
		const struct {
			DWORD id;
			DWORD ioctl;
			const wchar_t* type;
		} callbackTypes[] = {
			{ 0, IOCTL_ENUM_CREATE_PROCESS_NOTIFY, L"CreateProcess" },
			{ 1, IOCTL_ENUM_CREATE_THREAD_NOTIFY, L"CreateThread" },
			{ 3, IOCTL_ENUM_LOADIMAGE_NOTIFY, L"LoadImage" },
			{ 2, IOCTL_ENUM_REGISTRY_CALLBACK, L"Registry" },
			{ 4, IOCTL_ENUM_BUGCHECK_CALLBACK, L"BugCheck" },
			{ 5, IOCTL_ENUM_BUGCHECKREASON_CALLBACK, L"BugCheckReason" },
			{ 6, IOCTL_ENUM_SHUTDOWN_NOTIFY, L"Shutdown" },
			{ 7, IOCTL_ENUM_LASTSHUTDOWN_NOTIFY, L"LastChanceShutdown" },
			{ 8, IOCTL_ENUM_FS_NOTIFY, L"FileSystemNotify" },
			{ 11, IOCTL_ENUM_PRIORIRY_NOTIFY, L"PriorityCallback" },
			{ 15, IOCTL_ENUM_PLUGPLAY_NOTIFY, L"PlugPlay" },
			{ 10, IOCTL_ENUM_COALESCING_NOTIFY, L"CoalescingCallback" },
			{ 12, IOCTL_ENUM_DBGPRINT_CALLBACK, L"DbgPrint" },
			{ 16, IOCTL_ENUM_EMP_CALLBACK, L"EmpCallback" },
			{ 14, IOCTL_ENUM_NMI_CALLBACK, L"NmiCallback" }
		};

		// 处理常规回调
		for (const auto& cbType : callbackTypes) {
			INPUT inputs = { 0 };
			inputs.nSize = sizeof(DATA_INFO) * 1000;
			inputs.pBuffer = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);

			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", cbType.ioctl, __WFUNCTION__.c_str());
			BOOL status = DeviceIoControl(driverDevice, cbType.ioctl, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

			if (nRet > 1000) nRet = 1000;

			if (status && nRet > 0 && inputs.pBuffer) {
				pProcessInfo = (PDATA_INFO)inputs.pBuffer;
				for (ULONG i = 0; i < nRet; i++) {
					DATA_INFO data = pProcessInfo[i];
					auto callback = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
					callback.String1(to_hstring(data.Module));
					callback.String2(cbType.type);
					callback.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
					callback.ULongLong1((ULONG64)data.pvoidaddressdata1);
					callback.String4(ULongToHexString((ULONG64)data.pvoidaddressdata2));
					callback.ULongLong2((ULONG64)data.pvoidaddressdata2);
					callback.ULong1(cbType.id);
					callbackList.push_back(callback);
				}
			}

			bRet = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		}

		// 处理 ObCallback - PsProcessType
		{
			INPUT inputs = { 0 };
			inputs.nSize = sizeof(DATA_INFO) * 1000;
			inputs.pBuffer = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);

			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_OB_PROCESS_CALLBACK, __WFUNCTION__.c_str());
			BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_OB_PROCESS_CALLBACK, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

			if (nRet > 1000) nRet = 1000;

			if (status && nRet > 0 && inputs.pBuffer) {
				pProcessInfo = (PDATA_INFO)inputs.pBuffer;
				for (ULONG i = 0; i < nRet; i++) {
					DATA_INFO data = pProcessInfo[i];
					if (data.pvoidaddressdata1 != NULL) {
						auto callback = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
						callback.String1(to_hstring(data.Module));
						callback.String2(L"ObCallback-PsProcessType");
						callback.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
						callback.ULongLong1((ULONG64)data.pvoidaddressdata1);
						callback.String4(ULongToHexString((ULONG64)data.pvoidaddressdata2));
						callback.ULongLong2((ULONG64)data.pvoidaddressdata2);
						callback.ULong1(13);
						callbackList.push_back(callback);
					}
				}
			}

			bRet = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		}

		// 处理 ObCallback - PsThreadType
		{
			INPUT inputs = { 0 };
			inputs.nSize = sizeof(DATA_INFO) * 1000;
			inputs.pBuffer = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);

			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_OB_THREAD_CALLBACK, __WFUNCTION__.c_str());
			BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_OB_THREAD_CALLBACK, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

			if (nRet > 1000) nRet = 1000;

			if (status && nRet > 0 && inputs.pBuffer) {
				pProcessInfo = (PDATA_INFO)inputs.pBuffer;
				for (ULONG i = 0; i < nRet; i++) {
					DATA_INFO data = pProcessInfo[i];
					if (data.pvoidaddressdata1 != NULL) {
						auto callback = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
						callback.String1(to_hstring(data.Module));
						callback.String2(L"ObCallback-PsThreadType");
						callback.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
						callback.ULongLong1((ULONG64)data.pvoidaddressdata1);
						callback.String4(ULongToHexString((ULONG64)data.pvoidaddressdata2));
						callback.ULongLong2((ULONG64)data.pvoidaddressdata2);
						callback.ULong1(13);
						callbackList.push_back(callback);
					}
				}
			}

			bRet = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		}

		// 处理 ObCallback - Desktop
		{
			INPUT inputs = { 0 };
			inputs.nSize = sizeof(DATA_INFO) * 1000;
			inputs.pBuffer = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputs.nSize);

			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_OB_THREAD_CALLBACK, __WFUNCTION__.c_str());
			BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_OB_THREAD_CALLBACK, &inputs, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

			if (nRet > 1000) nRet = 1000;

			if (status && nRet > 0 && inputs.pBuffer) {
				pProcessInfo = (PDATA_INFO)inputs.pBuffer;
				for (ULONG i = 0; i < nRet; i++) {
					DATA_INFO data = pProcessInfo[i];
					if (data.pvoidaddressdata1 != NULL) {
						auto callback = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
						callback.String1(to_hstring(data.Module));
						callback.String2(L"ObCallback-Desktop");
						callback.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
						callback.ULongLong1((ULONG64)data.pvoidaddressdata1);
						callback.String4(ULongToHexString((ULONG64)data.pvoidaddressdata2));
						callback.ULongLong2((ULONG64)data.pvoidaddressdata2);
						callback.ULong1(13);
						callbackList.push_back(callback);
					}
				}
			}

			bRet = HeapFree(GetProcessHeap(), 0, inputs.pBuffer);
		}

		return TRUE;
	}

	BOOL KernelInstance::EnumMiniFilter(std::vector<winrt::StarlightGUI::GeneralEntry>& filterList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_MINIFILTER, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_MINIFILTER, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(data.Module));
				entry.String2(to_hstring(GetMiniFilterMajorFunction(data.ulongdata1)));
				entry.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
				entry.String4(ULongToHexString((ULONG64)data.pvoidaddressdata2));
				entry.String5(ULongToHexString((ULONG64)data.pvoidaddressdata3));
				entry.ULongLong1((ULONG64)data.pvoidaddressdata1);
				entry.ULongLong2((ULONG64)data.pvoidaddressdata2);
				entry.ULongLong3((ULONG64)data.pvoidaddressdata3);
				filterList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumStandardFilter(std::vector<winrt::StarlightGUI::GeneralEntry>& filterList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_STANDARD_FILTER, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_STANDARD_FILTER, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				if (pProcessInfo[i].ulongdata1 == ft_Unknown)
				{
					entry.String1(L"Unknown");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_File)
				{
					entry.String1(L"File");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_Disk)
				{
					entry.String1(L"Disk");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_Volume)
				{
					entry.String1(L"Volume");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_Keyboard)
				{
					entry.String1(L"Keyboard");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_Mouse)
				{
					entry.String1(L"Mouse");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_I8042prt)
				{
					entry.String1(L"I8042prt");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_Tcpip)
				{
					entry.String1(L"Tcpip");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_Ndis)
				{
					entry.String1(L"Ndis");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_PnpManager)
				{
					entry.String1(L"PnpManager");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_Tdx)
				{
					entry.String1(L"Tdx");
				}
				else if (pProcessInfo[i].ulongdata1 == ft_Raw)
				{
					entry.String1(L"Raw (ntoskrnl)");
				}
				entry.String2(to_hstring(data.wcstr) + L" -> " + to_hstring(data.wcstr1));
				entry.String3(data.wcstr2);
				entry.String4(ULongToHexString(data.ulong64data1));
				entry.String5(ULongToHexString(data.ulong64data2));
				entry.ULongLong1(data.ulong64data1);
				entry.ULongLong2(data.ulong64data2);
				entry.ULong1(data.ulongdata1);

				filterList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumSSDT(std::vector<winrt::StarlightGUI::GeneralEntry>& ssdtList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_SSDT, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_SSDT, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				if (data.pvoidaddressdata1 != NULL) {
					auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
					entry.String1(to_hstring(data.Module));
					entry.String2(to_hstring(data.Module1));
					entry.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
					entry.String4(ULongToHexString((ULONG64)data.pvoidaddressdata2));

					std::wstring hookType = L"-";
					if (data.pvoidaddressdata2 != NULL) {
						if (data.pvoidaddressdata1 != data.pvoidaddressdata2) {
							hookType = L"SSDT Hook";
						}
						else if (data.ulongdata2 == TRUE) {
							hookType = L"Inline Hook";
						}
						else if (data.ulongdata4 == TRUE) {
							hookType = L"EPT/NPT Hook";
						}
					}
					else {
						hookType = L"Unknown";
					}

					entry.String5(hookType);
					entry.ULongLong1((ULONG64)data.pvoidaddressdata1);
					entry.ULongLong2((ULONG64)data.pvoidaddressdata2);
					entry.ULong1(data.ulongdata1);
					entry.Bool1(data.ulongdata2 == TRUE);
					ssdtList.push_back(entry);
				}
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumSSSDT(std::vector<winrt::StarlightGUI::GeneralEntry>& sssdtList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 3000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_SSSDT, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_SSSDT, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 3000) nRet = 3000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				if (data.pvoidaddressdata1 != NULL) {
					auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
					entry.String1(to_hstring(data.Module1));
					entry.String2(to_hstring(data.Module));
					entry.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
					entry.String4(ULongToHexString((ULONG64)data.pvoidaddressdata2));

					std::wstring hookType = L"-";
					if (data.pvoidaddressdata2 == NULL) {
						hookType = L"Unknown";
					}
					else if (data.ulongdata2 == TRUE) {
						hookType = L"Inline Hook";
					}
					else if (data.pvoidaddressdata1 != data.pvoidaddressdata2) {
						hookType = L"SSSDT Hook";
					}

					entry.String5(hookType);
					entry.ULongLong1((ULONG64)data.pvoidaddressdata1);
					entry.ULongLong2((ULONG64)data.pvoidaddressdata2);
					entry.ULong1(data.ulongdata1);
					entry.Bool1(data.ulongdata2 == TRUE);
					sssdtList.push_back(entry);
				}
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumIoTimer(std::vector<winrt::StarlightGUI::GeneralEntry>& timerList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_IO_TIMER, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_IO_TIMER, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(data.Module));
				entry.String2(ULongToHexString((ULONG64)data.pvoidaddressdata1));
				entry.String3(ULongToHexString((ULONG64)data.pvoidaddressdata2));
				entry.ULongLong1((ULONG64)data.pvoidaddressdata1);
				entry.ULongLong2((ULONG64)data.pvoidaddressdata2);
				timerList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumExCallback(std::vector<winrt::StarlightGUI::GeneralEntry>& callbackList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_EXCALLBACK, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_EXCALLBACK, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(data.Module));
				entry.String2(to_hstring(data.Module1));
				entry.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
				entry.String4(ULongToHexString((ULONG64)data.pvoidaddressdata2));
				entry.String5(ULongToHexString((ULONG64)data.pvoidaddressdata3));
				entry.ULongLong1((ULONG64)data.pvoidaddressdata1);
				entry.ULongLong2((ULONG64)data.pvoidaddressdata2);
				entry.ULongLong3((ULONG64)data.pvoidaddressdata3);
				callbackList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumIDT(std::vector<winrt::StarlightGUI::GeneralEntry>& idtList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_IDT, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_IDT, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(data.Module));
				entry.String2(ULongToHexString((ULONG64)data.pvoidaddressdata1));
				entry.ULongLong1((ULONG64)data.pvoidaddressdata1);
				entry.ULong1(data.ulongdata1);
				entry.ULong2(data.ulongdata2);
				idtList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumGDT(std::vector<winrt::StarlightGUI::GeneralEntry>& gdtList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_GDT, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_GDT, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(data.Module));
				entry.String2(ULongToHexString((ULONG64)data.pvoidaddressdata1));
				entry.String3(ULongToHexString((ULONG64)data.pvoidaddressdata2));
				entry.ULongLong1((ULONG64)data.pvoidaddressdata1);
				entry.ULongLong2((ULONG64)data.pvoidaddressdata2);
				entry.ULong1(data.ulongdata1);
				entry.ULong2(data.ulongdata2);
				entry.ULong3(data.ulongdata3);
				gdtList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumPiDDBCacheTable(std::vector<winrt::StarlightGUI::GeneralEntry>& piddbList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_PIDDBCACHE_TABLE, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_PIDDBCACHE_TABLE, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(data.Module));
				entry.ULong1(data.ulongdata1);
				entry.ULong2(data.ulongdata2);
				piddbList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumHalDispatchTable(std::vector<winrt::StarlightGUI::GeneralEntry>& halList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_HALDISPATCHTABLE, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_HALDISPATCHTABLE, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(data.Module));
				entry.String2(to_hstring(data.Module1));
				entry.String3(ULongToHexString((ULONG64)data.pvoidaddressdata1));
				entry.ULongLong1((ULONG64)data.pvoidaddressdata1);
				halList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::EnumHalPrivateDispatchTable(std::vector<winrt::StarlightGUI::GeneralEntry>& halPrivateList) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT {
			ULONG_PTR nSize;
			PVOID MiniFilterInfo;
		};

		BOOL bRet = FALSE;
		INPUT input = { 0 };
		PDATA_INFO pProcessInfo = NULL;

		input.nSize = sizeof(DATA_INFO) * 1000;
		input.MiniFilterInfo = (PVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, input.nSize);

		ULONG nRet = 0;
		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_ENUM_HALPRIVATEDISPATCHTABLE, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_ENUM_HALPRIVATEDISPATCHTABLE, &input, sizeof(INPUT), &nRet, sizeof(ULONG), 0, NULL);

		if (nRet > 1000) nRet = 1000;

		if (status && nRet > 0 && input.MiniFilterInfo) {
			pProcessInfo = (PDATA_INFO)input.MiniFilterInfo;
			for (ULONG i = 0; i < nRet; i++) {
				DATA_INFO data = pProcessInfo[i];
				auto entry = winrt::make<winrt::StarlightGUI::implementation::GeneralEntry>();
				entry.String1(to_hstring(data.Module));
				entry.String2(to_hstring(data.Module1));
				entry.String3(ULongToHexString(data.ulong64data1));
				entry.ULongLong1(data.ulong64data1);
				halPrivateList.push_back(entry);
			}
		}

		bRet = HeapFree(GetProcessHeap(), 0, input.MiniFilterInfo);
		return status && bRet;
	}

	BOOL KernelInstance::DeuteriumInvoke(DEUTERIUM_PROXY_INVOKE& function) noexcept {
		if (!GetDriverDevice()) return FALSE;

		LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_AX_DEUTERIUM_INVOKE, __WFUNCTION__.c_str());
		BOOL status = DeviceIoControl(driverDevice, IOCTL_AX_DEUTERIUM_INVOKE, &function, sizeof(DEUTERIUM_PROXY_INVOKE), &function, sizeof(DEUTERIUM_PROXY_INVOKE), 0, NULL);

		return status;
	}

	BOOL KernelInstance::DeuteriumAlloc(DEUTERIUM_PROXY_ALLOCATE& function, bool map) noexcept {
		if (!GetDriverDevice()) return FALSE;

		BOOL status = FALSE;
		if (map) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_AX_DEUTERIUM_ALLOCATE_MODERN, __WFUNCTION__.c_str());
			status = DeviceIoControl(driverDevice, IOCTL_AX_DEUTERIUM_ALLOCATE_MODERN, &function, sizeof(DEUTERIUM_PROXY_ALLOCATE), &function, sizeof(DEUTERIUM_PROXY_ALLOCATE), 0, NULL);
		}
		else {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_AX_DEUTERIUM_ALLOCATE, __WFUNCTION__.c_str());
			status = DeviceIoControl(driverDevice, IOCTL_AX_DEUTERIUM_ALLOCATE, &function, sizeof(DEUTERIUM_PROXY_ALLOCATE), &function, sizeof(DEUTERIUM_PROXY_ALLOCATE), 0, NULL);
		}

		return status;
	}

	BOOL KernelInstance::DeuteriumFree(DEUTERIUM_PROXY_FREE& function, bool map) noexcept {
		if (!GetDriverDevice()) return FALSE;

		BOOL status = FALSE;
		if (map) {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_AX_DEUTERIUM_FREE_MODERN, __WFUNCTION__.c_str());
			status = DeviceIoControl(driverDevice, IOCTL_AX_DEUTERIUM_FREE_MODERN, &function, sizeof(DEUTERIUM_PROXY_FREE), &function, sizeof(DEUTERIUM_PROXY_FREE), 0, NULL);
		}
		else {
			LOG_WARNING(L"KernelInstance", L"Calling 0x%x from \"%s\"", IOCTL_AX_DEUTERIUM_FREE, __WFUNCTION__.c_str());
			status = DeviceIoControl(driverDevice, IOCTL_AX_DEUTERIUM_FREE, &function, sizeof(DEUTERIUM_PROXY_FREE), &function, sizeof(DEUTERIUM_PROXY_FREE), 0, NULL);
		}

		return status;
	}

	BOOL KernelInstance::ReadMemory(std::vector<BYTE>& data, PVOID address, ULONG size) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct CHECK_INPUT
		{
			ULONG Size;
			PVOID Address;
		};
		CHECK_INPUT check_input = { 0 };
		check_input.Address = address;
		check_input.Size = size;

		BOOL status = DeviceIoControl(driverDevice, IOCTL_READ_MEMORY_CHECK, &check_input, sizeof(CHECK_INPUT), 0, 0, 0, NULL);

		if (!status) return FALSE;
		
		struct READ_INPUT
		{
			ULONG64 Size;
			PBYTE Data;
		};
		READ_INPUT read_input = { 0 };
		read_input.Data = new BYTE[size]();
		read_input.Size = size;

		status = DeviceIoControl(driverDevice, IOCTL_READ_MEMORY, &read_input, sizeof(READ_INPUT), &read_input, sizeof(READ_INPUT), 0, NULL);

		if (status) {
			data.assign(read_input.Data, read_input.Data + read_input.Size);
		}
		delete[] read_input.Data;

		return status && !data.empty();
	}

	BOOL KernelInstance::WriteMemory(PVOID address, PVOID data, ULONG size) noexcept {
		if (!GetDriverDevice()) return FALSE;

		struct INPUT
		{
			PVOID Address;
			PVOID Data;
			SIZE_T Size;
		};
		INPUT input = { 0 };
		input.Address = address;
		input.Size = size;
		input.Data = data;

		BOOL status = DeviceIoControl(driverDevice, IOCTL_WRITE_MEMORY, &input, sizeof(INPUT), 0, 0, 0, NULL);

		return status;
	}

	// =================================
	//				PRIVATE
	// =================================

	/*
	* 获取驱动设备位置
	*/
	BOOL KernelInstance::GetDriverDevice() noexcept {
		if (driverDevice != NULL) return TRUE;
		if (!DriverUtils::LoadKernelDriver(kernelPath.c_str())) return FALSE;

		HANDLE device = CreateFile(L"\\\\.\\ArkDrv64", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (device == INVALID_HANDLE_VALUE) return FALSE;

		driverDevice = device;
		return TRUE;
	}

	BOOL KernelInstance::GetDriverDevice2() noexcept {
		if (driverDevice2 != NULL) return TRUE;
		if (!DriverUtils::LoadKernelDriver(astralPath.c_str())) return FALSE;

		HANDLE device = CreateFile(L"\\\\.\\AstralX", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (device == INVALID_HANDLE_VALUE) return FALSE;

		driverDevice2 = device;
		return TRUE;
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

