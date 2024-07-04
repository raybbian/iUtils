#pragma once

#include "driver.h"

NTSTATUS ActivatePTPFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
);

NTSTATUS ActivateUsbMuxFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
);
