#include "childqueue.h"
#include "log.h"
#include "child.h"
#include "urbhandle.h"
#include <usbioctl.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, KeystoneChildQueueInitialize)
#endif

NTSTATUS KeystoneChildQueueInitialize(
	IN WDFDEVICE Device
) {
	NTSTATUS status;
	PAGED_CODE();

	WDF_IO_QUEUE_CONFIG queueConfig;
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		&queueConfig,
		WdfIoQueueDispatchParallel
	);

	queueConfig.EvtIoInternalDeviceControl = KeystoneChildEvtIoInternalDeviceControl;
	queueConfig.EvtIoDefault = KeystoneChildEvtIoDefault;
	queueConfig.EvtIoStop = KeystoneChildEvtIoStop;

	WDFQUEUE queue;
	status = WdfIoQueueCreate(
		Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&queue
	);

	if (!NT_SUCCESS(status)) {
		LOG_ERROR("queue create failed");
		return status;
	}
	LOG_INFO("queue created");

	return status;
}

VOID KeystoneChildEvtIoDefault(
	WDFQUEUE Queue,
	WDFREQUEST Request
) {
	UNREFERENCED_PARAMETER(Queue);
	LOG_INFO("Default io handle"); //for debug purposes
	RequestIsUnsupported(Request);
}

VOID KeystoneChildEvtIoInternalDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t OutputBufferLength,
	IN size_t InputBufferLength,
	IN ULONG IoControlCode
) {
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	switch (IoControlCode) {
	case IOCTL_INTERNAL_USB_SUBMIT_URB: {
		UrbCompleteDispatch(Request);
		break;
	}
	default:
		LOG_ERROR("IOCTL %u not implemented yet!", IoControlCode);
		RequestIsUnsupported(Request);
		break;
	}
}

VOID KeystoneChildEvtIoStop(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN ULONG ActionFlags
) {
	UNREFERENCED_PARAMETER(Queue);
	LOG_INFO("IoStop Request 0x % p ActionFlags % d", Request, ActionFlags);
	return;
}

VOID ForwardRequestBeyondFDO(
	WDFREQUEST Request
) {
	WDFDEVICE Device = WdfPdoGetParent(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	WDFIOTARGET target = WdfDeviceGetIoTarget(Device);
	WdfRequestFormatRequestUsingCurrentType(Request);
	WDF_REQUEST_SEND_OPTIONS options;
	WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
	if (!WdfRequestSend(Request, target, &options))
		WdfRequestComplete(Request, WdfRequestGetStatus(Request));
}

VOID RequestIsUnsupported(
	WDFREQUEST Request
) {
	WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}
