#pragma once
#define SIRIUS_DRIVER_VERSION L"5.1.1"
#include "pch.h"
#if defined(_M_IX86)
#define PVOID PVOID64
#define ULONG_PTR unsigned long long
#endif
#pragma warning(push)
#pragma warning(disable: 4201)
/*
 * Sirius ARK IOCTL Definitions
 * @Author Stars
 *
 * Match driver version s1r10usly!
 */

 /*
  * Process operation section
  */
#define IOCTL_SIRIUS_SET_PROCESS_INFORMATION    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SIRIUS_QUERY_PROCESS_INFORMATION  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x001, METHOD_BUFFERED, FILE_ANY_ACCESS)
enum class ProcessSetInformation : ULONG {
    /* Operations */
    Terminate = 0,                              // Terminate the process. Arg=0: normal, Arg=1: thread, Arg=2: memory.
    Hide,                                       // Hide the process. (BSOD Warning)
    Suspend,                                    // Suspend the process.
    Resume,                                     // Resume the process.
    /* Attributes */
    Protection,                                 // [SI_PROCESS_PROTECTION], set the process protection state.
    Critical,                                   // [BOOLEAN state], set the process critical state.
    Token,                                      // [ULONG sourcePid], set the process token to target process.
    /* Modification */
    InjectDll,                                  // [SI_INJECT_DLL], inject a DLL into the process. Arg=0: CreateRemoteThread, Arg=1: QueueUserAPC, Arg=2: Manual Map.
};

enum class ProcessGetInformation : ULONG {
    BasicInformation = 0,                       // [SI_PROCESS_DATA], return the basic information of the process.
    FullInformation = 1,                        // [SI_PROCESS_DATA_FULL], return the full information of the process.
    Protection,                                 // [SI_PROCESS_PROTECTION], return the process protection state.
    Critical,                                   // [BOOLEAN state], return the process critical state.
    Thread,                                     // [SI_ENUMERATION], enum process threads.
    Module,                                     // [SI_ENUMERATION], enum process loaded modules.
    Handle,                                     // [SI_ENUMERATION], enum process handles.
    KernelCallbackTable,                        // [SI_ENUMERATION], enum process kernel callback table.
};

typedef struct _SI_PROCESS_INFORMATION {
    ULONG ProcessInformation;                   // Process information type.
    ULONG PID;                                  // Process ID.
    PVOID Buffer;                               // Argument buffer.
    ULONG Argument;                             // Extra arguments.
} SI_PROCESS_INFORMATION, * PSI_PROCESS_INFORMATION;

typedef struct _SI_PROCESS_PROTECTION
{
    UCHAR ProtectionType;                       // Process Protection Type (PS_PROTECTED_TYPE).
    UCHAR ProtectionLevel;                      // Process Protection Signer (PS_PROTECTED_SIGNER).
} SI_PROCESS_PROTECTION, * PSI_PROCESS_PROTECTION;

/*
 * Thread operation section
 */
#define IOCTL_SIRIUS_SET_THREAD_INFORMATION     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SIRIUS_QUERY_THREAD_INFORMATION   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x003, METHOD_BUFFERED, FILE_ANY_ACCESS)
enum class ThreadSetInformation : ULONG {
    /* Operations */
    Terminate = 0,                              // Terminate the thread. Arg=0: normal, Arg=1: super (BSOD warning).
    Suspend,                                    // Suspend the thread.
    Resume,                                     // Resume the thread.
    /* Attributes */
    PreviousMode,                               // [CHAR mode], set the thread previous mode.
    ApcQueueable,                               // [BOOLEAN state], set the thread ApcQueueable state.
    /* Modification */
    InjectDll,                                  // [SI_INJECT_DLL], inject a DLL into the thread. Arg=0: QueueUserAPC, Arg=1: Thread Hijacking, Arg=2: Manual Map.
};

enum class ThreadGetInformation : ULONG {
    BasicInformation = 0,                       // [SI_THREAD_DATA], return the basic information of the thread.
    FullInformation = 1,                        // [SI_THREAD_DATA_FULL], return the full information of the thread.
    PreviousMode,                               // [CHAR mode], return the thread previous mode.
    ApcQueueable,                               // [BOOLEAN state], return the thread ApcQueueable state.
};

