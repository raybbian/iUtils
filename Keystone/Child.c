#include "child.h"
#include "apple.h"
#include "log.h"
#include "childqueue.h"
#include "functions.h"

VOID KeystoneChildListInitialize(
	IN PWDFDEVICE_INIT DeviceInit
) {
	WDF_CHILD_LIST_CONFIG childListConfig;
	WDF_CHILD_LIST_CONFIG_INIT(
		&childListConfig,
		sizeof(IU_CHILD_IDENTIFIER),
		KeystoneEvtChildListCreateDevice
	);
	childListConfig.EvtChildListScanForChildren = KeystoneEvtChildListScanForChildren;
	childListConfig.EvtChildListIdentificationDescriptionCompare =
		KeystoneEvtWdfChildListIdentificationDescriptionCompare;
	WdfFdoInitSetDefaultChildListConfig(
		DeviceInit,
		&childListConfig,
		WDF_NO_OBJECT_ATTRIBUTES
	);
	return;
}

VOID KeystoneEvtChildListScanForChildren(
	IN WDFCHILDLIST ChildList
) {
	WDFDEVICE ParentDevice = WdfChildListGetDevice(ChildList);
	PIU_DEVICE Dev = DeviceGetContext(ParentDevice);

	LOG_INFO("Scanning for new children");

	if (Dev->AppleMode == APPLE_MODE_UNKNOWN) {
		LOG_INFO("Cannot scan for child devices with unknown apple mode");
		return;
	}
	if (Dev->Config.Descriptor->bConfigurationValue != GetDesiredConfigurationFromAppleMode(Dev->AppleMode)) {
		LOG_INFO("Device not in correct configuration for this apple mode");
		return;
	}

	WdfChildListBeginScan(ChildList);

	ActivatePTPFunction(Dev, ChildList);
	ActivateUsbMuxFunction(Dev, ChildList); 
	if (Dev->AppleMode == APPLE_MODE_NETWORK) {
		ActivateCdcNcmFunction(Dev, ChildList);
	}
	//TODO: others

	WdfChildListEndScan(ChildList);

	LOG_INFO("Scan ended");
}

NTSTATUS KeystoneEvtChildListCreateDevice(
	IN WDFCHILDLIST ChildList,
	IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
	IN PWDFDEVICE_INIT ChildInit
) {
	NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(ChildList);

	// ensure that pdo stack size is parent stack size + 1
	// allows us to send requests directly to next device under fdo
	WdfPdoInitAllowForwardingRequestToParent(ChildInit);

	WDFDEVICE DeviceObject = WdfChildListGetDevice(ChildList);
	PIU_DEVICE Dev = DeviceGetContext(DeviceObject);
	PIU_CHILD_IDENTIFIER ChildId = (PIU_CHILD_IDENTIFIER)IdentificationDescription;

	status = WdfPdoInitAssignDeviceID(ChildInit, &ChildId->HardwareId);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to set device id");
		return status;
	}
	status = WdfPdoInitAddHardwareID(ChildInit, &ChildId->HardwareId);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to add hw id");
		return status;
	}
	for (UCHAR i = 0; i < ChildId->NumberOfCompatibleIds; i++) {
		status = WdfPdoInitAddCompatibleID(ChildInit, &ChildId->CompatibleIds[i]);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Failed to add compat id");
			return status;
		}
	}

	//TODO: device instance id
	//TODO: device description
	//TODO: device text, locale

	//TODO: child context
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, CHILD_DEVICE_CONTEXT);
	WDFDEVICE childDevice;
	status = WdfDeviceCreate(&ChildInit, &deviceAttributes, &childDevice);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to create wdf CHILD device");
		return status;
	}
	PIU_CHILD_DEVICE ChildDev = ChildDeviceGetContext(childDevice);
	RtlZeroMemory(ChildDev, sizeof(IU_CHILD_DEVICE));
	ChildDev->Parent = DeviceGetContext(WdfChildListGetDevice(ChildList));

	ULONG ret;
	status = ExtractChildDeviceDescriptor(
		Dev,
		ChildId,
		&ChildDev->DeviceDescriptor,
		sizeof(ChildDev->DeviceDescriptor),
		&ret
	);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not extract child dev desc");
		return status;
	}

	ULONG configLen;
	status = ExtractChildConfigurationDescriptor(Dev, ChildId, ChildDev->Config.Buffer, sizeof(ChildDev->Config.Buffer), &configLen);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not extract child dev config");
		return status;
	}
	if (((PUSB_CONFIGURATION_DESCRIPTOR)ChildDev->Config.Buffer)->wTotalLength > IU_MAX_CONFIGURATION_BUFFER_SIZE) {
		LOG_ERROR("Device config is too big!");
		return STATUS_BUFFER_TOO_SMALL;
	}
	ChildDev->Config.Descriptor = (PUSB_CONFIGURATION_DESCRIPTOR)ChildDev->Config.Buffer;

	status = KeystoneChildQueueInitialize(childDevice);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to initialize queue for child");
		return status;
	}

	LOG_INFO("Created child device, interface: %d, cur_config %d", ChildId->FunctionType, ChildId->CurrentParentConfig);
	return status;
}

