#pragma once

#include "pch.h"
#include "NTBase.h"
#include "SiriusIO.h"
#include "SiriusError.h"
#include "unordered_set"

// Avoid macro conflicts
#undef EnumProcesses
#undef EnumProcessModules

enum class HalTableType : ULONG {
	HalDispatchTable = 0,
	HalPrivateDispatchTable,
	HalIommuDispatchTable,
	HalAcpiDispatchTable,
	HalSubComponents
};

namespace winrt::StarlightGUI::implementation {
	class KernelInstance {
	public:
		// Error handling
		static SISTATUS GetLastErrorCode() noexcept;
		static std::wstring GetLastErrorMessage() noexcept;
		static void QueryError() noexcept;

		// Process
		static BOOL SiTerminateProcess(ULONG pid) noexcept;
		static BOOL SiTerminateProcessEx(ULONG pid) noexcept;

		static BOOL SiSuspendProcess(ULONG pid) noexcept;
		static BOOL SiResumeProcess(ULONG pid) noexcept;
		static BOOL SiHideProcess(ULONG pid) noexcept;
		static BOOL SetPPL(ULONG pid, int level) noexcept;
		static BOOL SetCriticalProcess(ULONG pid) noexcept;
		static BOOL InjectDLLToProcess(ULONG pid, PWCHAR dllPath, ULONG size) noexcept;
		static BOOL ModifyProcessToken(ULONG sourcePID, ULONG targetPID) noexcept;

		// Thread
		static BOOL SiTerminateThread(ULONG tid) noexcept;
		static BOOL SiTerminateThreadEx(ULONG tid) noexcept;

		static BOOL SiSuspendThread(ULONG tid) noexcept;
		static BOOL SiResumeThread(ULONG tid) noexcept;

		// Driver
		static BOOL SiUnloadDriver(ULONG64 driverObj) noexcept;
		static BOOL SiHideDriver(ULONG64 driverObj) noexcept;

		// Enum
		static BOOL SiEnumProcesses(std::vector<winrt::StarlightGUI::ProcessInfo>& targetList, bool strengthen) noexcept;
		static BOOL SiEnumProcessThreads(ULONG pid, std::vector<winrt::StarlightGUI::ThreadInfo>& threads) noexcept;
		static BOOL SiEnumProcessHandles(ULONG pid, std::vector<winrt::StarlightGUI::HandleInfo>& handles) noexcept;
		static BOOL SiEnumProcessModules(ULONG pid, std::vector<winrt::StarlightGUI::MokuaiInfo>& modules) noexcept;
		static BOOL SiEnumProcessKernelCallbackTable(ULONG pid, std::vector<winrt::StarlightGUI::KCTInfo>& kcts) noexcept;
		static BOOL SiEnumDrivers(std::vector<winrt::StarlightGUI::KernelModuleInfo>& kernelModules) noexcept;
		static BOOL SiEnumObjectsByDirectory(std::wstring objectPath, std::vector<winrt::StarlightGUI::ObjectEntry>& objectList) noexcept;
		static BOOL SiEnumCallbacks(std::vector<winrt::StarlightGUI::GeneralEntry>& callbacks, CallbackType type = CallbackType::CreateProcess) noexcept;
		static BOOL SiEnumMiniFilter(std::vector<winrt::StarlightGUI::GeneralEntry>& miniFilters) noexcept;
		static BOOL SiEnumSSDT(std::vector<winrt::StarlightGUI::GeneralEntry>& ssdts) noexcept;
		static BOOL SiEnumSSSDT(std::vector<winrt::StarlightGUI::GeneralEntry>& sssdts) noexcept;
		static BOOL SiEnumIoTimer(std::vector<winrt::StarlightGUI::GeneralEntry>& timers) noexcept;
		static BOOL SiEnumIDT(std::vector<winrt::StarlightGUI::GeneralEntry>& idtEntries) noexcept;
		static BOOL SiEnumGDT(std::vector<winrt::StarlightGUI::GeneralEntry>& gdtEntries) noexcept;
		static BOOL SiEnumPiDDBCacheTable(std::vector<winrt::StarlightGUI::GeneralEntry>& piddbEntries) noexcept;
		static BOOL SiEnumHalDispatchTable(std::vector<winrt::StarlightGUI::GeneralEntry>& halEntries, HalTableType type = HalTableType::HalDispatchTable) noexcept;

