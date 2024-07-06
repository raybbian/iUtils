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
		WdfIoQueueDispatchSequential
	);

	queueConfig.EvtIoDeviceControl = KeystoneEvtIoDeviceControl;
	queueConfig.EvtIoInternalDeviceControl = KeystoneEvtIoInternalDeviceControl;
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

	switch (IoControlCode) {
	case IOCTL_QUERY_APPLE_MODE: {
		LOG_INFO("query apple mode");
		WDFMEMORY outMemory;
		status = WdfRequestRetrieveOutputMemory(Request, &outMemory);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Could not get output memory. Status = %X", status);
			goto Cleanup;
		}
		LOG_INFO("returning apple mode %d", Dev->AppleMode);
		PUCHAR outBuffer = (PUCHAR)WdfMemoryGetBuffer(outMemory, NULL);
		if (outBuffer == NULL) {
			LOG_ERROR("Out buffer is null");
			status = STATUS_INVALID_PARAMETER;
			goto Cleanup;
		}
		outBuffer[0] = Dev->AppleMode;
		WdfRequestSetInformation(Request, 1);
		break;
	}
	case IOCTL_SET_APPLE_MODE: {
		LOG_INFO("set apple mode");
		WDFMEMORY inMemory;
		status = WdfRequestRetrieveInputMemory(Request, &inMemory);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Could not get input memory. Status = %X", status);
			goto Cleanup;
		}
		PUCHAR inBuffer = (PUCHAR)WdfMemoryGetBuffer(inMemory, NULL);
		if (inBuffer == NULL) {
			LOG_ERROR("In buffer is null");
			status = STATUS_INVALID_PARAMETER;
			goto Cleanup;
		}
		status = SetAppleMode(Dev, inBuffer[0]);
		if (!NT_SUCCESS(status)) {
			LOG_INFO("Failed to send set apple mode");
			goto Cleanup;
		}
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;	
		break;
	}
Cleanup:
	WdfRequestComplete(Request, status);
}

VOID KeystoneEvtIoInternalDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t OutputBufferLength,
	IN size_t InputBufferLength,
	IN ULONG IoControlCode
) {
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	NTSTATUS status = STATUS_SUCCESS;

	switch (IoControlCode) {
	case IOCTL_INTERNAL_USB_SUBMIT_URB: {
		PIRP Irp = WdfRequestWdmGetIrp(Request);
		PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
		PURB Urb = (PURB)irpStack->Parameters.Others.Argument1;
		if (Urb == NULL) {
			status = STATUS_INVALID_PARAMETER;
			goto Cleanup;
		}
		switch (Urb->UrbHeader.Function) {
		case URB_FUNCTION_CONTROL_TRANSFER:
		case URB_FUNCTION_CONTROL_TRANSFER_EX: 
			//these forwarded from child pdo so no sync issues
			SendRequestToNext(Request);
			return;
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			goto Cleanup;
		}
		break;
	}
	default:
		LOG_ERROR("IOCTL %u badly forwarded to parent!", IoControlCode);
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto Cleanup;
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

VOID SendRequestToNext(
	WDFREQUEST Request
) {
	WDFIOTARGET target = WdfDeviceGetIoTarget(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	WdfRequestFormatRequestUsingCurrentType(Request);
	WDF_REQUEST_SEND_OPTIONS options;
	WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
	if (!WdfRequestSend(Request, target, &options))
		WdfRequestComplete(Request, WdfRequestGetStatus(Request));
}
