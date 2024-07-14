#include "pch.h"
#include "utils.h"

LONG GetDeviceIndex(
	IN PMESSENGER_CONTEXT MSGContext,
	IN PWCHAR SymbolicLink
) {
	//check if device already exists
	for (LONG i = 0; i < IU_MAX_NUMBER_OF_DEVICES; i++) {
		if (wcsnlen(MSGContext->Devices[i].SymbolicLink, MSG_MAX_SYMLINK_LENGTH) == 0)
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
		if (wcsnlen(MSGContext->Devices[i].SymbolicLink, MSG_MAX_SYMLINK_LENGTH) == 0) {
			return i;
		}
	}
	return -1;
}
