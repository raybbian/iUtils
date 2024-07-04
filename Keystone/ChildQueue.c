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

	queueConfig.EvtIoDeviceControl = KeystoneChildEvtIoDeviceControl;
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
	LOG_INFO("Default io handle");
	ForwardRequestToParent(Request);
}

VOID KeystoneChildEvtIoDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t OutputBufferLength,
	IN size_t InputBufferLength,
	IN ULONG IoControlCode
) {
	UNREFERENCED_PARAMETER(Queue);
	LOG_INFO("IoDeviceControl OutputBufferLength % d InputBufferLength % d IoControlCode % d",
		(int)OutputBufferLength, (int)InputBufferLength, IoControlCode);
	ForwardRequestToParent(Request);
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
		ForwardRequestToParent(Request);
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

	//
	// In most cases, the EvtIoStop callback function completes, cancels, or postpones
	// further processing of the I/O request.
	//
	// Typically, the driver uses the following rules:
	//
	// - If the driver owns the I/O request, it either postpones further processing
	//   of the request and calls WdfRequestStopAcknowledge, or it calls WdfRequestComplete
	//   with a completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
	//  
	//   The driver must call WdfRequestComplete only once, to either complete or cancel
	//   the request. To ensure that another thread does not call WdfRequestComplete
	//   for the same request, the EvtIoStop callback must synchronize with the driver's
	//   other event callback functions, for instance by using interlocked operations.
	//
	// - If the driver has forwarded the I/O request to an I/O target, it either calls
	//   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
	//   further processing of the request and calls WdfRequestStopAcknowledge.
	//
	// A driver might choose to take no action in EvtIoStop for requests that are
	// guaranteed to complete in a small amount of time. For example, the driver might
	// take no action for requests that are completed in one of the driver’s request handlers.
	//

	return;
}

VOID ForwardRequestToParent(
	WDFREQUEST Request
) {
	WDFDEVICE ParentDev = WdfPdoGetParent(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	WDF_REQUEST_FORWARD_OPTIONS options;
	WDF_REQUEST_FORWARD_OPTIONS_INIT(&options); // send and forget
	NTSTATUS status = WdfRequestForwardToParentDeviceIoQueue(Request, WdfDeviceGetDefaultQueue(ParentDev), &options);
	if (!NT_SUCCESS(status)) 
		WdfRequestComplete(Request, status);
}

VOID RequestIsUnsupported(
	WDFREQUEST Request
) {
	WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}
