#pragma once
#include "pch.h"
#include "utils.h"

CONFIGRET AddDevice(
	PMESSENGER_CONTEXT MSGContext,
	PWCHAR SymbolicLink
);

VOID RemoveDevice( //CALLED OUTSIDE OF USB HANDLE CALLBACK 
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext
);

DWORD WINAPI USBHandleCallback(
	HCMNOTIFICATION hNotify,
	PVOID Context,
	CM_NOTIFY_ACTION Action,
	PCM_NOTIFY_EVENT_DATA EventData,
	DWORD EventDataSize
);

DWORD WINAPI USBInterfaceCallback( // when devices added or removed
	HCMNOTIFICATION hNotify,
	PVOID Context,
	CM_NOTIFY_ACTION Action,
	PCM_NOTIFY_EVENT_DATA EventData,
	DWORD EventDataSize
);

CONFIGRET RegisterInterfaceNotifications(
	IN PMESSENGER_CONTEXT MSGContext
);

CONFIGRET RetrieveExistingDevices(
	OUT PVOID Buffer,
	IN ULONG BufferLen,
	OUT PULONG Received
);