typedef struct _SI_THREAD_INFORMATION {
    ULONG ThreadInformation;                    // Thread information type.
    ULONG TID;                                  // Thread ID.
    PVOID Buffer;                               // Argument buffer.
    ULONG Argument;                             // Extra arguments.
} SI_THREAD_INFORMATION, * PSI_THREAD_INFORMATION;

/*
 * File operation section
 */
#define IOCTL_SIRIUS_SET_FILE_INFORMATION       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x00E, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SIRIUS_QUERY_FILE_INFORMATION     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x00F, METHOD_BUFFERED, FILE_ANY_ACCESS)
enum class FileSetInformation : ULONG {
    Delete = 0,                                 // Delete the file or directory. Arg=0: normal, Arg=1: ntfs.
    Copy,                                       // [WCHAR targetPath], copy the file or directory. Arg=0: normal, Arg=1: ntfs.
    Rename                                      // [WCHAR targetPath], rename the file or directory. Arg=0: normal, Arg=1: ntfs.
};

enum class FileGetInformation : ULONG {
    FullInformation = 0,                        // WIP
    RawData,                                    // Read file raw data. Arg=0: normal, Arg=1: ntfs.
    DirectoryFile,                              // [SI_ENUMERATION], enum files. Arg=0: normal, Arg=1: ntfs direct.
    DirectoryFileByNTFS,                        // [SI_ENUMERATION], enum NTFS MFT. Arg=0: file only, Argument=1: full.
};

typedef struct _SI_FILE_INFORMATION {
    ULONG FileInformation;                      // File information type.
    WCHAR File[512];                            // File path.
    PVOID Buffer;                               // Argument buffer.
    ULONG Argument;                             // Extra arguments.
} SI_FILE_INFORMATION, * PSI_FILE_INFORMATION;

typedef struct _SI_FILE_RAW_READ {
    ULONG64 Offset;
    ULONG Size;
    ULONG BytesRead;
    ULONG64 FileSize;
    UCHAR Data[1];
} SI_FILE_RAW_READ, * PSI_FILE_RAW_READ;

/*
 * System operation section
 */
#define IOCTL_SIRIUS_SET_SYSTEM_INFORMATION     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SIRIUS_QUERY_SYSTEM_INFORMATION   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x101, METHOD_BUFFERED, FILE_ANY_ACCESS)
enum class SystemSetInformation : ULONG {
    WriteMemory = 0,                            // [SI_MEMORY], write memory.
    TriggerBugCheck,
    LoadImage,                                  // [SI_LOAD_IMAGE], load a image.
    UnloadImage,                                // [SI_UNLOAD_IMAGE], unload a image.
    RemoveCallback,                             // [SI_REMOVE_CALLBACK], remove a callback.
    RemoveIOTimer,                              // [PDEVICE_OBJECT], remove a IO timer.
    RemoveDPCTimer,                             // [PKTIMER], remove a DPC timer.
    CreateProcessState,                         // [BOOLEAN state], set process creation enabled/disabled state.
    CreateFileState,                            // [BOOLEAN state], set file creation enabled/disabled state.
    DSEState,                                   // [BOOLEAN state], set DSE enabled/disabled state. Arg=0: normal, Arg=1: by virtualization.
    LKDState,                                   // Enable Local Kernel Debugger dynamically.
    DisablePatchGuard,                          // Disable PatchGuard. Arg=0: normal, Arg=1: by virtualization.
    /* Symbols */
    RemoveFromPiDDBCacheTable,                  // [SI_REMOVE_PIDDB_CACHE], remove a PiDDB cache table entry.
    RemoveFromMmUnloadedDrivers,                // [SI_REMOVE_UNLOADED_DRIVER], remove a unloaded driver entry.
};

