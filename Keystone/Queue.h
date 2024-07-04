#pragma once

#include "driver.h"

NTSTATUS KeystoneQueueInitialize(
	IN WDFDEVICE Device
);

EVT_WDF_IO_QUEUE_IO_DEFAULT KeystoneEvtIoDefault;
