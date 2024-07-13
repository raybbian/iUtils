#pragma once
#include "pch.h"
#include "utils.h"

CONFIGRET AddDevice(
	PMESSENGER_CONTEXT MSGContext,
	PWCHAR SymbolicLink,
	BOOLEAN CallCallback
);

VOID RemoveDevice( //CALLED OUTSIDE OF USB HANDLE CALLBACK 
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext,
	BOOLEAN CallCallback
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
	INOUT PMESSENGER_CONTEXT MSGContext
);

VOID UnregisterDeviceFromHandleCallbackContext(
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext
);

VOID CALLBACK UnregisterDeviceHandleCallbackAsync(
	INOUT PTP_CALLBACK_INSTANCE Instance,
	INOUT PVOID Context,
	INOUT PTP_WORK pWork
);

BOOLEAN SendDeviceIoctl(
	IN PMESSENGER_CONTEXT MSGContext,
	IN LONG DeviceInd,
	IN DWORD IoctlCode,
	INOPT PVOID InputBuffer,
	IN ULONG InputBufferLen,
	INOPT PVOID OutputBuffer,
	IN ULONG OutputBufferLen
);
