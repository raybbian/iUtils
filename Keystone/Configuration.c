#include "driver.h"
#include "configuration.h"
#include "log.h"
#include "urbsend.h"
#include <usbioctl.h>
#include "child.h"

NTSTATUS FillDeviceDescriptor(
	IN PIU_DEVICE Dev
) {
	NTSTATUS ntStatus = STATUS_SUCCESS;

	LOG_INFO("Filling device descriptor");

	URB urb;
	USB_DEVICE_DESCRIPTOR tmpDesc;
	RtlZeroMemory(&urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
	urb.UrbHeader.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
	urb.UrbHeader.Length = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
	urb.UrbControlDescriptorRequest.TransferBufferLength = sizeof(tmpDesc);
	urb.UrbControlDescriptorRequest.TransferBuffer = (PVOID)&tmpDesc;
	urb.UrbControlDescriptorRequest.DescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
	urb.UrbControlDescriptorRequest.Index = 0;
	urb.UrbControlDescriptorRequest.LanguageId = 0;

	ntStatus = SendUrbSync(Dev, &urb);
	if (!NT_SUCCESS(ntStatus) || !USBD_SUCCESS(urb.UrbHeader.Status)) {
		LOG_ERROR("Getting device descriptor failed: status 0x%x, urb status: 0x%x", ntStatus, urb.UrbHeader.Status);
		return ntStatus;
	}

	if (urb.UrbControlDescriptorRequest.TransferBufferLength < sizeof(USB_DEVICE_DESCRIPTOR)) {
		LOG_ERROR("Invalid device descriptor of length %d", urb.UrbControlDescriptorRequest.TransferBufferLength);
		return STATUS_BAD_DEVICE_TYPE;
	}

	RtlCopyMemory(&Dev->DeviceDescriptor, &tmpDesc, sizeof(USB_DEVICE_DESCRIPTOR));
	return ntStatus;
}

NTSTATUS GetCurrentConfiguration(
	IN PIU_DEVICE Dev,
	OUT PUCHAR Configuration,
	OUT PINT32 Ret
) {
	NTSTATUS ntStatus = STATUS_SUCCESS;

	LOG_INFO("Get configuration enter");

	URB urb;
	RtlZeroMemory(&urb, sizeof(urb));
	urb.UrbHeader.Function = URB_FUNCTION_GET_CONFIGURATION;
	urb.UrbHeader.Length = sizeof(struct _URB_CONTROL_GET_CONFIGURATION_REQUEST);
	urb.UrbControlGetConfigurationRequest.TransferBufferLength = 1;
	urb.UrbControlGetConfigurationRequest.TransferBuffer = Configuration;

	ntStatus = SendUrbSync(Dev, &urb);
	if (!NT_SUCCESS(ntStatus) || !USBD_SUCCESS(urb.UrbHeader.Status)) {
		LOG_ERROR("Getting configuration failed: status: 0x%x, urb-status: 0x%x", ntStatus, urb.UrbHeader.Status);
		*Ret = 0;
		return ntStatus;
	}

	LOG_INFO("Current configuration is: %d", *Configuration);
	*Ret = urb.UrbControlGetConfigurationRequest.TransferBufferLength;
	return ntStatus;
}

NTSTATUS GetConfigDescriptorByIndex(
	IN PIU_DEVICE Dev,
	OUT PVOID Buffer,
	IN ULONG Size,
	IN UCHAR Index, //0-indexed
	OUT PULONG Received
) {
	NTSTATUS ntStatus = STATUS_SUCCESS;
		*Received = 0;

	if (Dev->DeviceDescriptor.bLength == 0) {
		LOG_ERROR("Cannot get configurations without device descriptor");
		return STATUS_NOT_FOUND;
	}
	if (Size < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
		LOG_ERROR("Buffer size too small to get config descriptor");
		return STATUS_BUFFER_TOO_SMALL;
	}
	// if invalid index
	if (Index < 0 || Index >= Dev->DeviceDescriptor.bNumConfigurations) {
		LOG_ERROR("Invalid configuration index: OOB");
		return STATUS_NO_MORE_ENTRIES;
	}

	URB urb;
	RtlZeroMemory(&urb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
	urb.UrbHeader.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
	urb.UrbHeader.Length = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
	urb.UrbControlDescriptorRequest.TransferBufferLength = Size;
	urb.UrbControlDescriptorRequest.TransferBuffer = Buffer;
	urb.UrbControlDescriptorRequest.DescriptorType = USB_CONFIGURATION_DESCRIPTOR_TYPE;
	urb.UrbControlDescriptorRequest.Index = Index; // usb device itself is 0 index
	urb.UrbControlDescriptorRequest.LanguageId = 0;

	ntStatus = SendUrbSync(Dev, &urb);
	if (!NT_SUCCESS(ntStatus) || !USBD_SUCCESS(urb.UrbHeader.Status)) {
		LOG_ERROR("Getting config descriptor failed: status 0x%x, urb status: 0x%x", ntStatus, urb.UrbHeader.Status);
		*Received = 0;
		return ntStatus;
	}

	// NOTE: filter driver has special case here, I don't think I need it?

	// do not cache result here, the cache should only be set for set_configuration
	*Received = urb.UrbControlDescriptorRequest.TransferBufferLength;
	return ntStatus;
}

NTSTATUS GetConfigDescriptorByValue(
	IN PIU_DEVICE Dev,
	OUT PVOID Buffer,
	IN ULONG Size,
	IN UCHAR Value,
	OUT PULONG Received
) {
	NTSTATUS ntStatus = STATUS_SUCCESS;
	*Received = 0;

	if (Dev->DeviceDescriptor.bLength == 0) {
		LOG_ERROR("Cannot get configurations without device descriptor");
		return STATUS_NOT_FOUND;
	}

	//other checks satisfied by other config desc function (slow?)

	//try searching index equal to value - 1 first, likely to be there
	UCHAR i = 0;
	ntStatus = GetConfigDescriptorByIndex(Dev, Buffer, Size, Value - 1, Received);
	while (!NT_SUCCESS(ntStatus) || ((PUSB_CONFIGURATION_DESCRIPTOR)Buffer)->bConfigurationValue != Value) {
		if (i >= Dev->DeviceDescriptor.bNumConfigurations) {
			*Received = 0;
			return STATUS_NOT_FOUND;
		}
		if (i == Value - 1) continue;
		ntStatus = GetConfigDescriptorByIndex(Dev, Buffer, Size, i, Received);
		i++;
	}
	//received length should already be set from last get
	return ntStatus;
}

static NTSTATUS UnsetConfiguration(
	INOUT PIU_DEVICE Dev
) {
	LOG_INFO("Resetting configuration");
	NTSTATUS status = STATUS_SUCCESS;
	URB urb;
	RtlZeroMemory(&urb, sizeof(URB));
	urb.UrbHeader.Function = URB_FUNCTION_SELECT_CONFIGURATION;
	urb.UrbHeader.Length = sizeof(struct _URB_SELECT_CONFIGURATION);

	status = SendUrbSync(Dev, &urb);
	if (!NT_SUCCESS(status) || !USBD_SUCCESS(urb.UrbHeader.Status)) {
		LOG_ERROR("Resetting configuration failed: status: 0x%x, urb status: 0x%x", status, urb.UrbHeader.Status);
		return status;
	}

	RtlZeroMemory(&Dev->Config, sizeof(Dev->Config));
	Dev->Config.Handle = urb.UrbSelectConfiguration.ConfigurationHandle;

	return status;
}

NTSTATUS SetConfigurationByValue(
	INOUT PIU_DEVICE Dev,
	IN UCHAR Value
) {
	if (Dev->Config.Descriptor && Dev->Config.Descriptor->bConfigurationValue == Value) {
		LOG_INFO("Configuration already set to value");
		return STATUS_SUCCESS;
	}

	NTSTATUS status = STATUS_SUCCESS; 
	PURB urbp = NULL;
	PUSBD_INTERFACE_LIST_ENTRY interfaces = NULL;

	PurgeAllChildQueuesSynchronously(Dev);
	MarkAllChildrenAsMissing(Dev);

	if (Value == 0)  //deconfigure
		return UnsetConfiguration(Dev);
	//if the device is already initialized and in a nonzero configuration, and we want to change it, we first reset it
	BOOLEAN needsReset = Dev->Config.Descriptor && Dev->Config.Descriptor->bConfigurationValue != 0 && Value != 0;
	if (needsReset) {
		status = UnsetConfiguration(Dev);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Could not reset device before switch");
			goto Cleanup;
		}
	}

	// 1. Get the configuration descriptor
	
	ULONG configLen;
	UCHAR tmpBuf[IU_MAX_CONFIGURATION_BUFFER_SIZE];
	status = GetConfigDescriptorByValue(Dev, tmpBuf, sizeof(tmpBuf), Value, &configLen);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not get configuration descriptor for set config");
		goto Cleanup;
	}
	PUSB_CONFIGURATION_DESCRIPTOR tmpDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)tmpBuf;
	if (tmpDescriptor->wTotalLength > IU_MAX_CONFIGURATION_BUFFER_SIZE) {
		LOG_ERROR("Device config is too big!");
		status = STATUS_BUFFER_TOO_SMALL;
		goto Cleanup;
	}
	LOG_INFO("Config len is %d bytes", tmpDescriptor->wTotalLength);

	// 2. Build the interface list

	ULONG numInterfaces = tmpDescriptor->bNumInterfaces;
	interfaces = ExAllocatePoolZero(
		NonPagedPoolNx,
		sizeof(USBD_INTERFACE_LIST_ENTRY) * (numInterfaces + 1),
		IU_ALLOC_CONFIG_POOL
	);
	if (!interfaces) {
		LOG_ERROR("Interface array alloc failed");
		status = STATUS_NO_MEMORY;
		goto Cleanup;
	}

	PUSB_INTERFACE_DESCRIPTOR intfDesc;
	PUCHAR startP = tmpBuf;
	for (ULONG i = 0; i < numInterfaces; i++) {
		intfDesc = USBD_ParseConfigurationDescriptorEx(
			tmpDescriptor,
			startP,
			-1,	//intfnum
			0,	//altset
			-1,	//intfclass
			-1,	//intfsubclass
			-1	//intfproto
		);
		if (!intfDesc) {
			LOG_ERROR("Not enough interface descriptors");
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto Cleanup;
		}

		interfaces[i].InterfaceDescriptor = intfDesc;
		interfaces[i].Interface = NULL;
		startP = (PUCHAR)intfDesc + intfDesc->bLength; //skip recent interface descriptor
	}

	interfaces[numInterfaces].InterfaceDescriptor = NULL;

	// 3. Build the configuration urb

	status = USBD_SelectConfigUrbAllocateAndBuild(
		Dev->WDM.Handle, 
		tmpDescriptor, 
		interfaces, 
		&urbp
	);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to create select config URB");
		goto Cleanup;
	}

	status = SendUrbSync(Dev, urbp);
	if (!NT_SUCCESS(status) || !USBD_SUCCESS(urbp->UrbHeader.Status)) {
		if (NT_SUCCESS(status)) status = urbp->UrbHeader.Status; //possible?
		LOG_ERROR("Failed to call change config: status %X", status);
		goto Cleanup;
	}

	// 4. Copy the configuration into the context

	RtlCopyMemory(Dev->Config.Buffer, tmpBuf, tmpDescriptor->wTotalLength);
	Dev->Config.Descriptor = (PUSB_CONFIGURATION_DESCRIPTOR)Dev->Config.Buffer;
	Dev->Config.Handle = urbp->UrbSelectConfiguration.ConfigurationHandle;
	status = UpdateConfigurationFromInterfaceList(Dev, interfaces);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to fill information");
		goto Cleanup;
	}

	LOG_INFO("Set configuration to %d", Value);
	ActivateChildren(Dev); //activate children on this new config
	goto Cleanup;
