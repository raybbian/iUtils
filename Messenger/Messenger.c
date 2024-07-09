#include "pch.h"
#include "messenger.h"

CONFIGRET AddDevice(
	PMESSENGER_CONTEXT MSGContext,
	PWCHAR SymbolicLink
) {
	CONFIGRET ret = CR_SUCCESS;
	if (wcslen(SymbolicLink) > MAX_PATH)
		return CR_BUFFER_SMALL;

	if (GetDeviceIndex(MSGContext, SymbolicLink) != -1) {
		//device already exists
		return CR_ALREADY_SUCH_DEVNODE;
	}

	LONG ind = FindEmptyDeviceIndex(MSGContext, SymbolicLink);
	if (ind == -1)
		return CR_BUFFER_SMALL;
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext = &MSGContext->Devices[ind];

	// fill Symbolic Link
	MSGDeviceContext->SymbolicLink = HeapAlloc(GetProcessHeap(), 0, wcslen(SymbolicLink));
	if (!MSGDeviceContext->SymbolicLink) {
		RemoveDevice(MSGDeviceContext);
		return CR_OUT_OF_MEMORY;
	}
	wcscpy(MSGDeviceContext->SymbolicLink, SymbolicLink);

	//init lock
	InitializeCriticalSection(&MSGDeviceContext->Lock);

	//init threadpool for unreg
	MSGDeviceContext->pWork = CreateThreadpoolWork(UnregisterCallback, MSGDeviceContext, NULL);
	if (!MSGDeviceContext->pWork) {
		RemoveDevice(MSGDeviceContext);
		return CR_FAILURE;
	}

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
		RemoveDevice(MSGDeviceContext);
		return CR_DEVICE_NOT_THERE;
	}

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
		RemoveDevice(MSGDeviceContext);
		return ret;
	}

	return ret;
}

VOID RemoveDevice( //CALLED OUTSIDE OF USB HANDLE CALLBACK 
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext
) {
	// close device handle
	if (MSGDeviceContext->DeviceHandle)
		CloseHandle(MSGDeviceContext->DeviceHandle);

	// unregister for notifs, if was marked by callback to unregister, let it do that
	if (MSGDeviceContext->HandleNotifications) {
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
	if (MSGDeviceContext->pWork)
		CloseThreadpoolWork(MSGDeviceContext->pWork);

	// delete critical section
	DeleteCriticalSection(&MSGDeviceContext->Lock);

	// free symlink string
	if (MSGDeviceContext->SymbolicLink)
		HeapFree(GetProcessHeap(), 0, MSGDeviceContext->SymbolicLink);

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
		if (MSGDeviceContext->DeviceHandle)
			CloseHandle(MSGDeviceContext->DeviceHandle);
		break;
	case CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED:
		//unregister and wait
		if (MSGDeviceContext->HandleNotifications) {
			UnregisterDeviceFromWithinHandleCallback(MSGDeviceContext);
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
		if (MSGDeviceContext->DeviceHandle == INVALID_HANDLE_VALUE)
			goto Cleanup;

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
			CloseHandle(MSGDeviceContext->DeviceHandle);
			goto Cleanup;
		}
		break;
	case CM_NOTIFY_ACTION_DEVICEREMOVEPENDING:
	case CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE:
		UnregisterDeviceFromWithinHandleCallback(MSGDeviceContext);
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
		AddDevice(MSGContext, symbolicLink);
		break;
	case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
		if (deviceInd == -1) goto Cleanup;
		RemoveDevice(&MSGContext->Devices[deviceInd]);
		break;
	}
Cleanup:
	return ERROR_SUCCESS;
}

CONFIGRET RegisterInterfaceNotifications(
	IN PMESSENGER_CONTEXT MSGContext
) {
	CM_NOTIFY_FILTER filter = { 0 };
	filter.cbSize = sizeof(filter);
	filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
	filter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_Keystone;
	return CM_Register_Notification(&filter, MSGContext, USBInterfaceCallback, &MSGContext->InterfaceNotification);
}

CONFIGRET RetrieveExistingDevices(
	OUT PVOID Buffer,
	IN ULONG BufferLen,
	OUT PULONG ReqLen
) {
	CONFIGRET ret;

	ret = CM_Get_Device_Interface_List_Size(
		ReqLen,
		&GUID_DEVINTERFACE_Keystone,
		NULL,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT
	);
	if (ret != CR_SUCCESS)
		return ret;

	if (*ReqLen > BufferLen) 
		return CR_BUFFER_SMALL;

	return CM_Get_Device_Interface_List(
		&GUID_DEVINTERFACE_Keystone,
		NULL,
		Buffer,
		BufferLen,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT
	);
}