		// Kernel Objects
		static BOOL RemoveCallback(winrt::StarlightGUI::GeneralEntry& entry) noexcept;
		static BOOL RemoveMiniFilter(winrt::StarlightGUI::GeneralEntry& entry) noexcept;
		static BOOL RemovePiDDBCache(winrt::StarlightGUI::GeneralEntry& entry) noexcept;

		// File
		static BOOL QueryFile(std::wstring path, std::vector<winrt::StarlightGUI::FileInfo>& files) noexcept;
		static BOOL DeleteFileAuto(std::wstring path) noexcept;
		static BOOL SiDeleteFile(std::wstring path) noexcept;
		static BOOL SiDeleteFileEx(std::wstring path) noexcept;
		static BOOL SiLockFile(std::wstring path) noexcept;
		static BOOL SiCopyFile(std::wstring from, std::wstring to) noexcept;
		static BOOL SiRenameFile(std::wstring from, std::wstring to) noexcept;

		// System
		static BOOL EnableHypervisor() noexcept;
		static BOOL DisableHypervisor() noexcept;
		static BOOL EnableCreateProcess() noexcept;
		static BOOL DisableCreateProcess() noexcept;
		static BOOL EnableCreateFile() noexcept;
		static BOOL DisableCreateFile() noexcept;
		static BOOL EnableModifyRegistry() noexcept;
		static BOOL DisableModifyRegistry() noexcept;
		static BOOL EnableDSE(bool hypervisor = false) noexcept;
		static BOOL DisableDSE(bool hypervisor = false) noexcept;
		static BOOL EnableLKD() noexcept;
		static BOOL DisablePatchGuard(bool hypervisor = false) noexcept;
		static BOOL BlueScreen();

		// Object
		static BOOL GetObjectDetails(std::wstring fullPath, std::wstring type, winrt::StarlightGUI::ObjectEntry& object) noexcept;

		// Memory
		static BOOL ReadMemory(std::vector<BYTE>& data, PVOID address, ULONG size) noexcept;
		static BOOL WriteMemory(PVOID address, PVOID data, ULONG size) noexcept;

	private:
		static BOOL GetDriverDevice() noexcept;
		static BOOL SiSetProcessInformation(ProcessSetInformation processInformation, ULONG pid, PVOID buffer, ULONG argument) noexcept;
		static BOOL SiQueryProcessInformation(ProcessGetInformation processInformation, ULONG pid, PVOID buffer, ULONG argument) noexcept;
		static BOOL SiSetThreadInformation(ThreadSetInformation threadInformation, ULONG tid, PVOID buffer, ULONG argument) noexcept;
		static BOOL SiQueryThreadInformation(ThreadGetInformation threadInformation, ULONG tid, PVOID buffer, ULONG argument) noexcept;
		static BOOL SiSetFileInformation(FileSetInformation fileInformation, LPCWSTR filePath, PVOID buffer, ULONG argument) noexcept;
		static BOOL SiQueryFileInformation(FileGetInformation fileInformation, LPCWSTR filePath, PVOID buffer, ULONG argument) noexcept;
		static BOOL SiSetSystemInformation(SystemSetInformation systemInformation, PVOID buffer, ULONG argument) noexcept;
		static BOOL SiQuerySystemInformation(SystemGetInformation systemInformation, PVOID buffer, ULONG argument) noexcept;
		static BOOL QuerySystemEnumeration(SystemGetInformation information, SI_ENUMERATION& enumData, ULONG itemSize, ULONG argument = 0) noexcept;
		static BOOL QueryProcessEnumeration(ProcessGetInformation information, ULONG pid, SI_ENUMERATION& enumData, ULONG itemSize, ULONG argument = 0) noexcept;
		static BOOL QueryFileEnumeration(FileGetInformation information, LPCWSTR path, SI_ENUMERATION& enumData, ULONG itemSize, ULONG argument = 0) noexcept;
		static std::string GetMiniFilterMajorFunction(ULONG64 Index) noexcept;
	};

	class DriverUtils {

	public:
		static bool LoadKernelDriver(LPCWSTR kernelPath) noexcept;
		static bool LoadDriver(LPCWSTR kernelPath, LPCWSTR fileName) noexcept;
		static bool StopKernelDriver() noexcept;
		static void FixServices() noexcept;
	};
}
