#pragma once

#include "pch.h"

#define EXPORT __declspec(dllexport)

#define MSG_DEBUG(...) {WCHAR cad[512]; swprintf_s(cad, 511, __VA_ARGS__); OutputDebugString(cad);}

struct _MESSENGER_CONTEXT;

typedef struct _MESSENGER_DEVICE_CONTEXT {
	HCMNOTIFICATION HandleNotifications;
	HANDLE DeviceHandle;
	CRITICAL_SECTION Lock;
	PTP_WORK pWork;

	PWCHAR SymbolicLink;
	struct _MESSENGER_CONTEXT* ParentContext;
	BOOLEAN ShouldUnregister; //set to true from within callback context
	BOOLEAN LockInitialized;
	UCHAR DeviceInd;
} MESSENGER_DEVICE_CONTEXT, *PMESSENGER_DEVICE_CONTEXT; 

typedef struct _MESSENGER_CONTEXT {
	HCMNOTIFICATION InterfaceNotification;
	VOID(*DeviceAddCallback)(LONG);
	VOID(*DeviceRemoveCallback)(LONG);
	MESSENGER_DEVICE_CONTEXT Devices[IU_MAX_NUMBER_OF_DEVICES];
} MESSENGER_CONTEXT, *PMESSENGER_CONTEXT;

LONG GetDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext,
	IN PWCHAR SymbolicLink
);

LONG FindEmptyDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext
);
