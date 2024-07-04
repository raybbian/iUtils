#pragma once

#include "driver.h"
#include "child.h"

//TODO: queue context?

NTSTATUS KeystoneChildQueueInitialize(
    IN WDFDEVICE Device
);
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL KeystoneChildEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL KeystoneChildEvtIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_DEFAULT KeystoneChildEvtIoDefault;
EVT_WDF_IO_QUEUE_IO_STOP KeystoneChildEvtIoStop;

VOID ForwardRequestToParent(
	WDFREQUEST Request
);

VOID RequestIsUnsupported(
	WDFREQUEST Request
);
