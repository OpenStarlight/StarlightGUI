#pragma once

#include "pch.h"
#include "NTBase.h"
#include "IOBase.h"
#include "AstralIO.h"
#include "unordered_set"

// Avoid macro conflicts
#undef EnumProcesses
#undef EnumProcessModules

namespace winrt::StarlightGUI::implementation {
	class KernelInstance {
	public:
		// Process
		static BOOL _ZwTerminateProcess(ULONG pid) noexcept;
		static BOOL MurderProcess(ULONG pid) noexcept;

		static BOOL _SuspendProcess(ULONG pid) noexcept;
		static BOOL _ResumeProcess(ULONG pid) noexcept;
		static BOOL HideProcess(ULONG pid) noexcept;
		static BOOL SetPPL(ULONG pid, int level) noexcept;
		static BOOL SetCriticalProcess(ULONG pid) noexcept;
		static BOOL InjectDLLToProcess(ULONG pid, PWCHAR dllPath) noexcept;
		static BOOL ModifyProcessToken(ULONG pid, ULONG type) noexcept;

		// Thread
		static BOOL _ZwTerminateThread(ULONG tid) noexcept;
		static BOOL MurderThread(ULONG tid) noexcept;

		static BOOL _SuspendThread(ULONG tid) noexcept;
		static BOOL _ResumeThread(ULONG tid) noexcept;

		// Driver
		static BOOL UnloadDriver(ULONG64 driverObj) noexcept;
		static BOOL HideDriver(ULONG64 driverObj) noexcept;

		// Enum
		static BOOL EnumProcesses(std::vector<winrt::StarlightGUI::ProcessInfo>& targetList) noexcept;
		static BOOL EnumProcesses2(std::vector<winrt::StarlightGUI::ProcessInfo>& targetList) noexcept;
		static BOOL EnumProcessThreads(ULONG64 eprocess, std::vector<winrt::StarlightGUI::ThreadInfo>& threads) noexcept;
		static BOOL EnumProcessHandles(ULONG pid, std::vector<winrt::StarlightGUI::HandleInfo>& handles) noexcept;
		static BOOL EnumProcessModules(ULONG64 eprocess, std::vector<winrt::StarlightGUI::MokuaiInfo>& threads) noexcept;
		static BOOL EnumProcessKernelCallbackTable(ULONG64 eprocess, std::vector<winrt::StarlightGUI::KCTInfo>& threads) noexcept;
		static BOOL EnumDrivers(std::vector<winrt::StarlightGUI::KernelModuleInfo>& kernelModules) noexcept;
		static BOOL EnumObjectsByDirectory(std::wstring objectPath, std::vector<winrt::StarlightGUI::ObjectEntry>& objectList) noexcept;
		static BOOL EnumNotifies(std::vector<winrt::StarlightGUI::GeneralEntry>& callbacks) noexcept;
		static BOOL EnumMiniFilter(std::vector<winrt::StarlightGUI::GeneralEntry>& miniFilters) noexcept;
		static BOOL EnumStandardFilter(std::vector<winrt::StarlightGUI::GeneralEntry>& filters) noexcept;
		static BOOL EnumSSDT(std::vector<winrt::StarlightGUI::GeneralEntry>& ssdts) noexcept;
		static BOOL EnumSSSDT(std::vector<winrt::StarlightGUI::GeneralEntry>& sssdts) noexcept;
		static BOOL EnumExCallback(std::vector<winrt::StarlightGUI::GeneralEntry>& callbacks) noexcept;
		static BOOL EnumIoTimer(std::vector<winrt::StarlightGUI::GeneralEntry>& timers) noexcept;
		static BOOL EnumIDT(std::vector<winrt::StarlightGUI::GeneralEntry>& idtEntries) noexcept;
		static BOOL EnumGDT(std::vector<winrt::StarlightGUI::GeneralEntry>& gdtEntries) noexcept;
		static BOOL EnumPiDDBCacheTable(std::vector<winrt::StarlightGUI::GeneralEntry>& piddbEntries) noexcept;
		static BOOL EnumHalDispatchTable(std::vector<winrt::StarlightGUI::GeneralEntry>& halEntries) noexcept;
		static BOOL EnumHalPrivateDispatchTable(std::vector<winrt::StarlightGUI::GeneralEntry>& halPrivateEntries) noexcept;

