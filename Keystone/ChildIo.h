#pragma once

#include "driver.h"
#include "child.h"

//TODO: queue context?

NTSTATUS KeystoneChildQueueInitialize(
    IN WDFDEVICE Device
);
EVT_WDF_IO_QUEUE_IO_READ KeystoneChildEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE KeystoneChildEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL KeystoneChildEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL KeystoneChildEvtIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP KeystoneChildEvtIoStop;

NTSTATUS ForwardIRPToParent(
	IN PIU_CHILD_DEVICE Dev,
	IN PIRP Irp
);
