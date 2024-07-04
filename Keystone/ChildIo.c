#include "childio.h"
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

	queueConfig.EvtIoRead = KeystoneChildEvtIoRead;
	queueConfig.EvtIoWrite = KeystoneChildEvtIoWrite;
	queueConfig.EvtIoDeviceControl = KeystoneChildEvtIoDeviceControl;
	queueConfig.EvtIoInternalDeviceControl = KeystoneChildEvtIoInternalDeviceControl;
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

VOID KeystoneChildEvtIoRead(
	WDFQUEUE Queue,
	WDFREQUEST Request,
	size_t Length
) {
	UNREFERENCED_PARAMETER(Queue);
	LOG_INFO("Read request length %d", (int)Length);

	WdfRequestComplete(Request, STATUS_SUCCESS);
	return;
}

VOID KeystoneChildEvtIoWrite(
	WDFQUEUE Queue,
	WDFREQUEST Request,
	size_t Length
) {
	UNREFERENCED_PARAMETER(Queue);
	LOG_INFO("Write request length %d", (int)Length);

	WdfRequestComplete(Request, STATUS_SUCCESS);
	return;
}

VOID KeystoneChildEvtIoDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t OutputBufferLength,
	IN size_t InputBufferLength,
	IN ULONG IoControlCode
) {
	UNREFERENCED_PARAMETER(Queue);
	//This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.
	LOG_INFO("IoDeviceControl OutputBufferLength % d InputBufferLength % d IoControlCode % d",
		(int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

	WdfRequestComplete(Request, STATUS_SUCCESS);
	return;
}

VOID KeystoneChildEvtIoInternalDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t OutputBufferLength,
	IN size_t InputBufferLength,
	IN ULONG IoControlCode
) {
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	PIU_CHILD_DEVICE Dev = ChildDeviceGetContext(WdfIoQueueGetDevice(Queue));
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	switch (IoControlCode) {
	case IOCTL_INTERNAL_USB_SUBMIT_URB: {
		PIRP irp = WdfRequestWdmGetIrp(Request);
		status = UrbHandleDispatch(Dev, irp);
		break;
	}
	default:
		LOG_ERROR("IOCTL %u not implemented yet!", IoControlCode);
		break;
	}

	WdfRequestComplete(Request, status);
	return;
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

NTSTATUS ForwardIRPToParent(
	IN PIU_CHILD_DEVICE Dev,
	IN PIRP Irp
) {
	LOG_INFO("irp forwarded");
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(Dev->Parent->WDM.NextStackDevice, Irp);
}
