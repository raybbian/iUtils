#include "pch.h"
#include "api.h"
#include "utils.h"
#include "messenger.h"

PVOID MSGInit() {
	CONFIGRET ret = CR_SUCCESS;
	PWCHAR buf = NULL;
	PMESSENGER_CONTEXT MSGContext = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MESSENGER_CONTEXT));
	if (MSGContext == NULL) {
		goto Error;
	}

	ret = RegisterInterfaceNotifications(MSGContext);
	if (ret != CR_SUCCESS)
		goto Error;

	//get the names of current devices
	ULONG reqLen = 0;
	WCHAR tmpBuf[1];
	ret = RetrieveExistingDevices(tmpBuf, sizeof(tmpBuf), &reqLen);
	if (ret != CR_SUCCESS && ret != CR_BUFFER_SMALL)
		goto Error;
	buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, reqLen);
	if (!buf)
		goto Error;
	ret = RetrieveExistingDevices(buf, reqLen, &reqLen);
	if (ret != CR_SUCCESS)
		goto Error;

	PWCHAR curChar = buf;
	while (curChar != '\0') {
		AddDevice(MSGContext, curChar);
		curChar += wcslen(curChar) + 1;
	}

	goto Cleanup;
Error:
	if (MSGContext) {
		if (MSGContext->InterfaceNotification)
			CM_Unregister_Notification(MSGContext->InterfaceNotification);
		HeapFree(GetProcessHeap(), 0, MSGContext);
		MSGContext = NULL;
	}
Cleanup:
	if (buf) 
		HeapFree(GetProcessHeap(), 0, buf);
	return MSGContext;
}

VOID MSGClose(PMESSENGER_CONTEXT MSGContext) {
	if (!MSGContext) return; 

	for (LONG i = 0; i < IU_MAX_NUMBER_OF_DEVICES; i++) {
		RemoveDevice(&MSGContext->Devices[i]);
	}
	//cleanup other context attributes
	CM_Unregister_Notification(MSGContext->InterfaceNotification);
	HeapFree(GetProcessHeap(), 0, MSGContext);
}