		// Kernel Objects
		static BOOL RemoveNotify(winrt::StarlightGUI::GeneralEntry& entry) noexcept;
		static BOOL RemoveMiniFilter(winrt::StarlightGUI::GeneralEntry& entry) noexcept;
		static BOOL RemoveStandardFilter(winrt::StarlightGUI::GeneralEntry& entry) noexcept;
		static BOOL UnhookSSDT(winrt::StarlightGUI::GeneralEntry& entry) noexcept;
		static BOOL UnhookSSSDT(winrt::StarlightGUI::GeneralEntry& entry) noexcept;
		static BOOL RemoveExCallback(winrt::StarlightGUI::GeneralEntry& entry) noexcept;
		static BOOL RemovePiDDBCache(winrt::StarlightGUI::GeneralEntry& entry) noexcept;

		// File
		static BOOL QueryFile(std::wstring path, std::vector<winrt::StarlightGUI::FileInfo>& files) noexcept;
		static BOOL DeleteFileAuto(std::wstring path) noexcept;
		static BOOL _DeleteFileAuto(std::wstring path) noexcept;
		static BOOL MurderFileAuto(std::wstring path) noexcept;
		static BOOL LockFile(std::wstring path) noexcept;
		static BOOL _CopyFile(std::wstring from, std::wstring to, std::wstring name) noexcept;
		static BOOL _RenameFile(std::wstring from, std::wstring to) noexcept;

		// System
		static BOOL EnableHVM() noexcept;
		static BOOL EnableCreateProcess() noexcept;
		static BOOL DisableCreateProcess() noexcept;
		static BOOL EnableCreateFile() noexcept;
		static BOOL DisableCreateFile() noexcept;
		static BOOL EnableLoadDriver() noexcept;
		static BOOL DisableLoadDriver() noexcept;
		static BOOL EnableUnloadDriver() noexcept;
		static BOOL DisableUnloadDriver() noexcept;
		static BOOL EnableModifyRegistry() noexcept;
		static BOOL DisableModifyRegistry() noexcept;
		static BOOL ProtectDisk() noexcept;
		static BOOL UnprotectDisk() noexcept;
		static BOOL EnableDSE() noexcept;
		static BOOL DisableDSE() noexcept;
		static BOOL EnableObCallback() noexcept;
		static BOOL DisableObCallback() noexcept;
		static BOOL EnableCmpCallback() noexcept;
		static BOOL DisableCmpCallback() noexcept;
		static BOOL EnableLKD() noexcept;
		static BOOL DisableLKD() noexcept;
		static BOOL EnableEPTScan() noexcept;
		static BOOL DisableEPTScan() noexcept;
		static BOOL DisablePatchGuard(int type) noexcept;
		static BOOL Shutdown();
		static BOOL Reboot();
		static BOOL RebootForce();
		static BOOL BlueScreen(int color);

		// Object
		static BOOL GetObjectDetails(std::wstring fullPath, std::wstring type, winrt::StarlightGUI::ObjectEntry& object) noexcept;

		// Deuterium
		static BOOL DeuteriumInvoke(DEUTERIUM_PROXY_INVOKE& function) noexcept;
		static BOOL DeuteriumAlloc(DEUTERIUM_PROXY_ALLOCATE& function, bool map) noexcept;
		static BOOL DeuteriumFree(DEUTERIUM_PROXY_FREE& function, bool map) noexcept;

		// Memory
		static BOOL ReadMemory(std::vector<BYTE>& data, PVOID address, ULONG size) noexcept;
		static BOOL WriteMemory(PVOID address, PVOID data, ULONG size) noexcept;

	private:
		static BOOL GetDriverDevice() noexcept;
		static BOOL GetDriverDevice2() noexcept;
		static BOOL _DeleteFile(std::wstring path) noexcept;
		static BOOL MurderFile(std::wstring path) noexcept;
		static std::string GetMiniFilterMajorFunction(ULONG64 Index) noexcept;
	};

	class KernelBase {
	public:
		static DWORD64 GetCIBaseAddress();
	};

	class DriverUtils {

	public:
		static bool LoadKernelDriver(LPCWSTR kernelPath) noexcept;
		static bool LoadDriver(LPCWSTR kernelPath, LPCWSTR fileName) noexcept;
		static void FixServices() noexcept;
	};
}