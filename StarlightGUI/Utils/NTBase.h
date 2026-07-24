#pragma once

#include "pch.h"

// NT 相关定义
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
EXTERN_C LONG NTAPI NtQueryObject(
	HANDLE Handle,
	ULONG ObjectInformationClass,
	PVOID ObjectInformation,
	ULONG ObjectInformationLength,
	PULONG ReturnLength);

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

typedef LONG(*NtQueryDirectoryObject_t)(
	HANDLE DirectoryHandle,
	PVOID Buffer,
	ULONG Length,
	BOOLEAN ReturnSingleEntry,
	BOOLEAN RestartScan,
	PULONG Context,
	PULONG ReturnLength
	);

typedef LONG(*NtQueryObject_t)(
	HANDLE Handle,
	ULONG ObjectInformationClass,
	PVOID ObjectInformation,
	ULONG ObjectInformationLength,
	PULONG ReturnLength
	);

typedef LONG(*NtQuerySymbolicLinkObject_t)(
	HANDLE LinkHandle,
	PUNICODE_STRING LinkTarget,
	PULONG ReturnedLength
	);

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

typedef LONG(*NtQueryEvent_t)(
	HANDLE EventHandle,
	EVENT_INFORMATION_CLASS EventInformationClass,
	PVOID EventInformation,
	ULONG EventInformationLength,
	PULONG ReturnLength
	);

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

typedef LONG(*NtQueryMutant_t)(
	HANDLE MutantHandle,
	MUTANT_INFORMATION_CLASS MutantInformationClass,
	PVOID MutantInformation,
	ULONG MutantInformationLength,
	PULONG ReturnLength
	);

typedef enum _SEMAPHORE_INFORMATION_CLASS
{
	SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS;

typedef struct _SEMAPHORE_BASIC_INFORMATION
{
	LONG CurrentCount;
	LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

typedef LONG(*NtQuerySemaphore_t)(
	HANDLE SemaphoreHandle,
	SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
	PVOID SemaphoreInformation,
	ULONG SemaphoreInformationLength,
	PULONG ReturnLength
	);

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

typedef LONG(*NtQuerySection_t)(
	HANDLE SectionHandle,
	SECTION_INFORMATION_CLASS SectionInformationClass,
	PVOID SectionInformation,
	ULONG SectionInformationLength,
	PULONG ReturnLength
	);

typedef enum _TIMER_INFORMATION_CLASS
{
	TimerBasicInformation
} TIMER_INFORMATION_CLASS;

typedef struct _TIMER_BASIC_INFORMATION
{
	LARGE_INTEGER RemainingTime;
	BOOLEAN TimerState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

typedef LONG(*NtQueryTimer_t)(
	HANDLE TimerHandle,
	TIMER_INFORMATION_CLASS TimerInformationClass,
	PVOID TimerInformation,
	ULONG TimerInformationLength,
	PULONG ReturnLength
	);

typedef enum _IO_COMPLETION_INFORMATION_CLASS
{
	IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS;

typedef struct _IO_COMPLETION_BASIC_INFORMATION
{
	LONG Depth;
} IO_COMPLETION_BASIC_INFORMATION, *PIO_COMPLETION_BASIC_INFORMATION;

typedef LONG(*NtQueryIoCompletion_t)(
	HANDLE IoCompletionHandle,
	IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
	PVOID IoCompletionInformation,
	ULONG IoCompletionInformationLength,
	PULONG ReturnLength
	);

typedef LONG(*NtOpenDirectoryObject_t)(
	PHANDLE DirectoryHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenSymbolicLinkObject_t)(
	PHANDLE LinkHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenEvent_t)(
	PHANDLE EventHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenMutant_t)(
	PHANDLE MutantHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenSemaphore_t)(
	PHANDLE SemaphoreHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenSection_t)(
	PHANDLE SectionHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenTimer_t)(
	PHANDLE TimerHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenFile_t)(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes,
	PVOID IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions
	);

typedef LONG(*NtOpenSession_t)(
	PHANDLE SessionHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenCpuPartition_t)(
	PHANDLE CpuPartitionHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenJobObject_t)(
	PHANDLE JobHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenIoCompletion_t)(
	PHANDLE IoCompletionHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

typedef LONG(*NtOpenPartition_t)(
	PHANDLE PartitionHandle,
	ACCESS_MASK DesiredAccess,
	PVOID ObjectAttributes
	);

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

typedef BOOL(*SetWindowCompositionAttribute_t)(HWND hWnd, PWINDOWCOMPOSITIONATTRIBDATA data);