enum class SystemGetInformation : ULONG {
    /* Objects */
    Process = 0,                                // [SI_ENUMERATION], enum processes.
    Thread,                                     // [SI_ENUMERATION], enum threads.
    Module,                                     // [SI_ENUMERATION], enum modules.
    Handle,                                     // [SI_ENUMERATION], enum handles.
    IOTimer,                                    // [SI_ENUMERATION], enum IO timers.
    DPCTimer,                                   // [SI_ENUMERATION], enum DPC timers.
    Minifilter,                                 // [SI_ENUMERATION], enum minifilters.
    ObjectType,                                 // [SI_ENUMERATION], enum object types.
    Resource,                                   // [SI_ENUMERATION], enum system resources.
    Job,                                        // [SI_ENUMERATION], enum job objects.
    SSDT,                                       // [SI_ENUMERATION], enum SSDT.
    ShadowSSDT,                                 // [SI_ENUMERATION], enum ShadowSSDT.
    FilterSSDT,                                 // [SI_ENUMERATION], enum FilterSSDT.
    IDT,                                        // [SI_ENUMERATION], enum IDT.
    GDT,                                        // [SI_ENUMERATION], enum GDT.
    Callback,                                   // [SI_ENUMERATION], enum callbacks.
    MADT_Entries,                               // [SI_ENUMERATION], enum MADT entries.
    /* System private symbols */
    HalDispatchTable,                           // [SI_ENUMERATION], enum HalDispatchTable.
    HalPrivateDispatchTable,                    // [SI_ENUMERATION], enum HalPrivateDispatchTable.
    HalIommuDispatchTable,                      // [SI_ENUMERATION], enum HalIommuDispatchTable.
    HalAcpiDispatchTable,                       // [SI_ENUMERATION], enum HalAcpiDispatchTable.
    HalSubComponents,                           // [SI_ENUMERATION], enum HalSubComponents.
    SeCiCallbacks,                              // [SI_ENUMERATION], enum SeCiCallbacks.
    PiDDBCacheTable,                            // [SI_ENUMERATION], enum PiDDB cache table.
    MmUnloadedDrivers,                          // [SI_ENUMERATION], enum MmUnloadedDrivers.
    /* Operations */
    ReadMemory,                                 // [SI_MEMORY], read memory.
    /* Firmware */
    Firmware,                                   // [SI_FIRMWARE_DATA], get firmware table info. Arg=FirmwareType.
};

typedef struct _SI_SYSTEM_INFORMATION {
    ULONG SystemInformation;                    // System information type.
    PVOID Buffer;                               // Argument buffer.
    ULONG Argument;                             // Extra arguments.
} SI_SYSTEM_INFORMATION, * PSI_SYSTEM_INFORMATION;

/*
 * Driver operation section
 */
#define SI_ERROR_DETAIL_MAX_LENGTH              512
#define IOCTL_SIRIUS_GET_ERROR_CODE            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SIRIUS_GET_ERROR_DETAIL          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x701, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SIRIUS_GET_HITOKOTO              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x702, METHOD_BUFFERED, FILE_ANY_ACCESS)
typedef struct _SI_ERROR_DETAIL {
    ULONG Code;
    WCHAR Data[SI_ERROR_DETAIL_MAX_LENGTH];
} SI_ERROR_DETAIL, * PSI_ERROR_DETAIL;

#define IOCTL_SET_MONITOR		    	        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x600, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_LOG		    	            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x601, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_LOG_DETAILED		    	    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x602, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MONITOR_PROCESSES		    	    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x610, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MONITOR_THREADS		    	    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x611, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MONITOR_IMAGES		    	    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x612, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MONITOR_REGISTRIES		        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x613, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MONITOR_FILES		    	        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x614, METHOD_BUFFERED, FILE_ANY_ACCESS)


// 虚拟化：如果由某个进程打开设备句柄并发送 IOCTL 开启，必须在退出时关闭，未关闭会自动关
// 可在注册表 HKLM\\SOFTWARE\\Sirius 创建 DWORD AutoVirtualization=1 以自动启动虚拟化
#define IOCTL_METAVERSE_INITIALIZE          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_METAVERSE_EXIT                CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_METAVERSE_CHECK_SUPPORT       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)

enum class CallbackType : ULONG {
    CreateProcess = 0,
    CreateThread,
    LoadImage,
    Object,
    Registry,
    PowerSetting,
    PlugPlay,
    Shutdown,
    LastChanceShutdown,
    FileSystemChange,
    BugCheck,
    BugCheckReason,
    ExCallback,
    LogonSessionTerminated,
    LogonSessionTerminatedEx,
    DbgPrint,
    IoPriority,
    Coalescing,
    ImageVerification,
    Nmi
};

enum class ObCallbackType : ULONG {
    Process = 0,
    Thread,
    Desktop
};

enum class FirmwareType : ULONG {
    BGRT = 0,                                   // Boot Graphics Resource Table.
    FPDT,                                       // Firmware Performance Data Table.
    UEFI,                                       // Unified Extensible Firmware Interface info.
    RSDT,                                       // RSDT/XSDT root table info.
    FADT,                                       // Fixed ACPI Description Table.
    MADT,                                       // Multiple APIC Description Table.
    GTDT,                                       // Generic Timer Description Table.
    CSRT,                                       // Core System Resources Table.
    DBG2,                                       // Debug Port Table 2.
    WSMT,                                       // Windows SMM Security Mitigations Table.
    iBFT,                                       // iSCSI Boot Firmware Table.
};

