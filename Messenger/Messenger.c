#include "pch.h"
#include "messenger.h"
#include "utils.h"

CONFIGRET AddDevice(
	PMESSENGER_CONTEXT MSGContext,
	PWCHAR SymbolicLink,
	BOOLEAN CallCallback
) {
	CONFIGRET ret = CR_SUCCESS;

	MSG_DEBUG(L"[IUMSG] Adding Device %ws\n", SymbolicLink);

	if (GetDeviceIndex(MSGContext, SymbolicLink) != -1) {
		MSG_DEBUG(L"[IUMSG] Tried to add device that already exists\n");
		return CR_ALREADY_SUCH_DEVNODE;
	}

	LONG ind = FindEmptyDeviceIndex(MSGContext);
	if (ind == -1) {
		MSG_DEBUG(L"[IUMSG] There are too many device being used!\n");
		return CR_BUFFER_SMALL;
	}

	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext = &MSGContext->Devices[ind];
	SIZE_T linkLen = wcslen(SymbolicLink);
	MSGDeviceContext->DeviceInd = (UCHAR)ind;
	MSGDeviceContext->ParentContext = MSGContext;
	// fill Symbolic Link
	MSGDeviceContext->SymbolicLink = HeapAlloc(GetProcessHeap(), 0, (linkLen + 1) * sizeof(WCHAR));
	if (!MSGDeviceContext->SymbolicLink) {
		MSG_DEBUG(L"[IUMSG] Could not allocate memory for symbolic link string\n");
		RemoveDevice(MSGDeviceContext, FALSE);
		return CR_OUT_OF_MEMORY;
	}
	wcscpy_s(MSGDeviceContext->SymbolicLink, linkLen, SymbolicLink);

	//init lock
	InitializeCriticalSection(&MSGDeviceContext->Lock);
	MSGDeviceContext->LockInitialized = TRUE;

	//init threadpool for unreg
	MSGDeviceContext->pWork = CreateThreadpoolWork(UnregisterDeviceHandleCallbackAsync, MSGDeviceContext, NULL);
	if (!MSGDeviceContext->pWork) {
		MSG_DEBUG(L"[IUMSG] Could not initialize threadpool for async unregister\n");
		RemoveDevice(MSGDeviceContext, FALSE);
		return CR_FAILURE;
	}

	MSG_DEBUG(L"[IUMSG] Opening device handle for device %ws\n", SymbolicLink);

	// Open device handle
	MSGDeviceContext->DeviceHandle = CreateFile(
		SymbolicLink,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (MSGDeviceContext->DeviceHandle == INVALID_HANDLE_VALUE) {
		MSG_DEBUG(L"[IUMSG] Could not open device handle\n");
		RemoveDevice(MSGDeviceContext, FALSE);
		return CR_DEVICE_NOT_THERE;
	}

	MSG_DEBUG(L"[IUMSG] Setting up handle notifications for device %ws\n", SymbolicLink);

	// set up handle notifications
	CM_NOTIFY_FILTER filter = { 0 };
	filter.cbSize = sizeof(filter);
	filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE;
	filter.u.DeviceHandle.hTarget = MSGDeviceContext->DeviceHandle;
	ret = CM_Register_Notification(
		&filter,
		(PVOID)&MSGContext->Devices[ind],
		USBHandleCallback,
		&MSGContext->Devices[ind].HandleNotifications
	);
	if (ret != CR_SUCCESS) {
		MSG_DEBUG(L"[IUMSG] Could not register device for handle notifications\n");
		RemoveDevice(MSGDeviceContext, FALSE);
		return ret;
	}

	MSG_DEBUG(L"[IUMSG] Successfully added device %ws\n", SymbolicLink);
	if (CallCallback && MSGContext->DeviceAddCallback != NULL) {
		MSG_DEBUG(L"[IUMSG] Calling add device callback");
		MSGContext->DeviceAddCallback(MSGDeviceContext->DeviceInd);
	}

	return ret;
}

VOID RemoveDevice( //CALLED OUTSIDE OF USB HANDLE CALLBACK 
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext,
	BOOLEAN CallCallback
) {

	PMESSENGER_CONTEXT MSGContext = MSGDeviceContext->ParentContext;
	if (CallCallback && MSGContext->DeviceRemoveCallback != NULL) {
		MSG_DEBUG(L"[IUMSG] Calling remove device callback");
		MSGContext->DeviceRemoveCallback(MSGDeviceContext->DeviceInd);
	}

	// close device handle
	if (MSGDeviceContext->DeviceHandle) {
		MSG_DEBUG(L"[IUMSG] Closing device handle");
		CloseHandle(MSGDeviceContext->DeviceHandle);
	}

	// unregister for notifs, if was marked by callback to unregister, let it do that
	if (MSGDeviceContext->HandleNotifications) {
		MSG_DEBUG(L"[IUMSG] Unregistering notifications\n");
		BOOL callbackUnregister = FALSE;
		EnterCriticalSection(&MSGDeviceContext->Lock);
		if (MSGDeviceContext->ShouldUnregister)
			callbackUnregister = TRUE;
		LeaveCriticalSection(&MSGDeviceContext->Lock);

		if (!callbackUnregister)
			CM_Unregister_Notification(MSGDeviceContext->HandleNotifications);
		else  //pWork init before handle notifs, can't be null
			WaitForThreadpoolWorkCallbacks(MSGDeviceContext->pWork, FALSE);
	}

	// close work thread pool
	if (MSGDeviceContext->pWork) {
		MSG_DEBUG(L"[IUMSG] Closing thread pool\n");
		CloseThreadpoolWork(MSGDeviceContext->pWork);
	}

	// delete critical section
	if (MSGDeviceContext->LockInitialized) {
		DeleteCriticalSection(&MSGDeviceContext->Lock);
		MSGDeviceContext->LockInitialized = FALSE;
	}

	// free symlink string
	if (MSGDeviceContext->SymbolicLink) {
		MSG_DEBUG(L"[IUMSG] Freeing device symlink\n");
		HeapFree(GetProcessHeap(), 0, MSGDeviceContext->SymbolicLink);
	}

	// reset ShouldRegister
	memset(MSGDeviceContext, 0, sizeof(MESSENGER_DEVICE_CONTEXT));
}

DWORD WINAPI USBHandleCallback(
	HCMNOTIFICATION hNotify,
	PVOID Context,
	CM_NOTIFY_ACTION Action,
	PCM_NOTIFY_EVENT_DATA EventData,
	DWORD EventDataSize
) {
	CONFIGRET ret = ERROR_SUCCESS;
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext = (PMESSENGER_DEVICE_CONTEXT)Context;
	MSGDeviceContext->HandleNotifications = hNotify;

	switch (Action) {
	case CM_NOTIFY_ACTION_DEVICEQUERYREMOVE:
		MSG_DEBUG(L"[IUMSG] Handling device query remove\n");
		if (MSGDeviceContext->DeviceHandle)
			CloseHandle(MSGDeviceContext->DeviceHandle);
		break;
	case CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED:
		//unregister and wait
		MSG_DEBUG(L"[IUMSG] Handling device query remove failed\n");
		if (MSGDeviceContext->HandleNotifications) {
			UnregisterDeviceFromHandleCallbackContext(MSGDeviceContext);
			WaitForThreadpoolWorkCallbacks(MSGDeviceContext->pWork, FALSE);
		}

		// close old handle
		if (MSGDeviceContext->DeviceHandle)
			CloseHandle(MSGDeviceContext->DeviceHandle);

		// Open device handle again
		MSGDeviceContext->DeviceHandle = CreateFile(
			MSGDeviceContext->SymbolicLink,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if (MSGDeviceContext->DeviceHandle == INVALID_HANDLE_VALUE) {
			MSG_DEBUG(L"[IUMSG] Failed to reopen device handle\n");
			goto Cleanup;
		}

		// re-register notifications
		CM_NOTIFY_FILTER filter = { 0 };
		filter.cbSize = sizeof(filter);
		filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE;
		filter.u.DeviceHandle.hTarget = MSGDeviceContext->DeviceHandle;
		ret = CM_Register_Notification(
			&filter,
			(PVOID)MSGDeviceContext,
			USBHandleCallback,
			&MSGDeviceContext->HandleNotifications
		);
		if (ret != CR_SUCCESS) {
			MSG_DEBUG(L"[IUMSG] Failed to reregister for device handle notifications\n");
			CloseHandle(MSGDeviceContext->DeviceHandle);
			goto Cleanup;
		}
		break;
	case CM_NOTIFY_ACTION_DEVICEREMOVEPENDING:
	case CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE:
		MSG_DEBUG(L"[IUMSG] Handling device remove action\n");
		UnregisterDeviceFromHandleCallbackContext(MSGDeviceContext);
		if (MSGDeviceContext->DeviceHandle)
			CloseHandle(MSGDeviceContext->DeviceHandle);
		break;
	}
Cleanup:
	return ret;
}

DWORD WINAPI USBInterfaceCallback( // when devices added or removed
	HCMNOTIFICATION hNotify,
	PVOID Context,
	CM_NOTIFY_ACTION Action,
	PCM_NOTIFY_EVENT_DATA EventData,
	DWORD EventDataSize
) {
	PMESSENGER_CONTEXT MSGContext = (PMESSENGER_CONTEXT)Context;

	PWCHAR symbolicLink = EventData->u.DeviceInterface.SymbolicLink;
	LONG deviceInd = GetDeviceIndex(MSGContext, symbolicLink);

	switch (Action) {
	case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
		MSG_DEBUG(L"[IUMSG] Found new device! Adding...\n");
		AddDevice(MSGContext, symbolicLink, TRUE);
		break;
	case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
		MSG_DEBUG(L"[IUMSG] Removing device...\n");
		if (deviceInd == -1) {
			MSG_DEBUG(L"[IUMSG] Device to be removed not found!\n");
			goto Cleanup;
		}
		RemoveDevice(&MSGContext->Devices[deviceInd], TRUE);
		break;
	}
Cleanup:
	return ERROR_SUCCESS;
}

CONFIGRET RegisterInterfaceNotifications(
	IN PMESSENGER_CONTEXT MSGContext
) {
	MSG_DEBUG(L"[IUMSG] Registering for interface notifications...\n");
	CM_NOTIFY_FILTER filter = { 0 };
	filter.cbSize = sizeof(filter);
	filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
	filter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_Keystone;
	return CM_Register_Notification(&filter, MSGContext, USBInterfaceCallback, &MSGContext->InterfaceNotification);
}

CONFIGRET RetrieveExistingDevices(
	INOUT PMESSENGER_CONTEXT MSGContext
) {
	MSG_DEBUG(L"[IUMSG] Retrieving existing devices\n");
	CONFIGRET ret;

	LONG reqLen = 0;
	ret = CM_Get_Device_Interface_List_Size(
		&reqLen,
		&GUID_DEVINTERFACE_Keystone,
		NULL,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT
	);
	if (ret != CR_SUCCESS) {
		MSG_DEBUG(L"[IUMSG] Failed to get interface list size\n");
		return ret;
	}
	MSG_DEBUG(L"[IUMSG] Interface list size is %d\n", reqLen);

	PWCHAR buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (reqLen + 1));
	if (!buf) {
		MSG_DEBUG(L"[IUMSG] Failed to allocate memory for interface list\n");
		ret = CR_OUT_OF_MEMORY;
		goto Cleanup;
	}

	ret = CM_Get_Device_Interface_List(
		&GUID_DEVINTERFACE_Keystone,
		NULL,
		buf,
		reqLen,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT
	);
	if (ret != CR_SUCCESS) {
		MSG_DEBUG(L"[IUMSG] Failed to get full interface list\n");
		goto Cleanup;
	}

	PWCHAR curChar = buf;
	while (*curChar != '\0' && reqLen > 0) {
		SIZE_T strLen = wcsnlen(curChar, reqLen);
		if (strLen == reqLen) {
			MSG_DEBUG(L"[IUMSG] No more devices!\n");
			break;
		}
		MSG_DEBUG(L"[IUMSG] Found device %ws\n", curChar);
		AddDevice(MSGContext, curChar, FALSE);
		curChar += strLen + 1;
		reqLen -= strLen + 1;
	}

Cleanup:
	if (buf)
		HeapFree(GetProcessHeap(), 0, buf);
	return ret;
}

VOID UnregisterDeviceFromHandleCallbackContext(
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext
) {
	MSG_DEBUG(L"[IUMSG] Unregistering device from within callback context\n");
	EnterCriticalSection(&MSGDeviceContext->Lock);
	if (!MSGDeviceContext->ShouldUnregister) {
		MSGDeviceContext->ShouldUnregister = TRUE;
		SubmitThreadpoolWork(MSGDeviceContext->pWork);
	}
	LeaveCriticalSection(&MSGDeviceContext->Lock);
}

VOID CALLBACK UnregisterDeviceHandleCallbackAsync(
	INOUT PTP_CALLBACK_INSTANCE Instance,
	INOUT PVOID Context,
	INOUT PTP_WORK pWork
) {
	CONFIGRET ret;
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext = (PMESSENGER_DEVICE_CONTEXT)Context;

	MSG_DEBUG(L"[IUMSG] Unregistering device: inside async function\n");

	EnterCriticalSection(&MSGDeviceContext->Lock);
	if (MSGDeviceContext->ShouldUnregister) {
		ret = CM_Unregister_Notification(MSGDeviceContext->HandleNotifications);
		if (ret != CR_SUCCESS) {
			MSG_DEBUG(L"[IUMSG] Somehow failed to unregister device in async function\n");
			goto Cleanup; //wtf
		}
		MSGDeviceContext->HandleNotifications = NULL;
		MSGDeviceContext->ShouldUnregister = FALSE;
	}
Cleanup:
	LeaveCriticalSection(&MSGDeviceContext->Lock);
}

BOOLEAN SendDeviceIoctl(
	IN PMESSENGER_CONTEXT MSGContext,
	IN LONG DeviceInd,
	IN DWORD IoctlCode,
	INOPT PVOID InputBuffer,
	IN ULONG InputBufferLen,
	INOPT PVOID OutputBuffer,
	IN ULONG OutputBufferLen
) {
	if (MSGContext == NULL) {
		MSG_DEBUG(L"[IUMSG] Bad MSG Context\n");
		return FALSE;
	}

	if (MSGContext->Devices[DeviceInd].SymbolicLink == NULL) {
		MSG_DEBUG(L"[IUMSG] No such device\n");
		return FALSE;
	}

	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext = &MSGContext->Devices[DeviceInd];
	if (!MSGDeviceContext->DeviceHandle) {
		MSG_DEBUG(L"[IUMSG] Device does not have open handle\n");
		return FALSE;
	}

	EnterCriticalSection(&MSGDeviceContext->Lock);

	if (MSGDeviceContext->ShouldUnregister) {
		MSG_DEBUG(L"[IUMSG] Device is pending removal\n");
		LeaveCriticalSection(&MSGDeviceContext->Lock);
		return FALSE;
	}

	ULONG ret;
	BOOL sent = DeviceIoControl(
		MSGDeviceContext->DeviceHandle,
		IoctlCode,
		InputBuffer,
		InputBufferLen,
		OutputBuffer,
		OutputBufferLen,
		&ret,
		0
	);

	LeaveCriticalSection(&MSGDeviceContext->Lock);

	if (!sent) {
		MSG_DEBUG(L"[IUMSG] Failed to send device IOCTL\n");
		return FALSE;
	}
	return TRUE;
}
