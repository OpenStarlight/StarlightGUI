#pragma once

#include "Sirius.h"
#include "SiriusIO.h"

//
// Virtualization
//
enum class VirtualizationCollection : COLLECTION_ENUM {
	StartMetaverse,
	StopMetaverse,
	CheckStatus,
	CheckSupport
};

enum class MetaverseMode : TYPE_ENUM {
	Performance,	// Fast mode, skips memory protection, etc. Keep high performance first. For experiencing virtualization features only.
	Normal,			// Common mode, runs with no additional features enabled. For daily use.
	Stealth			// Hidden mode, adds extra concealment methods to hide self. For countering detection.
};

typedef struct _METAVERSE_CONFIGURATION {
	 MetaverseMode Mode;
} METAVERSE_CONFIGURATION, * PMETAVERSE_CONFIGURATION;

//
// Monitor
//
enum class MonitorCollection : COLLECTION_ENUM {
	SetMonitorState,
	SetMonitorType,
	SetLogDetailed,
	GetLog
};

enum class MonitorType : TYPE_ENUM {
	Process,
	Thread,
	Image,
	Registry,
	File
};
