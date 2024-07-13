#include "pch.h"
#include "api.h"
#include "utils.h"
#include "messenger.h"

PMESSENGER_CONTEXT WINAPI MSGInit() {
	CONFIGRET ret = CR_SUCCESS;
	PMESSENGER_CONTEXT MSGContext = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MESSENGER_CONTEXT));
	if (MSGContext == NULL) {
		MSG_DEBUG(L"[IUMSG] Failed to allocate memory for MSG context\n");
		goto Error;
	}

	ret = RegisterInterfaceNotifications(MSGContext);
	if (ret != CR_SUCCESS) {
		MSG_DEBUG(L"[IUMSG] Failed to register for interface notifications\n");
		goto Error;
	}

	//add current devices
	ret = RetrieveExistingDevices(MSGContext);
	if (ret != CR_SUCCESS) {
		MSG_DEBUG(L"[IUMSG] Failed to retrieve and add existing devices\n");
		goto Error;
	}

	MSG_DEBUG(L"[IUMSG] MSGInit success\n");
	goto Cleanup;
Error:
	if (MSGContext) {
		if (MSGContext->InterfaceNotification)
			CM_Unregister_Notification(MSGContext->InterfaceNotification);
		HeapFree(GetProcessHeap(), 0, MSGContext);
		MSGContext = NULL;
	}
Cleanup:
	return MSGContext;
}

LONG WINAPI MSGGetDevices(
	IN PMESSENGER_CONTEXT MSGContext
) {
	if (MSGContext == NULL) {
		MSG_DEBUG(L"[IUMSG] Bad MSG Context\n");
		return -MSG_FAILURE;
	}
	LONG deviceFlags = 0;
	for (LONG i = 0; i < IU_MAX_NUMBER_OF_DEVICES; i++) {
		if (MSGContext->Devices[i].SymbolicLink != NULL) {
			deviceFlags |= (1 << i);
		}
	}
	return deviceFlags;
}

LONG WINAPI MSGGetAppleMode(
	IN PMESSENGER_CONTEXT MSGContext,
	IN LONG DeviceInd
) {
	MSG_DEBUG(L"[IUMSG] GetAppleMode %d\n", DeviceInd);

	UCHAR appleMode = 0;
	BOOLEAN success = SendDeviceIoctl(MSGContext, DeviceInd, IU_IOCTL_GET_MODE, NULL, 0, &appleMode, sizeof(appleMode));
	if (!success) return -MSG_FAILURE;
	return appleMode;
}

LONG WINAPI MSGGetConfiguration(
	IN PMESSENGER_CONTEXT MSGContext,
	IN LONG DeviceInd
) {
	MSG_DEBUG(L"[IUMSG] GetConfiguration %d\n", DeviceInd);

	UCHAR configuration = 0;
	BOOLEAN success = SendDeviceIoctl(MSGContext, DeviceInd, IU_IOCTL_GET_CONFIGURATION, NULL, 0, &configuration, sizeof(configuration));
	if (!success) return -MSG_FAILURE;
	return configuration;
}

MSG_STATUS WINAPI MSGSetConfiguration(
	IN PMESSENGER_CONTEXT MSGContext,
	IN LONG DeviceInd,
	IN LONG Configuration
) {
	MSG_DEBUG(L"[IUMSG] Set Configuration for %d to %d\n", DeviceInd, Configuration);

	UCHAR tmpConfiguration = (UCHAR)Configuration;
	BOOLEAN success = SendDeviceIoctl(MSGContext, DeviceInd, IU_IOCTL_SET_CONFIGURATION, &tmpConfiguration, sizeof(tmpConfiguration), NULL, 0);
	if (!success) return -MSG_FAILURE;
	return MSG_SUCCESS;
}

VOID WINAPI MSGClose(IN PMESSENGER_CONTEXT MSGContext) {
	if (!MSGContext) {
		MSG_DEBUG(L"[IUMSG] Called MSGClose with NULL MSGContext\n");
		return;
	}

	for (LONG i = 0; i < IU_MAX_NUMBER_OF_DEVICES; i++) {
		RemoveDevice(&MSGContext->Devices[i]);
	}
	//cleanup other context attributes
	CONFIGRET ret = CM_Unregister_Notification(MSGContext->InterfaceNotification);
	if (ret != CR_SUCCESS) {
		MSG_DEBUG(L"[IUMSG] Could not unregister interface notifications, closing anyway\n");
	}
	HeapFree(GetProcessHeap(), 0, MSGContext);
	MSG_DEBUG(L"[IUMSG] Closed messenger\n");
}