Cleanup:
	if (urbp) 
		USBD_UrbFree(Dev->WDM.Handle, urbp);
	if (interfaces)
		ExFreePoolWithTag(interfaces, IU_ALLOC_CONFIG_POOL);
	return status;
}

NTSTATUS SetInterface(
	PIU_DEVICE Dev,
	UCHAR InterfaceNumber,
	UCHAR Altsetting
) {
	NTSTATUS status = STATUS_SUCCESS;

	if (!Dev->Config.Descriptor || !Dev->Config.Handle) {
		LOG_ERROR("Device is not configured, cannot set inferface");
		return STATUS_INVALID_DEVICE_STATE;
	}

	PUSB_INTERFACE_DESCRIPTOR intfDesc = USBD_ParseConfigurationDescriptorEx(
		Dev->Config.Descriptor,
		Dev->Config.Descriptor,
		InterfaceNumber,
		Altsetting,
		-1, -1, -1
	);
	if (!intfDesc) {
		LOG_ERROR("Cannot set nonexistent interface");
		return STATUS_NO_MORE_ENTRIES;
	}

	USBD_INTERFACE_LIST_ENTRY intfSelection; // need dyn alloc for async?
	intfSelection.InterfaceDescriptor = intfDesc;
	intfSelection.Interface = NULL;
	PURB urbp;
	status = USBD_SelectInterfaceUrbAllocateAndBuild(
		Dev->WDM.Handle,
		Dev->Config.Handle,
		&intfSelection,
		&urbp
	);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to build urb. Status = %X", status);
		return STATUS_UNSUCCESSFUL;
	}

	//need free urb from here
	status = SendUrbSync(Dev, urbp);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("could not send urb");
		goto Cleanup;
	}

	status = UpdateInterfaceFromInterfaceEntry(Dev, &intfSelection);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Update interface failed!");
		goto Cleanup;
	}

	LOG_INFO("set interface success");
