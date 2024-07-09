#pragma once

#include "pch.h"

typedef struct _MESSENGER_DEVICE_CONTEXT {
	HCMNOTIFICATION HandleNotifications;
	HANDLE DeviceHandle;
	CRITICAL_SECTION Lock;
	PTP_WORK pWork;

	PWCHAR SymbolicLink;
	BOOL ShouldUnregister; //set to true from within callback context
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
	IN PMESSENGER_CONTEXT MSGContext,
	IN PWCHAR SymbolicLink
);

VOID UnregisterDeviceFromWithinHandleCallback(
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext
);

VOID CALLBACK UnregisterCallback(
	INOUT PTP_CALLBACK_INSTANCE Instance,
	INOUT PVOID Context,
	INOUT PTP_WORK pWork
);
