#pragma once

#include <Windows.h>

#pragma comment(lib, "ntdll.lib")

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING* PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES* POBJECT_ATTRIBUTES;

typedef struct _IO_STATUS_BLOCK {
	union {
		LONG Status;
		PVOID Pointer;
	} DUMMYUNIONNAME;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

EXTERN_C VOID NTAPI RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString);
EXTERN_C LONG NTAPI NtQuerySystemInformation(
	ULONG systemInformationClass,
	PVOID systemInformation,
	ULONG systemInformationLength,
	PULONG returnLength);
EXTERN_C LONG NTAPI NtQueryObject(
	HANDLE handle,
	ULONG objectInformationClass,
	PVOID objectInformation,
	ULONG objectInformationLength,
	PULONG returnLength);
EXTERN_C LONG NTAPI NtQueryDirectoryObject(
	HANDLE directoryHandle,
	PVOID buffer,
	ULONG length,
	BOOLEAN returnSingleEntry,
	BOOLEAN restartScan,
	PULONG context,
	PULONG returnLength);
EXTERN_C LONG NTAPI NtQuerySymbolicLinkObject(
	HANDLE linkHandle,
	PUNICODE_STRING linkTarget,
	PULONG returnedLength);

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#define OBJ_PERMANENT                       0x00000010L
#define OBJ_CASE_INSENSITIVE                0x00000040L

typedef enum _EVENT_TYPE
{
	NotificationEvent,
	SynchronizationEvent
} EVENT_TYPE;

