#include "queue.h"
#include "public.h"
#include "log.h"
#include "apple.h"
#include <usbioctl.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, KeystoneQueueInitialize)
#endif

NTSTATUS KeystoneQueueInitialize(
	IN WDFDEVICE Device
) {
	NTSTATUS status;
	PAGED_CODE();

	WDF_IO_QUEUE_CONFIG queueConfig;
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		&queueConfig,
		WdfIoQueueDispatchParallel
	);

	queueConfig.EvtIoDeviceControl = KeystoneEvtIoDeviceControl;
	queueConfig.EvtIoStop = KeystoneEvtIoStop;

	status = WdfIoQueueCreate(
		Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		NULL
	);

	if (!NT_SUCCESS(status)) {
		LOG_ERROR("queue create failed");
		return status;
	}
	LOG_INFO("queue created");

	return status;
}

VOID KeystoneEvtIoDeviceControl(
	WDFQUEUE Queue,
	WDFREQUEST Request,
	size_t OutputBufferLength,
	size_t InputBufferLength,
	ULONG IoControlCode
) {
	NTSTATUS status = STATUS_SUCCESS;
	PIU_DEVICE Dev = DeviceGetContext(WdfIoQueueGetDevice(Queue));
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	LOG_INFO("Control request for %ws", Dev->Udid);

	switch (IoControlCode) {
	//case MY THING:
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;	
		goto Cleanup;
		break;
	}
Cleanup:
	WdfRequestComplete(Request, status);
}

VOID KeystoneEvtIoStop(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN ULONG ActionFlags
) {
	UNREFERENCED_PARAMETER(Queue);
	LOG_INFO("fdo IoStop Request 0x % p ActionFlags % d", Request, ActionFlags);
	return;
}