typedef struct _SI_REMOVE_CALLBACK {
    ULONG Type;
    PVOID Address;
    PVOID Address2;
} SI_REMOVE_CALLBACK, * PSI_REMOVE_CALLBACK;

typedef struct _SI_REMOVE_PIDDB_CACHE {
    WCHAR Name[256];
    ULONG Timestamp;
} SI_REMOVE_PIDDB_CACHE, * PSI_REMOVE_PIDDB_CACHE;

typedef struct _SI_REMOVE_UNLOADED_DRIVER {
    WCHAR Name[256];
    PVOID StartAddress;
    PVOID EndAddress;
    LARGE_INTEGER CurrentTime;
} SI_REMOVE_UNLOADED_DRIVER, * PSI_REMOVE_UNLOADED_DRIVER;

typedef struct _SI_LOAD_IMAGE {
    WCHAR Path[512];
    BOOLEAN LoadAsDriver;
} SI_LOAD_IMAGE, * PSI_LOAD_IMAGE;

typedef struct _SI_UNLOAD_IMAGE {
    WCHAR ServiceName[256];
    PVOID Base;
    BOOLEAN UnloadAsDriver;
} SI_UNLOAD_IMAGE, * PSI_UNLOAD_IMAGE;

typedef struct _SI_INJECT_DLL {
    WCHAR DllPath[512];
    ULONG Method;
    ULONG Flags;
} SI_INJECT_DLL, * PSI_INJECT_DLL;

typedef struct _SI_PROCESS_DATA {
    PVOID Eprocess;
    ULONG Pid;
    ULONG ParentPid;
    ULONG64 VirtualSize;
    ULONG64 WorkingSetSize;
    ULONG64 WorkingSetPrivateSize;
    WCHAR ImageName[256];
    WCHAR ImagePath[512];
    UCHAR ProtectionType;
    UCHAR ProtectionLevel;
} SI_PROCESS_DATA, * PSI_PROCESS_DATA;

typedef struct _SI_THREAD_DATA {
    PVOID Ethread;
    ULONG Tid;
    ULONG ParentPid;
    PVOID StartAddress;
    PVOID Win32StartAddress;
    enum _KTHREAD_STATE State;
    CHAR PreviousMode;
    ULONG Priority;
} SI_THREAD_DATA, * PSI_THREAD_DATA;

typedef struct _SI_IO_TIMER_DATA {
    WCHAR Path[512];
    SHORT Type;
    SHORT TimerFlag;
    PVOID TimerRoutine;
    PVOID Context; // OUT OPTIONAL
    PVOID DeviceObject;
} SI_IO_TIMER_DATA, * PSI_IO_TIMER_DATA;

typedef struct _SI_DPC_TIMER_DATA {
    WCHAR Path[512];
    PVOID Timer;
    PVOID DPC;
    PVOID DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    LONG Period;
    ULONG64 DueTime;
    ULONG Processor;
} SI_DPC_TIMER_DATA, * PSI_DPC_TIMER_DATA;

typedef struct _SI_CALLBACK_DATA {
    WCHAR Path[512];
    PVOID Address;
    PVOID Address2; // OUT OPTIONAL
    PVOID Address3; // OUT OPTIONAL
    PVOID Address4; // OUT OPTIONAL
    ULONG Index;
    ULONG Flag; // OUT OPTIONAL
} SI_CALLBACK_DATA, * PSI_CALLBACK_DATA;

typedef struct _SI_MINIFILTER_DATA {
    WCHAR Name[256];
    WCHAR Path[512];
    PVOID Base;
    PVOID DriverObject;
    UCHAR MajorFunction;
    ULONG Flags; // OUT OPTIONAL
    PVOID PreOperation; // OUT OPTIONAL
    PVOID PostOperation; // OUT OPTIONAL
} SI_MINIFILTER_DATA, * PSI_MINIFILTER_DATA;

typedef struct _SI_OBJECT_TYPE_DATA {
    WCHAR Name[128];
} SI_OBJECT_TYPE_DATA, * PSI_OBJECT_TYPE_DATA;

