#pragma once

#include "driver.h"

NTSTATUS KeystoneQueueInitialize(
	IN WDFDEVICE Device
);

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL KeystoneEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL KeystoneEvtIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP KeystoneEvtIoStop;

VOID SendRequestToNext(
	WDFREQUEST Request
);
