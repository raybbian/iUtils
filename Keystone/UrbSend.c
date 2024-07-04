#include "Driver.h"
#include <usbioctl.h>
#include "urbsend.h"
#include "log.h"

typedef enum {
	IRPLOCK_CANCELABLE,
	IRPLOCK_CANCEL_STARTED,
	IRPLOCK_CANCEL_COMPLETE,
	IRPLOCK_COMPLETED
} IRPLOCK;

static NTSTATUS OnSendUrbSyncComplete(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	INOUT PVOID Context
) {
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	PLONG lock = (PLONG)Context;

	if (InterlockedExchange(lock, IRPLOCK_COMPLETED) == IRPLOCK_CANCEL_STARTED) {
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	return STATUS_SUCCESS;
}

NTSTATUS SendUrbSync(
	IN PIU_DEVICE Dev,
	INOUT PVOID Urb
) {
	return SendUrbSyncEx(Dev, Urb, IOCTL_INTERNAL_USB_SUBMIT_URB, 1000);
}

NTSTATUS SendUrbSyncEx(
	IN PIU_DEVICE Dev,
	INOUT PVOID Urb,
	IN ULONG ControlCode,
	IN LONG Timeout
) {
	NTSTATUS status;

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	IO_STATUS_BLOCK io_status;
	IRP* irp = IoBuildDeviceIoControlRequest(
		ControlCode,
		Dev->WDM.NextStackDevice,
		NULL,
		0,
		NULL,
		0,
		TRUE,
		&event,
		&io_status
	);
	if (!irp) {
		return STATUS_NO_MEMORY;
	}

	//not broken, but maybe use forward irp synchronously instead?
	IO_STACK_LOCATION* nextIrpStack = IoGetNextIrpStackLocation(irp);
	nextIrpStack->Parameters.Others.Argument1 = Urb;
	nextIrpStack->Parameters.Others.Argument2 = NULL;

	IRPLOCK lock = IRPLOCK_CANCELABLE;

	IoSetCompletionRoutine(irp, OnSendUrbSyncComplete, &lock, TRUE, TRUE, TRUE);

	status = IoCallDriver(Dev->WDM.NextStackDevice, irp);

	if (status == STATUS_PENDING) {
		LARGE_INTEGER dueTime;
		dueTime.QuadPart = -(Timeout * 10000);

		status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &dueTime);
		if (status == STATUS_TIMEOUT) {
			// timeout
			if (InterlockedExchange((void*)&lock, IRPLOCK_CANCEL_STARTED) == IRPLOCK_CANCELABLE) {
				IoCancelIrp(irp);
				if (InterlockedExchange((void*)&lock, IRPLOCK_CANCEL_COMPLETE) == IRPLOCK_COMPLETED) {
					IoCompleteRequest(irp, IO_NO_INCREMENT);
				}
			}
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
			LOG_ERROR("Request timed out. Status = %08Xh", status);
			io_status.Status = status;
			return status;
		}
		LOG_INFO("Request success, below handled async. Status = %08Xh", io_status.Status);
		return io_status.Status;
	}
	LOG_INFO("Request success, below handled sync. Status = %08Xh", status);
	return status;
}
