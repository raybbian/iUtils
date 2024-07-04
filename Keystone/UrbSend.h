#pragma once

#include "driver.h"

NTSTATUS SendUrbSync(
	IN PIU_DEVICE Dev,
	INOUT PVOID Urb
);

NTSTATUS SendUrbSyncEx(
	IN PIU_DEVICE Dev,
	INOUT PVOID Urb,
	IN ULONG ControlCode,
	IN LONG Timeout
);
