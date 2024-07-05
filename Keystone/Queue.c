#include "queue.h"
#include "public.h"
#include "log.h"

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
	PIU_DEVICE Dev = DeviceGetContext(WdfIoQueueGetDevice(Queue));
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);
	switch (IoControlCode) {
	case IOCTL_QUERY_APPLE_MODE:
		LOG_INFO("query apple mode called %d", Dev->AppleMode);
		WdfRequestComplete(Request, STATUS_SUCCESS);
		break;
	case IOCTL_SET_APPLE_MODE:
		LOG_INFO("query apple mode called");
		WdfRequestComplete(Request, STATUS_SUCCESS);
		break;
	default:
		WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
		break;
	}
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