typedef struct _SI_JOB_DATA {
    WCHAR Name[256];
    PVOID Job;
    PVOID Affinity;
    ULONG TotalProcesses;
    ULONG ActiveProcesses;
} SI_JOB_DATA, * PSI_JOB_DATA;

typedef struct _SI_ERESOURCE_DATA {
    PVOID Resource;
    LONG ActiveCount;
    ULONG ContentionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
    ULONG Flag;
} SI_ERESOURCE_DATA, * PSI_ERESOURCE_DATA;

typedef struct _SI_FUNCTION_DATA {
    CHAR Name[128];
    PVOID Address;
} SI_FUNCTION_DATA, * PSI_FUNCTION_DATA;

typedef struct _SI_IDT_DATA {
    PVOID Offset;
    USHORT Selector;
    UCHAR Type;
    UCHAR Dpl;
} SI_IDT_DATA, * PSI_IDT_DATA;

typedef struct _SI_GDT_DATA {
    PVOID Base;
    ULONG64 Limit;
    UCHAR Dpl;
    UCHAR Type;
    UCHAR Granularity;
} SI_GDT_DATA, * PSI_GDT_DATA;

typedef struct _SI_PIDDB_CACHE_DATA {
    WCHAR Name[256];
    ULONG Timestamp;
    ULONG LoadStatus;
} SI_PIDDB_CACHE_DATA, * PSI_PIDDB_CACHE_DATA;

typedef struct _SI_UNLOADED_DRIVER_DATA {
    WCHAR Name[256];
    PVOID StartAddress;
    PVOID EndAddress;
    LARGE_INTEGER CurrentTime;
} SI_UNLOADED_DRIVER_DATA, * PSI_UNLOADED_DRIVER_DATA;

typedef struct _SI_FILE_DATA {
    WCHAR Name[256];
    WCHAR Path[512];
    BOOLEAN Directory;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER ChangeTime;
    ULONG64 AllocatedSize;
    ULONG64 DataSize;
    ULONG FileAttributes;
} SI_FILE_DATA, * PSI_FILE_DATA;

typedef struct _SI_MODULE_DATA {
    WCHAR Name[256];
    WCHAR Path[512];
    PVOID Base;
    ULONG Size;
    PVOID DriverObject;
} SI_MODULE_DATA, * PSI_MODULE_DATA;

typedef struct _SI_HANDLE_DATA {
    WCHAR TypeName[64];
    HANDLE Handle;
    PVOID Object;
    ULONG GrantedAccess;
    ULONG Attributes;
} SI_HANDLE_DATA, * PSI_HANDLE_DATA;

typedef struct _SI_ENUMERATION {
    PVOID Buffer; // IN OUT
    ULONG BufferSize;
    ULONG Count;
    PVOID Arg; // IN OPTIONAL
} SI_ENUMERATION, * PSI_ENUMERATION;

typedef struct _SI_MEMORY {
    PVOID Address;
    ULONG Size;
    UCHAR Data[1];
} SI_MEMORY, * PSI_MEMORY;

typedef struct _SI_LOG_ENTRY {
    WCHAR Data[512];
    ULONG Length;
} SI_LOG_ENTRY, * PSI_LOG_ENTRY;

typedef struct _SI_FIRMWARE_ACPI_HEADER {
    UCHAR Revision;
    UCHAR Checksum;
    ULONG Length;
    CHAR Signature[5];
    CHAR OemId[7];
    CHAR OemTableId[9];
    ULONG OemRevision;
    ULONG CreatorId;
    ULONG CreatorRevision;
} SI_FIRMWARE_ACPI_HEADER, * PSI_FIRMWARE_ACPI_HEADER;

typedef struct _SI_MADT_ENTRY_DATA {
    UCHAR Type;
    UCHAR Length;
    UCHAR ProcessorId;
    UCHAR ApicId;
    ULONG Flags;
    ULONG Address;
    ULONG Gsi;
    UCHAR Bus;
    UCHAR Source;
    USHORT IntFlags;
    ULONG X2ApicId;
    ULONG ProcessorUid;
    CHAR TypeName[32];
} SI_MADT_ENTRY_DATA, * PSI_MADT_ENTRY_DATA;