BOOLEAN KeystoneEvtWdfChildListIdentificationDescriptionCompare(
	IN WDFCHILDLIST ChildList,
	IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER FirstIdentificationDescription,
	IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SecondIdentificationDescription
) {
	UNREFERENCED_PARAMETER(ChildList);
	PIU_CHILD_IDENTIFIER First = (PIU_CHILD_IDENTIFIER)FirstIdentificationDescription;
	PIU_CHILD_IDENTIFIER Second = (PIU_CHILD_IDENTIFIER)SecondIdentificationDescription;

	return First->FunctionType == Second->FunctionType && First->CurrentParentConfig == Second->CurrentParentConfig;
}

NTSTATUS ExtractChildDeviceDescriptor(
	IN PIU_DEVICE Dev,
	IN PIU_CHILD_IDENTIFIER ChildId,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG Received
) {
	UNREFERENCED_PARAMETER(ChildId);
	if (BufferSize < sizeof(USB_DEVICE_DESCRIPTOR)) {
		return STATUS_BUFFER_TOO_SMALL;
	}
	ULONG writeSize = sizeof(USB_DEVICE_DESCRIPTOR);
	RtlCopyMemory(
		Buffer,
		&Dev->DeviceDescriptor,
		writeSize
	);
	*Received = writeSize;
	//make this device only have one configuration, all other fields stay same
	((PUSB_DEVICE_DESCRIPTOR)Buffer)->bNumConfigurations = 1;

	//for (ULONG i = 0; i < *Received; i++) {
	//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%02X ", ((PUCHAR)Buffer)[i]);
	//}
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "\n");

	return STATUS_SUCCESS;
}

NTSTATUS ExtractChildConfigurationDescriptor(
	IN PIU_DEVICE Dev,
	IN PIU_CHILD_IDENTIFIER ChildId,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG Received
) {
	if (BufferSize < sizeof(USB_CONFIGURATION_DESCRIPTOR)) { //needs to at least fit this
		return STATUS_BUFFER_TOO_SMALL;
	}
	*Received = 0;
	PUCHAR writePointer = Buffer;

	// copy the configuration descriptor over
	ULONG writeSize = sizeof(USB_CONFIGURATION_DESCRIPTOR);
	RtlCopyMemory(writePointer, Dev->Config.Descriptor, writeSize);
	writePointer += writeSize;
	BufferSize -= writeSize;
	*Received += writeSize;

	// copy interfaces over
	PUCHAR endDesc = (PUCHAR)Dev->Config.Descriptor;
	for (UCHAR i = 0; i < ChildId->NumberOfInterfaces; i++) {
		PUCHAR stDesc = (PUCHAR)USBD_ParseConfigurationDescriptorEx(
			Dev->Config.Descriptor,
			endDesc, //after reading interface, make sure that it is skipped so this doesn't infinite loop
			ChildId->InterfacesUsed[i],
			0, //altsetting
			-1, -1, -1
		);
		if (!stDesc) {
			LOG_ERROR("Mismatch num interfaces when cerating child");
			return STATUS_BAD_DATA;
		}
		//get the next interface descriptor in the configuration buffer
		//from stDesc until any next interface should be presented as the entire interface descriptor including altsettings
		endDesc = (PUCHAR)USBD_ParseConfigurationDescriptorEx(
			Dev->Config.Descriptor,
			stDesc + ((PUSB_INTERFACE_DESCRIPTOR)stDesc)->bLength,
			-1, 
			0, //altsetting
			-1, -1, -1
		);
		if (endDesc == NULL) {
			//if this was the last interface, point to the end of the configuration
			endDesc = (PUCHAR)Dev->Config.Descriptor + Dev->Config.Descriptor->wTotalLength;
		}
		//copy the interface with all its endpoints
		ULONG descLength = (ULONG)(endDesc - stDesc);
		writeSize = min(descLength, BufferSize);
		RtlCopyMemory(writePointer, stDesc, writeSize);
		writePointer += writeSize;
		BufferSize -= writeSize;
		*Received += writeSize;
	}
	PUSB_CONFIGURATION_DESCRIPTOR out = (PUSB_CONFIGURATION_DESCRIPTOR)Buffer;
	out->wTotalLength = (USHORT)*Received;
	out->bNumInterfaces = ChildId->NumberOfInterfaces;

	//debug
	//for (ULONG i = 0; i < *Received; i++) {
	//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%02X ", ((PUCHAR)Buffer)[i]);
	//}
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "\n");

	return STATUS_SUCCESS;
}
