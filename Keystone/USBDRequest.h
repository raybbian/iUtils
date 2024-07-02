#pragma once

#include "driver.h"

NTSTATUS SendUSBDRequest(
	IN PIU_DEVICE Dev,
	INOUT PVOID Urb
);

NTSTATUS SendUSBDRequestEx(
	IN PIU_DEVICE Dev,
	INOUT PVOID Urb,
	IN ULONG ControlCode,
	IN LONG Timeout
);
