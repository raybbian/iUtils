#pragma once

#include "driver.h"
#include "child.h"

NTSTATUS UrbHandleDispatch(
	PIU_CHILD_DEVICE Dev,
	PIRP Irp
);

NTSTATUS UrbHandleGetDescriptorfromDevice(
	PIU_CHILD_DEVICE Dev,
	PURB Urb
);

NTSTATUS UrbHandleSetConfiguration(
	PIU_CHILD_DEVICE Dev,
	PURB Urb
);
