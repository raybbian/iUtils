#include "pch.h"
#include "utils.h"

LONG GetDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext,
	IN PWCHAR SymbolicLink
) {
	//check if device already exists
	for (LONG i = 0; i < IU_MAX_NUMBER_OF_DEVICES; i++) {
		if (wcscmp(SymbolicLink, MSGContext->Devices[i].SymbolicLink) == 0) {
			return i;
		}
	}
	return -1;
}

LONG FindEmptyDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext,
	IN PWCHAR SymbolicLink
) {
	//find spot for device
	for (LONG i = 0; i < IU_MAX_NUMBER_OF_DEVICES; i++) {
		if (MSGContext->Devices[i].SymbolicLink == NULL) {
			return i;
		}
	}
	return -1;
}

VOID UnregisterDeviceFromWithinHandleCallback(
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext
) {
	EnterCriticalSection(&MSGDeviceContext->Lock);
	if (!MSGDeviceContext->ShouldUnregister) {
		MSGDeviceContext->ShouldUnregister = TRUE;
		SubmitThreadpoolWork(MSGDeviceContext->pWork);
	}
	LeaveCriticalSection(&MSGDeviceContext->Lock);
}

VOID CALLBACK UnregisterCallback(
	INOUT PTP_CALLBACK_INSTANCE Instance,
	INOUT PVOID Context,
	INOUT PTP_WORK pWork
) {
	CONFIGRET ret;
	PMESSENGER_DEVICE_CONTEXT MSGDeviceContext = (PMESSENGER_DEVICE_CONTEXT)Context;

	EnterCriticalSection(&MSGDeviceContext->Lock);
	if (MSGDeviceContext->ShouldUnregister) {
		ret = CM_Unregister_Notification(MSGDeviceContext->HandleNotifications);
		if (ret != CR_SUCCESS)
			goto Cleanup; //wtf
		MSGDeviceContext->HandleNotifications = NULL;
		MSGDeviceContext->ShouldUnregister = FALSE;
	}
Cleanup:
	LeaveCriticalSection(&MSGDeviceContext->Lock);
}