#include "driver.h"
#include "configuration.h"
#include "log.h"
#include "usbdrequest.h"

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

	ntStatus = SendUSBDRequest(Dev, &urb);
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

	if (Dev->DeviceDescriptor.bLength == 0) {
		LOG_ERROR("Cannot get configurations without device descriptor");
		return STATUS_NOT_FOUND;
	}
	if (Size < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
		LOG_ERROR("Buffer size too small to get config descriptor");
		*Received = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	// if invalid index
	if (Index < 0 || Index >= Dev->DeviceDescriptor.bNumConfigurations) {
		LOG_ERROR("Invalid configuration index: OOB");
		*Received = 0;
		return STATUS_NO_MORE_ENTRIES;
	}
	// return cached config if it is active one
	if (Dev->Config.Descriptor && Dev->Config.Index == Index) {
		LOG_INFO("Returning cached config desc");
		Size = Dev->Config.Descriptor->bLength; //only return the 9 bytes - we already have all the information
		RtlCopyMemory(Buffer, Dev->Config.Descriptor, Size);
		*Received = Size;
		return STATUS_SUCCESS;
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

	ntStatus = SendUSBDRequest(Dev, &urb);
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
	OUT PULONG Received,
	OUT PUCHAR Index
) {
	NTSTATUS ntStatus = STATUS_SUCCESS;
	*Index = 0;
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
			*Index = 0;
			*Received = 0;
			return STATUS_NOT_FOUND;
		}
		if (i == Value - 1) continue;
		ntStatus = GetConfigDescriptorByIndex(Dev, Buffer, Size, i, Received);
		*Index = i;
		i++;
	}
	//received length should already be set from last get
	return ntStatus;
}

NTSTATUS UnsetConfiguration(
	INOUT PIU_DEVICE Dev
) {
	NTSTATUS status = STATUS_SUCCESS;
	URB urb;
	RtlZeroMemory(&urb, sizeof(URB));
	urb.UrbHeader.Function = URB_FUNCTION_SELECT_CONFIGURATION;
	urb.UrbHeader.Length = sizeof(struct _URB_SELECT_CONFIGURATION);

	status = SendUSBDRequest(Dev, &urb);
	if (!NT_SUCCESS(status) || !USBD_SUCCESS(urb.UrbHeader.Status)) {
		LOG_ERROR("Resetting configuration failed: status: 0x%x, urb status: 0x%x", status, urb.UrbHeader.Status);
		return status;
	}

	Dev->Config.Descriptor = NULL;
	Dev->Config.Handle = urb.UrbSelectConfiguration.ConfigurationHandle;
	Dev->Config.Value = 0;
	Dev->Config.Index = 0;
	Dev->Config.TotalSize = 0;
	RtlZeroMemory(Dev->Config.DescriptorBuffer, sizeof(Dev->Config.DescriptorBuffer));
	Dev->Config.IsConfigured = FALSE;

	// TODO: clear pipes

	return status;
}

NTSTATUS SetConfigurationByValue(
	INOUT PIU_DEVICE Dev,
	IN UCHAR Value
) {
	NTSTATUS status;
	if (Dev->Config.Value == Value) {
		LOG_INFO("Configuration already set to value");
		return STATUS_SUCCESS;
	}

	if (Value == 0) { //deconfigure
		LOG_INFO("Resetting configuration");
		return UnsetConfiguration(Dev);
	}
	
	ULONG configLen;
	UCHAR configInd;
	UCHAR configBuf[IU_MAX_CONFIGURATION_DESCRIPTOR_LENGTH];
	status = GetConfigDescriptorByValue(
		Dev, 
		configBuf,
		sizeof(configBuf), 
		Value,
		&configLen,
		&configInd
	);
	if (!NT_SUCCESS(status)) {
		LOG_INFO("Getting config descriptor failed");
		return status;
	}
	if (configLen > IU_MAX_CONFIGURATION_DESCRIPTOR_LENGTH) {
		LOG_INFO("Configuration descriptor exceeds max size!");
		return STATUS_BUFFER_TOO_SMALL;
	}

	LOG_INFO("Config len is %d bytes", configLen);

	ULONG numInterfaces = ((PUSB_CONFIGURATION_DESCRIPTOR)configBuf)->bNumInterfaces;
	PUSBD_INTERFACE_LIST_ENTRY interfaces = ExAllocatePoolZero(
		NonPagedPoolNx,
		sizeof(USBD_INTERFACE_LIST_ENTRY) * (numInterfaces + 1),
		'nocc' // [C]hange [Con]figuration
	);
	if (!interfaces) {
		LOG_ERROR("Interface array alloc failed");
		return STATUS_NO_MEMORY;
	}

	PURB urbp = NULL;
	PUSB_INTERFACE_DESCRIPTOR intfDesc;
	PUCHAR startP = configBuf;
	for (ULONG i = 0; i < numInterfaces; i++) {
		intfDesc = USBD_ParseConfigurationDescriptorEx(
			(PUSB_CONFIGURATION_DESCRIPTOR)configBuf,
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

	status = USBD_SelectConfigUrbAllocateAndBuild(
		Dev->WDM.Handle, 
		(PUSB_CONFIGURATION_DESCRIPTOR)configBuf, 
		interfaces, 
		&urbp
	);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to create select config URB");
		goto Cleanup;
	}

	// NOTE: no need to set max transfer size?
	status = SendUSBDRequest(Dev, urbp);
	if (!NT_SUCCESS(status) || !USBD_SUCCESS(urbp->UrbHeader.Status)) {
		LOG_ERROR("Failed to call change config");
		if (NT_SUCCESS(status)) status = urbp->UrbHeader.Status; //possible?
		goto Cleanup;
	}

	// TODO: get pipe info from interfaces

	Dev->Config.Handle = urbp->UrbSelectConfiguration.ConfigurationHandle;
	Dev->Config.TotalSize = configLen;
	Dev->Config.Value = Value;
	Dev->Config.Index = configInd;
	RtlCopyMemory(Dev->Config.DescriptorBuffer, configBuf, configLen);
	Dev->Config.Descriptor = (PUSB_CONFIGURATION_DESCRIPTOR)Dev->Config.DescriptorBuffer;
	Dev->Config.IsConfigured = TRUE;

	LOG_INFO("Set configuration to %d", Value);

Cleanup:
	if (interfaces)
		ExFreePool2(interfaces, 'nocc', 0, 0);
	if (urbp) 
		USBD_UrbFree(Dev->WDM.Handle, urbp);
	return status;
}

