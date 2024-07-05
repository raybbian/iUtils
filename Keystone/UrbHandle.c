#include "urbhandle.h"
#include "configuration.h"
#include "child.h"
#include "log.h"
#include "urbsend.h"
#include "childqueue.h"

VOID UrbCompleteDispatch(
	WDFREQUEST Request
) {
	PIRP Irp = WdfRequestWdmGetIrp(Request);
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	PURB Urb = (PURB)irpStack->Parameters.Others.Argument1;
	if (Urb == NULL) {
		WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
		return;
	}

	switch (Urb->UrbHeader.Function) {
	case URB_FUNCTION_SELECT_CONFIGURATION:
		UrbCompleteSetConfiguration(Request);
		break;
	case URB_FUNCTION_SELECT_INTERFACE:
		UrbCompleteSetInterface(Request);
		break;
	case URB_FUNCTION_CONTROL_TRANSFER:
	case URB_FUNCTION_CONTROL_TRANSFER_EX:
		LOG_INFO("ctrl xfer");
		ForwardRequestBeyondFDO(Request);
		break;
	case URB_FUNCTION_ABORT_PIPE:
	case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
	case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
	case URB_FUNCTION_ISOCH_TRANSFER:
	case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
	case URB_FUNCTION_SYNC_CLEAR_STALL:
	case URB_FUNCTION_SYNC_RESET_PIPE:
	case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
	case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
	case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
	case URB_FUNCTION_GET_STATUS_FROM_OTHER:
	case URB_FUNCTION_GET_CONFIGURATION:
		ForwardRequestBeyondFDO(Request);
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
		UrbCompleteGetDescriptorfromDevice(Request);
		LOG_INFO("returned descriptor!");
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
		RequestIsUnsupported(Request);
		LOG_INFO("return endpoint descriptor");
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
		RequestIsUnsupported(Request);
		LOG_INFO("return interface descriptor");
		break;
	case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
	case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
	case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
		RequestIsUnsupported(Request);
		LOG_INFO("allow setting descriptors?");
		break;
	case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
	case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
	case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
	case URB_FUNCTION_SET_FEATURE_TO_OTHER:
		LOG_INFO("allow set feature?");
		RequestIsUnsupported(Request);
		break;
	case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
	case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
	case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
	case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
		LOG_INFO("allow clear feature?");
		RequestIsUnsupported(Request);
		break;
	case URB_FUNCTION_VENDOR_DEVICE:
	case URB_FUNCTION_VENDOR_INTERFACE:
	case URB_FUNCTION_VENDOR_ENDPOINT:
	case URB_FUNCTION_VENDOR_OTHER:
	case URB_FUNCTION_CLASS_DEVICE:
	case URB_FUNCTION_CLASS_INTERFACE:
	case URB_FUNCTION_CLASS_ENDPOINT:
	case URB_FUNCTION_CLASS_OTHER:
		LOG_INFO("Forwarding vendor/class urb");
		ForwardRequestBeyondFDO(Request);
		break;
	case URB_FUNCTION_GET_INTERFACE:
		UrbCompleteGetInterface(Request);
		break;
	case URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR:
		LOG_INFO("return ms feature descriptor unsupported");
		RequestIsUnsupported(Request);
		break;
	default:
		LOG_INFO("unrecognized URB has been sent!");
		RequestIsUnsupported(Request);
		break;
	}
}

VOID UrbCompleteGetDescriptorfromDevice(
	WDFREQUEST Request
) {
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	PIU_CHILD_DEVICE Dev = ChildDeviceGetContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	PIRP Irp = WdfRequestWdmGetIrp(Request);
	PURB Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;

	ULONG transferBufferSz = Urb->UrbControlDescriptorRequest.TransferBufferLength;
	PVOID transferBuffer = Urb->UrbControlDescriptorRequest.TransferBuffer;
	UCHAR descriptionInd = Urb->UrbControlDescriptorRequest.Index;
	if (!transferBuffer) {
		LOG_ERROR("MDL (DMA?) not implemented yet!");
		RequestIsUnsupported(Request);
		return;
	}

	switch (Urb->UrbControlDescriptorRequest.DescriptorType) {
	case USB_DEVICE_DESCRIPTOR_TYPE:
		RtlCopyMemory(
			transferBuffer,
			&Dev->DeviceDescriptor,
			min(transferBufferSz, sizeof(Dev->DeviceDescriptor))
		);
		Urb->UrbControlDescriptorRequest.TransferBufferLength = min(transferBufferSz, sizeof(Dev->DeviceDescriptor));
		status = STATUS_SUCCESS;
		break;
	case USB_CONFIGURATION_DESCRIPTOR_TYPE:
		RtlCopyMemory(
			transferBuffer,
			Dev->Config.Descriptor,
			min(transferBufferSz, Dev->Config.Descriptor->wTotalLength)
		);
		Urb->UrbControlDescriptorRequest.TransferBufferLength = min(transferBufferSz, Dev->Config.Descriptor->wTotalLength);
		status = STATUS_SUCCESS;
		break;
	case USB_STRING_DESCRIPTOR_TYPE:
		ForwardRequestBeyondFDO(Request);
		return;
	default: {
		PUSB_COMMON_DESCRIPTOR desc = NULL;
		for (UCHAR i = 0; i <= descriptionInd; i++) {
			desc = USBD_ParseDescriptors(
				Dev->Config.Descriptor, 
				Dev->Config.Descriptor->wTotalLength, 
				Dev->Config.Descriptor, 
				Urb->UrbControlDescriptorRequest.DescriptorType
			);
			if (!desc) {
				LOG_ERROR("Desc intex oob");
				status = STATUS_NO_MORE_ENTRIES;
				goto Cleanup;
			}
		}
		if (!desc) { //appease compiler
			status = STATUS_UNSUCCESSFUL;
			goto Cleanup;
		}
		RtlCopyMemory(
			transferBuffer,
			desc,
			min(transferBufferSz, desc->bLength)
		);
		Urb->UrbControlDescriptorRequest.TransferBufferLength = min(transferBufferSz, desc->bLength);
		status = STATUS_SUCCESS;
		break;
	}
	}
Cleanup:
	WdfRequestComplete(Request, status);
}

