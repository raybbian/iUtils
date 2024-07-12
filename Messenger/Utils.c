#include "pch.h"
#include "utils.h"

LONG GetDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext,
	IN PWCHAR SymbolicLink
) {
	//check if device already exists
	for (LONG i = 0; i < IU_MAX_NUMBER_OF_DEVICES; i++) {
		if (MSGContext->Devices[i].SymbolicLink == NULL)
			continue;
		if (wcscmp(SymbolicLink, MSGContext->Devices[i].SymbolicLink) == 0) 
			return i;
	}
	return -1;
}

LONG FindEmptyDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext
) {
	//find spot for device
	for (LONG i = 0; i < IU_MAX_NUMBER_OF_DEVICES; i++) {
		if (MSGContext->Devices[i].SymbolicLink == NULL) {
			return i;
		}
	}
	return -1;
}
