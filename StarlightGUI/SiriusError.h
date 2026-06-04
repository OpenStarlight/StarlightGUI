#pragma once

typedef unsigned long SISTATUS;
typedef SISTATUS *PSISTATUS;

#undef SUCCESS
#undef ERROR
#define SUCCESS(code) ((SISTATUS)code < 0x10000)
#define SUCCESS_STRICT(code) ((SISTATUS)code == 0x00000)
#define ERROR(code) ((SISTATUS)code >= 0x10000)

// Success codes
// Success: 0x00000~0x0FFFF, strict success: 0x00000
// 
// Operation completed successfully.
#define SI_SUCCESS				0x00000

// Operation completed but no further information is provided.
#define SI_NO_INFORMATION		0x00001

// Operation completed but some errors may have occured.
#define SI_DONE_WITH_ERROR		0x00002

// Operation completed but the return value is not complete.
// eg. Buffer too small to contain all things. / Some memory is corrupted.
#define SI_PARTIAL_SUCCESS		0x00003

// Operation completed with custom information. Call Error::GetErrorDetail() for further information.
#define SI_SUCCESS_CUSTOM		0x00004

// Operation has been done before and this call is unnecessary.
#define SI_ALREADY_DONE			0x00005

// Error codes
// Error: 0x10000+
//
// Operation completed unsuccessfully.
#define SI_ERROR				0x10000

// Operation completed unsuccessfully with custom information. Call Error::GetErrorDetail() for further information.
#define SI_ERROR_CUSTOM			0x10001

// Parameter(s) invalid.
#define SI_INVALID_PARAMETER	0x10002

// Memory violation during operation.
#define SI_MEMORY_VIOLATION		0x10003

// Access violation during operation.
#define SI_ACCESS_VIOLATION		0x10004

// Specified target does not exist.
#define SI_NOT_FOUND			0x10005

// The operation is not available on current environment.
// eg. Pattern scan failed so we don't have the symbol. / MmGetSystemRoutineAddress() failed.
#define SI_NOT_AVAILABLE		0x10006

// The operation requires virtualization enabled.
#define SI_NOT_VIRTUALIZED		0x10007

// A pool has failed to be allocated during the operation.
#define SI_ALLOCATION_FAILED	0x10008

// A pending operation has been issued and cannot receive more.
#define SI_BUSY					0x10009

// Data format does not match.
#define SI_BAD_FORMAT			0x1000A

// This should not happen. Cannot continue.
#define SI_BAD_RESULT			0x1000B

// Not implemented yet.
#define SI_NOT_IMPLEMENTED		0x1000C