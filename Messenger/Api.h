#pragma once
#include "pch.h"
#include "messenger.h"

typedef enum _MSG_STATUS {
	MSG_SUCCESS = 0,
	MSG_FAILURE = 1,
} MSG_STATUS;

EXPORT PMESSENGER_CONTEXT WINAPI MSGInit();

EXPORT LONG WINAPI MSGGetDevices(
	IN PMESSENGER_CONTEXT MSGContext
);

EXPORT LONG WINAPI MSGGetAppleMode(
	IN PMESSENGER_CONTEXT MSGContext,
	IN LONG DeviceInd
);

EXPORT LONG WINAPI MSGGetConfiguration(
	IN PMESSENGER_CONTEXT MSGContext,
	IN LONG DeviceInd
);

EXPORT LONG WINAPI MSGSetConfiguration(
	IN PMESSENGER_CONTEXT MSGContext,
	IN LONG DeviceInd,
	IN LONG Configuration
);

EXPORT VOID WINAPI MSGClose(IN PMESSENGER_CONTEXT MSGContext);