typedef struct _SI_FIRMWARE_DATA {
    ULONG Type;
    SI_FIRMWARE_ACPI_HEADER Header;
    union {
        struct {
            BOOLEAN Xsdt;
            ULONG EntryCount;
            ULONG RsdtAddress;
            ULONG64 XsdtAddress;
            ULONG64 RootTableAddress;
        } Rsdt;
        struct {
            ULONG64 ImageAddress;
            ULONG ImageOffsetX;
            ULONG ImageOffsetY;
            ULONG ImageWidth;
            ULONG ImageHeight;
            UCHAR ImageType;
            UCHAR Status;
            UCHAR Version;
        } Bgrt;
        struct {
            ULONG BootRecordCount;
            ULONG64 BootPerformanceAddress;
            ULONG64 S3PerformanceAddress;
            ULONG64 ResetEnd;
            ULONG64 OsLoaderLoadImageStart;
            ULONG64 OsLoaderStartImageStart;
            ULONG64 ExitBootServicesEntry;
            ULONG64 ExitBootServicesExit;
            ULONG64 S3ResumeCount;
            ULONG64 S3FullResume;
            ULONG64 S3AverageResume;
        } Fpdt;
        struct {
            BOOLEAN SecureBootEnabled;
            BOOLEAN SecureBootCapable;
            BOOLEAN SetupMode;
            BOOLEAN AuditMode;
            BOOLEAN DeployedMode;
            BOOLEAN BootOptionSupport;
            BOOLEAN BootCurrentValid;
            BOOLEAN BootNextValid;
            BOOLEAN TimeoutValid;
            BOOLEAN PlatformRecoverySupport;
            USHORT BootCurrent;
            USHORT BootNext;
            USHORT Timeout;
            ULONG64 OsIndications;
            ULONG64 OsIndicationsSupported;
            WCHAR PlatformLang[64];
            WCHAR Lang[64];
        } Uefi;
        struct {
            ULONG FirmwareCtrl;
            ULONG Dsdt;
            UCHAR PreferredPmProfile;
            USHORT SciInt;
            ULONG SmiCmd;
            UCHAR AcpiEnable;
            UCHAR AcpiDisable;
            UCHAR S4BiosReq;
            UCHAR PstateCnt;
            ULONG Pm1aEvtBlk;
            ULONG Pm1bEvtBlk;
            ULONG Pm1aCntBlk;
            ULONG Pm1bCntBlk;
            ULONG Pm2CntBlk;
            ULONG PmTmrBlk;
            ULONG Gpe0Blk;
            ULONG Gpe1Blk;
            UCHAR Pm1EvtLen;
            UCHAR Pm1CntLen;
            UCHAR Pm2CntLen;
            UCHAR PmTmrLen;
            UCHAR Gpe0BlkLen;
            UCHAR Gpe1BlkLen;
            UCHAR Gpe1Base;
            UCHAR CstCnt;
            USHORT PLvl2Lat;
            USHORT PLvl3Lat;
            USHORT FlushSize;
            USHORT FlushStride;
            UCHAR DutyOffset;
            UCHAR DutyWidth;
            UCHAR DayAlrm;
            UCHAR MonAlrm;
            UCHAR Century;
            USHORT IapcBootArch;
            ULONG Flags;
            UCHAR ResetValue;
            USHORT ArmBootArch;
            UCHAR FadtMinorVersion;
            ULONG64 XFirmwareCtrl;
            ULONG64 XDsdt;
            ULONG64 HypervisorVendorId;
        } Fadt;
        struct {
            ULONG LocalApicAddress;
            ULONG Flags;
            ULONG EntryCount;
        } Madt;
        struct {
            ULONG64 CounterBlockAddresss;
            ULONG SecureEL1TimerGSIV;
            ULONG SecureEL1TimerFlags;
            ULONG NonSecureEL1TimerGSIV;
            ULONG NonSecureEL1TimerFlags;
            ULONG VirtualTimerGSIV;
            ULONG VirtualTimerFlags;
            ULONG NonSecureEL2TimerGSIV;
            ULONG NonSecureEL2TimerFlags;
            ULONG64 CounterReadBlockAddress;
            ULONG PlatformTimerCount;
            ULONG PlatformTimerOffset;
        } Gtdt;
        struct {
            ULONG ResourceGroupCount;
        } Csrt;
        struct {
            ULONG OffsetDbgDeviceInfo;
            ULONG NumberDbgDeviceInfo;
        } Dbg2;
        struct {
            ULONG ProtectionFlags;
        } Wsmt;
    };
} SI_FIRMWARE_DATA, * PSI_FIRMWARE_DATA;