Cleanup:
	if (urbp) 
		USBD_UrbFree(Dev->WDM.Handle, urbp);
	return status;
}

NTSTATUS UpdateConfigurationFromInterfaceList(
	PIU_DEVICE Dev,
	PUSBD_INTERFACE_LIST_ENTRY InterfaceList //after calling set interface
) {
	NTSTATUS status = STATUS_SUCCESS;
	for (UCHAR i = 0; i < Dev->Config.Descriptor->bNumInterfaces; i++) {
		status = UpdateInterfaceFromInterfaceEntry(Dev, &InterfaceList[i]);
		if (!NT_SUCCESS(status)) {
			LOG_INFO("update configuration failed");
			return status;
		}
	}
	return status;
}

NTSTATUS UpdateInterfaceFromInterfaceEntry(
	PIU_DEVICE Dev,
	PUSBD_INTERFACE_LIST_ENTRY InterfaceEntry
) {
	if (!InterfaceEntry || !InterfaceEntry->Interface) {
		return STATUS_INVALID_PARAMETER;
	}
	PUSBD_INTERFACE_INFORMATION intf = InterfaceEntry->Interface;
	if (intf->NumberOfPipes > IU_MAX_NUMBER_OF_ENDPOINTS_PER_INTERFACE) {
		LOG_ERROR("Too many pipes for one interface");
		return STATUS_BUFFER_TOO_SMALL;
	}
	RtlCopyMemory(
		&Dev->Config.Interfaces[intf->InterfaceNumber],
		intf,
		intf->Length
	);
	return STATUS_SUCCESS;
}
