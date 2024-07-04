#include "queue.h"
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

	queueConfig.EvtIoDefault = KeystoneEvtIoDefault;

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

VOID KeystoneEvtIoDefault(
	WDFQUEUE Queue,
	WDFREQUEST Request
) {
	WDFDEVICE Device = WdfIoQueueGetDevice(Queue);
	WDF_REQUEST_SEND_OPTIONS options;
	WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
	if (!WdfRequestSend(Request, WdfDeviceGetIoTarget(Device), &options)) 
		WdfRequestComplete(Request, WdfRequestGetStatus(Request));
}