typedef enum _PS_PROTECTED_TYPE
{
    PsProtectedTypeNone = 0,
    PsProtectedTypeProtectedLight,
    PsProtectedTypeProtected
} PS_PROTECTED_TYPE, * PPS_PROTECTED_TYPE;

typedef enum _PS_PROTECTED_SIGNER
{
    PsProtectedSignerNone = 0,
    PsProtectedSignerAuthenticode,
    PsProtectedSignerCodeGen,
    PsProtectedSignerAntimalware,
    PsProtectedSignerLsa,
    PsProtectedSignerWindows,
    PsProtectedSignerWinTcb,
    PsProtectedSignerWinSystem,
    PsProtectedSignerApp
} PS_PROTECTED_SIGNER, * PPS_PROTECTED_SIGNER;

typedef enum _KTHREAD_STATE
{
    Initialized = 0,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
    GateWaitObsolete,
    WaitingForProcessInSwap
} KTHREAD_STATE, * PKTHREAD_STATE;

typedef struct _SI_PROCESS_DATA_FULL {
    WCHAR ImageName[256];
    WCHAR ImagePath[512];
    WCHAR CommandLine[1024];
    PVOID Eprocess;
    ULONG PID;
    ULONG ParentPID;
    UCHAR PriorityClass;
    ULONG BasePriority;
    ULONG IoPriority;
    ULONG PagePriority;
    SIZE_T VirtualSize;
    SIZE_T WorkingSetSize;
    SIZE_T PrivateWorkingSetSize;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    ULONG HandleCount;
    ULONG SessionID;
    ULONG Cookie;
    UCHAR ProtectionType;
    UCHAR ProtectionSigner;
    PVOID Peb;
    PVOID KernelCallbackTable;
    ULONG ImageSubsystem;
    PVOID Token;
    PVOID ObjectTable;
    // Flags
    union {
        ULONG64 All;
        struct {
            ULONG64 IsWow64Process : 1;
            ULONG64 IsSecureProcess : 1;
            ULONG64 IsSubsystemProcess : 1;
            ULONG64 IsProtectedProcess : 1;
            ULONG64 IsProtectedProcessLight : 1;
            ULONG64 IsPackagedProcess : 1;
            ULONG64 IsAppContainer : 1;
            ULONG64 Foreground : 1;
            ULONG64 BreakOnTermination : 1;
            ULONG64 TokenVirtualization : 1;
            ULONG64 InPrivate : 1;
            ULONG64 ProhibitChildProcesses : 1;
            ULONG64 AlwaysAllowSecureChildProcess : 1;
            ULONG64 AuditProhibitChildProcesses : 1;
            ULONG64 HighGraphicsPriority : 1;
            ULONG64 BeingDebugged : 1;
            ULONG64 EnableCsrDebug : 1;
        };
    } Flags;
} SI_PROCESS_DATA_FULL, * PSI_PROCESS_DATA_FULL;

typedef struct _SI_THREAD_DATA_FULL {
    PVOID Ethread;
    ULONG TID;
    ULONG ParentPID;
    PVOID StartAddress;
    PVOID Win32StartAddress;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    KTHREAD_STATE State;
    CHAR PreviousMode;
    ULONG Priority;
    ULONG BasePriority;
    ULONG IoPriority;
    ULONG PagePriority;
    ULONG PriorityBoost;
    PVOID Teb;
    ULONG SuspendCount;
    ULONG ContextSwitches;
    union {
        ULONG64 All;
        struct {
            ULONG64 IsIoPending : 1;
            ULONG64 HideFromDebugger : 1;
            ULONG64 BreakOnTermination : 1;
        };
    } Flags;
} SI_THREAD_DATA_FULL, * PSI_THREAD_DATA_FULL;

typedef struct _SI_FILE_DATA_FULL {
    WCHAR Name[256];
    WCHAR Path[512];
    BOOLEAN Directory;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER ChangeTime;
    ULONG64 AllocatedSize;
    ULONG64 DataSize;
    ULONG FileAttributes;
    ULONG64 FileReference;
    ULONG64 ParentReference;
    ULONG NtfsFlags;
    ULONG AttributeType;
    USHORT AttributeId;
    USHORT SequenceNumber;
    BOOLEAN Resident;
    UCHAR FileNameNamespace;
    WCHAR AttributeName[128];
} SI_FILE_DATA_FULL, * PSI_FILE_DATA_FULL;

#pragma warning(pop)
