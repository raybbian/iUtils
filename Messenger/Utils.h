#pragma once

#include "pch.h"

#define EXPORT __declspec(dllexport)

#define MSG_DEBUG(...) {WCHAR cad[512]; _snwprintf(cad, 500, __VA_ARGS__); OutputDebugString(cad);}

typedef struct _MESSENGER_DEVICE_CONTEXT {
	HCMNOTIFICATION HandleNotifications;
	HANDLE DeviceHandle;
	CRITICAL_SECTION Lock;
	PTP_WORK pWork;

	PWCHAR SymbolicLink;
	BOOLEAN ShouldUnregister; //set to true from within callback context
	BOOLEAN LockInitialized;
} MESSENGER_DEVICE_CONTEXT, *PMESSENGER_DEVICE_CONTEXT; 

typedef struct _MESSENGER_CONTEXT {
	HCMNOTIFICATION InterfaceNotification;
	MESSENGER_DEVICE_CONTEXT Devices[IU_MAX_NUMBER_OF_DEVICES];
} MESSENGER_CONTEXT, *PMESSENGER_CONTEXT;

LONG GetDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext,
	IN PWCHAR SymbolicLink
);

LONG FindEmptyDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext
);