static NTSTATUS SetAndFillInterfaceInformation(
	PIU_CHILD_DEVICE Dev,
	PUSBD_INTERFACE_INFORMATION Intf
) {
	NTSTATUS status = STATUS_SUCCESS;
	if (Intf == NULL) {
		return STATUS_INVALID_PARAMETER;
	}
	UCHAR intfNum = Intf->InterfaceNumber;
	if (Intf->AlternateSetting != Dev->Parent->Config.Interfaces[intfNum].AlternateSetting) {
		//if the alternate setting for that interface doesn't match the one we have, switch it
		LOG_INFO("child switching interface %d to altsetting %d", intfNum, Intf->AlternateSetting);
		status = SetInterface(Dev->Parent, intfNum, Intf->AlternateSetting);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Failed to change interface as requested by child");
			return status;
		}
	}

	RtlCopyMemory(
		Intf, 
		&Dev->Parent->Config.Interfaces[intfNum], 
		Dev->Parent->Config.Interfaces[intfNum].Length
	);
	return status;
}

VOID UrbCompleteSetConfiguration(
	WDFREQUEST Request
) {
	NTSTATUS status = STATUS_SUCCESS;
	PIU_CHILD_DEVICE Dev = ChildDeviceGetContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	PIRP Irp = WdfRequestWdmGetIrp(Request);
	PURB Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;

	PUSB_CONFIGURATION_DESCRIPTOR desc = Urb->UrbSelectConfiguration.ConfigurationDescriptor;
	if (!desc || desc->bConfigurationValue != Dev->Parent->Config.Descriptor->bConfigurationValue) {
		LOG_ERROR("Child cannot change configuration of parent!");
		status = STATUS_UNSUCCESSFUL;
		goto Cleanup;
	}

	//configuration is already set, so we should just return the pipe information and handles, and set altsettings
	USBD_INTERFACE_INFORMATION* curIntf = &Urb->UrbSelectConfiguration.Interface;
	for (UCHAR i = 0; i < desc->bNumInterfaces; i++) { //for each interface
		status = SetAndFillInterfaceInformation(Dev, curIntf);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("failed to fill interface information");
			goto Cleanup;
		}
	}

	LOG_INFO("Set config for child");
	Urb->UrbSelectConfiguration.ConfigurationHandle = Dev->Parent->Config.Handle;
Cleanup:
	WdfRequestComplete(Request, status);
}

VOID UrbCompleteSetInterface(
	WDFREQUEST Request
) {
	NTSTATUS status = STATUS_SUCCESS;
	PIU_CHILD_DEVICE Dev = ChildDeviceGetContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	PIRP Irp = WdfRequestWdmGetIrp(Request);
	PURB Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;

	PUSBD_INTERFACE_INFORMATION curIntf = &Urb->UrbSelectInterface.Interface;
	status = SetAndFillInterfaceInformation(Dev, curIntf);
	if (!NT_SUCCESS(status)) {
		LOG_INFO("Failed to set interface for child");
		goto Cleanup;
	}
	LOG_INFO("Set interface for child");
Cleanup:
	WdfRequestComplete(Request, status);
}

VOID UrbCompleteGetInterface(
	WDFREQUEST Request
) {
	NTSTATUS status = STATUS_SUCCESS;
	PIU_CHILD_DEVICE Dev = ChildDeviceGetContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));
	PIRP Irp = WdfRequestWdmGetIrp(Request);
	PURB Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;

	if (Urb->UrbControlGetInterfaceRequest.TransferBufferLength != 1) {
		status = STATUS_UNSUCCESSFUL;
		goto Cleanup;
	}
	if (Urb->UrbControlGetInterfaceRequest.TransferBuffer == NULL) {
		LOG_ERROR("no mdl supported yet");
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto Cleanup;
	}
	USHORT reqIntfInd = Urb->UrbControlGetInterfaceRequest.Interface;
	if (reqIntfInd >= Dev->Config.Descriptor->bNumInterfaces || reqIntfInd < 0) {
		LOG_ERROR("requested interface out of index");
	}
	PUCHAR startP = (PUCHAR)Dev->Config.Descriptor;
	for (USHORT i = 0; i <= reqIntfInd; i++) {
		PUSB_INTERFACE_DESCRIPTOR intf = USBD_ParseConfigurationDescriptorEx(
			Dev->Config.Descriptor,
			startP,
			-1, 
			0, //only check per interface, not per setting
			-1, -1, -1
		);
		if (!intf) {
			LOG_ERROR("Not enough intfs");
			status = STATUS_BAD_DATA;
			goto Cleanup;
		}
		*(PUCHAR)Urb->UrbControlGetInterfaceRequest.TransferBuffer = 
			Dev->Parent->Config.Interfaces[intf->bInterfaceNumber].AlternateSetting;
		startP = (PUCHAR)intf + intf->bLength;
	}
	LOG_INFO("returned interface altsetting");

Cleanup:
	WdfRequestComplete(Request, status);
}