typedef struct _EVENT_BASIC_INFORMATION
{
	EVENT_TYPE EventType;   // The type of the event object (NotificationEvent or SynchronizationEvent).
	LONG EventState;        // The current state of the event object. Nonzero if the event is signaled; zero if not signaled.
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

typedef enum _EVENT_INFORMATION_CLASS
{
	EventBasicInformation
} EVENT_INFORMATION_CLASS;

EXTERN_C LONG NTAPI NtQueryEvent(
	HANDLE eventHandle,
	EVENT_INFORMATION_CLASS eventInformationClass,
	PVOID eventInformation,
	ULONG eventInformationLength,
	PULONG returnLength);

typedef enum _MUTANT_INFORMATION_CLASS
{
	MutantBasicInformation
} MUTANT_INFORMATION_CLASS, *PMUTANT_INFORMATION_CLASS;

typedef struct _MUTANT_BASIC_INFORMATION
{
	LONG CurrentCount;
	BOOLEAN OwnedByCaller;
	BOOLEAN AbandonedState;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

EXTERN_C LONG NTAPI NtQueryMutant(
	HANDLE mutantHandle,
	MUTANT_INFORMATION_CLASS mutantInformationClass,
	PVOID mutantInformation,
	ULONG mutantInformationLength,
	PULONG returnLength);

typedef enum _SEMAPHORE_INFORMATION_CLASS
{
	SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS;

typedef struct _SEMAPHORE_BASIC_INFORMATION
{
	LONG CurrentCount;
	LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

EXTERN_C LONG NTAPI NtQuerySemaphore(
	HANDLE semaphoreHandle,
	SEMAPHORE_INFORMATION_CLASS semaphoreInformationClass,
	PVOID semaphoreInformation,
	ULONG semaphoreInformationLength,
	PULONG returnLength);

typedef enum _SECTION_INFORMATION_CLASS
{
	SectionBasicInformation,
	SectionImageInformation
} SECTION_INFORMATION_CLASS;

typedef struct _SECTIONBASICINFO {
	PVOID BaseAddress;
	ULONG AllocationAttributes;
	LARGE_INTEGER MaximumSize;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

EXTERN_C LONG NTAPI NtQuerySection(
	HANDLE sectionHandle,
	SECTION_INFORMATION_CLASS sectionInformationClass,
	PVOID sectionInformation,
	ULONG sectionInformationLength,
	PULONG returnLength);

typedef enum _TIMER_INFORMATION_CLASS
{
	TimerBasicInformation
} TIMER_INFORMATION_CLASS;

typedef struct _TIMER_BASIC_INFORMATION
{
	LARGE_INTEGER RemainingTime;
	BOOLEAN TimerState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

EXTERN_C LONG NTAPI NtQueryTimer(
	HANDLE timerHandle,
	TIMER_INFORMATION_CLASS timerInformationClass,
	PVOID timerInformation,
	ULONG timerInformationLength,
	PULONG returnLength);

typedef enum _IO_COMPLETION_INFORMATION_CLASS
{
	IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS;

typedef struct _IO_COMPLETION_BASIC_INFORMATION
{
	LONG Depth;
} IO_COMPLETION_BASIC_INFORMATION, *PIO_COMPLETION_BASIC_INFORMATION;

EXTERN_C LONG NTAPI NtQueryIoCompletion(
	HANDLE ioCompletionHandle,
	IO_COMPLETION_INFORMATION_CLASS ioCompletionInformationClass,
	PVOID ioCompletionInformation,
	ULONG ioCompletionInformationLength,
	PULONG returnLength);
EXTERN_C LONG NTAPI NtOpenDirectoryObject(
	PHANDLE directoryHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenSymbolicLinkObject(
	PHANDLE linkHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenEvent(
	PHANDLE eventHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenMutant(
	PHANDLE mutantHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenSemaphore(
	PHANDLE semaphoreHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenSection(
	PHANDLE sectionHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenTimer(
	PHANDLE timerHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenFile(
	PHANDLE fileHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes,
	PIO_STATUS_BLOCK ioStatusBlock,
	ULONG shareAccess,
	ULONG openOptions);
EXTERN_C LONG NTAPI NtOpenSession(
	PHANDLE sessionHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenCpuPartition(
	PHANDLE cpuPartitionHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenJobObject(
	PHANDLE jobHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenIoCompletion(
	PHANDLE ioCompletionHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);
EXTERN_C LONG NTAPI NtOpenPartition(
	PHANDLE partitionHandle,
	ACCESS_MASK desiredAccess,
	POBJECT_ATTRIBUTES objectAttributes);

typedef struct _OBJECT_DIRECTORY_INFORMATION {
	UNICODE_STRING Name;
	UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, * POBJECT_DIRECTORY_INFORMATION;

typedef struct _OBJECT_BASIC_INFORMATION
{
	ULONG Attributes;               // The attributes of the object include whether the object is permanent, can be inherited, and other characteristics.
	ACCESS_MASK GrantedAccess;      // Specifies a mask that represents the granted access when the object was created.
	ULONG HandleCount;              // The number of handles that are currently open for the object.
	ULONG PointerCount;             // The number of references to the object from both handles and other references, such as those from the system.
	ULONG PagedPoolCharge;          // The amount of paged pool memory that the object is using.
	ULONG NonPagedPoolCharge;       // The amount of non-paged pool memory that the object is using.
	ULONG Reserved[3];              // Reserved for future use.
	ULONG NameInfoSize;             // The size of the name information for the object.
	ULONG TypeInfoSize;             // The size of the type information for the object.
	ULONG SecurityDescriptorSize;   // The size of the security descriptor for the object.
	LARGE_INTEGER CreationTime;     // The time when a symbolic link was created. Not supported for other types of objects.
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

enum ZBID
{
	ZBID_DEFAULT = 0,
	ZBID_DESKTOP = 1,
	ZBID_UIACCESS = 2,
	ZBID_IMMERSIVE_IHM = 3,
	ZBID_IMMERSIVE_NOTIFICATION = 4,
	ZBID_IMMERSIVE_APPCHROME = 5,
	ZBID_IMMERSIVE_MOGO = 6,
	ZBID_IMMERSIVE_EDGY = 7,
	ZBID_IMMERSIVE_INACTIVEMOBODY = 8,
	ZBID_IMMERSIVE_INACTIVEDOCK = 9,
	ZBID_IMMERSIVE_ACTIVEMOBODY = 10,
	ZBID_IMMERSIVE_ACTIVEDOCK = 11,
	ZBID_IMMERSIVE_BACKGROUND = 12,
	ZBID_IMMERSIVE_SEARCH = 13,
	ZBID_GENUINE_WINDOWS = 14,
	ZBID_IMMERSIVE_RESTRICTED = 15,
	ZBID_SYSTEM_TOOLS = 16,
	// Windows 10+
	ZBID_LOCK = 17,
	ZBID_ABOVELOCK_UX = 18,
};

enum ACCENT_STATE {
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};

struct ACCENT_POLICY {
	INT AccentState;
	INT AccentFlags;
	INT GradientColor;
	INT AnimationId;
};

typedef struct _WINDOWCOMPOSITIONATTRIBDATA {
	INT Attrib;
	PVOID pvData;
	SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA, *PWINDOWCOMPOSITIONATTRIBDATA;

typedef BOOL(*SetWindowCompositionAttribute_t)(HWND windowHandle, PWINDOWCOMPOSITIONATTRIBDATA data);
