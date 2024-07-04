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

	switch (Urb->UrbHeader.Function) {
	case URB_FUNCTION_SELECT_CONFIGURATION:
		LOG_INFO("handled set config");
		UrbCompleteSetConfiguration(Request);
		break;
	case URB_FUNCTION_SELECT_INTERFACE:
		LOG_INFO("need to select and return interface handle?");
		RequestIsUnsupported(Request);
		break;
	case URB_FUNCTION_ABORT_PIPE:
		LOG_INFO("need to clear and cancel any transformations going back");
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
		LOG_INFO("need to return current frame number (passthrough?)");
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_CONTROL_TRANSFER:
	case URB_FUNCTION_CONTROL_TRANSFER_EX:
		LOG_INFO("need to process and make control transfer target child");
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_ISOCH_TRANSFER:
		LOG_INFO("no? need to handle isoch");
		RequestIsUnsupported(Request);
		break;
	case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
		LOG_INFO("Need to handle resetting of pipe, data toggle, etc.");
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_SYNC_CLEAR_STALL:
	case URB_FUNCTION_SYNC_RESET_PIPE:
		LOG_INFO("usually used for defective device.. required?");
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
		UrbCompleteGetDescriptorfromDevice(Request);
		LOG_INFO("returned descriptor!");
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
		LOG_INFO("return endpoint descriptor");
		RequestIsUnsupported(Request);
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
		LOG_INFO("return interface descriptor");
		RequestIsUnsupported(Request);
		break;
	case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
	case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
	case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
		LOG_INFO("allow setting descriptors?");
		RequestIsUnsupported(Request);
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
	case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
	case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
	case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
	case URB_FUNCTION_GET_STATUS_FROM_OTHER:
		LOG_INFO("forward get status request?");
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_VENDOR_DEVICE:
	case URB_FUNCTION_VENDOR_INTERFACE:
	case URB_FUNCTION_VENDOR_ENDPOINT:
	case URB_FUNCTION_VENDOR_OTHER:
		LOG_INFO("Forwarding vendor urb");
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_GET_CONFIGURATION:
	case URB_FUNCTION_GET_INTERFACE:
		LOG_INFO("return proper values");
		ForwardRequestToParent(Request);
		break;
	case URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR:
		LOG_INFO("return ms feature descriptor");
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
	if (!transferBuffer) {
		LOG_ERROR("MDL (DMA?) not implemented yet!");
		ForwardRequestToParent(Request);
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
	default:
		LOG_INFO("Unsupported get descriptor from device %d", Urb->UrbControlDescriptorRequest.DescriptorType);
		break;
	}
	WdfRequestComplete(Request, status);
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
	USBD_INTERFACE_INFORMATION* curIntf = (USBD_INTERFACE_INFORMATION*)&Urb->UrbSelectConfiguration.Interface;

	//configuration is already set, so we should just return the pipe information and handles, and set altsettings
	for (UCHAR i = 0; i < desc->bNumInterfaces; i++) { //for each interface
		UCHAR intfNum = curIntf->InterfaceNumber;
		if (intfNum > IU_MAX_NUMBER_OF_INTERFACES) {
			LOG_ERROR("intfnum child requested is too high");
			status = STATUS_NO_MORE_ENTRIES;
			goto Cleanup;
		}

		if (curIntf->AlternateSetting != Dev->Parent->Config.InterfaceAltsetting[intfNum]) {
			//if the alternate setting for that interface doesn't match the one we have, switch it
			LOG_INFO("child switching interface %d to altsetting %d", intfNum, curIntf->AlternateSetting);
			status = SetInterface(Dev->Parent, intfNum, curIntf->AlternateSetting);
			if (!NT_SUCCESS(status)) {
				LOG_ERROR("Failed to change interface as requested by child");
				goto Cleanup;
			}
		}

		PUSB_INTERFACE_DESCRIPTOR parentIntfDesc = USBD_ParseConfigurationDescriptor(
			Dev->Config.Descriptor,
			intfNum,
			curIntf->AlternateSetting
		);
		if (!parentIntfDesc) {
			LOG_ERROR("Selected interface does not exist on this configuration");
			status = STATUS_UNSUCCESSFUL;
			goto Cleanup;
		}

		//fill information requested (should matck)
		curIntf->InterfaceNumber = parentIntfDesc->bInterfaceNumber;
		curIntf->AlternateSetting = parentIntfDesc->bAlternateSetting;
		curIntf->InterfaceHandle = Dev->Parent->Config.InterfaceHandles[intfNum];
		curIntf->Class = parentIntfDesc->bInterfaceClass;
		curIntf->SubClass = parentIntfDesc->bInterfaceSubClass;
		curIntf->Protocol = parentIntfDesc->bInterfaceProtocol;
		curIntf->NumberOfPipes = parentIntfDesc->bNumEndpoints;

		//loop over and find endpoint descriptors to access pipe handles
		PUCHAR startP = (PUCHAR)parentIntfDesc;
		for (UCHAR j = 0; j < curIntf->NumberOfPipes; j++) {
			PUSB_ENDPOINT_DESCRIPTOR endpointDesc = (PUSB_ENDPOINT_DESCRIPTOR)USBD_ParseDescriptors(
				Dev->Parent->Config.Descriptor,
				Dev->Parent->Config.Descriptor->wTotalLength,
				startP,
				USB_ENDPOINT_DESCRIPTOR_TYPE
			);
			if (!endpointDesc) {
				LOG_ERROR("Interface mismatched number of endpoints");
				status = STATUS_BAD_DATA;
				goto Cleanup;
			}
			UCHAR pipeNum = endpointDesc->bEndpointAddress & 0x0F;
			curIntf->Pipes[j] = Dev->Parent->Config.Pipes[pipeNum];
			startP = (PUCHAR)endpointDesc + endpointDesc->bLength;
		}
	}

	LOG_INFO("Child got configs");
	Urb->UrbSelectConfiguration.ConfigurationHandle = Dev->Parent->Config.Handle;
Cleanup:
	WdfRequestComplete(Request, status);
}