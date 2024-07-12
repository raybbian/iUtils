#include "functions.h"
#include "child.h"
#include "log.h"

//these functions may need to be updated if apple changes things

// to be called during child list scan
NTSTATUS ActivatePTPFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
) {
	NTSTATUS status = STATUS_SUCCESS;
	//Init PnP Device
	PUSB_INTERFACE_DESCRIPTOR ptpDesc = USBD_ParseConfigurationDescriptorEx(
		Dev->Config.Descriptor,
		Dev->Config.Descriptor,
		-1,
		-1,
		USB_DEVICE_CLASS_IMAGE,
		USB_DEVICE_SUBCLASS_STILL_IMAGE_CAPTURE,
		USB_DEVICE_PROTOCOL_PTP
	);
	if (!ptpDesc) {
		LOG_INFO("Failed to get interface for apple PTP");
		return STATUS_NOT_SUPPORTED;
	}

	IU_CHILD_IDENTIFIER ptpId;
	RtlZeroMemory(&ptpId, sizeof(ptpId)); //ids are compared byte by byte (no callback is supplied to compare), so need to make sure zero for consistency
	WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
		(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&ptpId,
		sizeof(IU_CHILD_IDENTIFIER)
	);
	ptpId.FunctionType = APPLE_FUNCTION_PTP;
	ptpId.CurrentParentMode = Dev->AppleMode;
	ptpId.CurrentParentConfig = Dev->Config.Descriptor->bConfigurationValue;
	ptpId.NumberOfInterfaces = 1;
	ptpId.NumberOfCompatibleIds = 6;
	ptpId.InterfacesUsed[0] = ptpDesc->bInterfaceNumber;
	RtlInitUnicodeString(&ptpId.HardwareId, L"USB\\VID_05AC&PID_12AB&MI_00");
	RtlInitUnicodeString(&ptpId.CompatibleIds[0], L"USB\\COMPAT_VID_05ac&Class_06&SubClass_01&Prot_01");
	RtlInitUnicodeString(&ptpId.CompatibleIds[1], L"USB\\COMPAT_VID_05ac&Class_06&SubClass_01");
	RtlInitUnicodeString(&ptpId.CompatibleIds[2], L"USB\\COMPAT_VID_05ac&Class_06");
	RtlInitUnicodeString(&ptpId.CompatibleIds[3], L"USB\\Class_06&SubClass_01&Prot_01");
	RtlInitUnicodeString(&ptpId.CompatibleIds[4], L"USB\\Class_06&SubClass_01");
	RtlInitUnicodeString(&ptpId.CompatibleIds[5], L"USB\\Class_06");
	RtlInitUnicodeString(&ptpId.FunctionalDescription, L"PTP");

	
	status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
		ChildList,
		(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&ptpId,
		NULL
	);
	if (status == STATUS_OBJECT_NAME_EXISTS) {
		LOG_INFO("Updating ptp instead");
		status = STATUS_SUCCESS;
	}
	else if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not add ptp");
	}
	else {
		LOG_INFO("Activated ptp");
	}
	return status;
}

NTSTATUS ActivateUsbMuxFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
) {
	NTSTATUS status = STATUS_SUCCESS;
	//init UsbMux Device
	PUSB_INTERFACE_DESCRIPTOR usbmuxDesc = USBD_ParseConfigurationDescriptorEx(
		Dev->Config.Descriptor,
		Dev->Config.Descriptor,
		-1,
		-1,
		USB_DEVICE_CLASS_VENDOR_SPECIFIC,
		APPLE_USBMUX_SUBCLASS,
		APPLE_USBMUX_PROTOCOL
	);
	if (!usbmuxDesc) {
		LOG_INFO("Failed to get interface for apple usbmux");
		return STATUS_NOT_SUPPORTED;
	}

	IU_CHILD_IDENTIFIER usbmuxId;
	RtlZeroMemory(&usbmuxId, sizeof(usbmuxId));
	WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
		(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&usbmuxId,
		sizeof(IU_CHILD_IDENTIFIER)
	);
	usbmuxId.FunctionType = APPLE_FUNCTION_USB_MUX;
	usbmuxId.CurrentParentMode = Dev->AppleMode;
	usbmuxId.CurrentParentConfig = Dev->Config.Descriptor->bConfigurationValue;
	usbmuxId.NumberOfInterfaces = 1;
	usbmuxId.NumberOfCompatibleIds = 6;
	usbmuxId.InterfacesUsed[0] = usbmuxDesc->bInterfaceNumber;
	RtlInitUnicodeString(&usbmuxId.HardwareId, L"USB\\VID_05AC&PID_12AB&MI_01");
	RtlInitUnicodeString(&usbmuxId.CompatibleIds[0], L"USB\\COMPAT_VID_05ac&Class_ff&SubClass_fe&Prot_02");
	RtlInitUnicodeString(&usbmuxId.CompatibleIds[1], L"USB\\COMPAT_VID_05ac&Class_ff&SubClass_fe");
	RtlInitUnicodeString(&usbmuxId.CompatibleIds[2], L"USB\\COMPAT_VID_05ac&Class_ff");
	RtlInitUnicodeString(&usbmuxId.CompatibleIds[3], L"USB\\Class_ff&SubClass_fe&Prot_02");
	RtlInitUnicodeString(&usbmuxId.CompatibleIds[4], L"USB\\Class_ff&SubClass_fe");
	RtlInitUnicodeString(&usbmuxId.CompatibleIds[5], L"USB\\Class_ff");
	RtlInitUnicodeString(&usbmuxId.FunctionalDescription, L"Apple USBMux");

	status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
		ChildList,
		(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&usbmuxId,
		NULL
	);
	if (status == STATUS_OBJECT_NAME_EXISTS) {
		LOG_INFO("Updating usbmux instead");
		status = STATUS_SUCCESS;
	}
	else if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not add usbmux");
	}
	else {
		LOG_INFO("Activated usbmux");
	}
	return status;
}

#define CDC_CLASS_DESCRIPTOR_TYPE 36 //(USB_CLASS_TYPE | USB_DESCRIPTOR_TYPE)
#define CDC_CLASS_UNION_TYPE 6

NTSTATUS ActivateCdcNcmFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
) {
	NTSTATUS status = STATUS_SUCCESS;

	PUSB_INTERFACE_DESCRIPTOR cdcControlDesc = USBD_ParseConfigurationDescriptorEx(
		Dev->Config.Descriptor,
		Dev->Config.Descriptor,
		-1,
		-1,
		USB_DEVICE_CLASS_COMMUNICATIONS,
		USB_DEVICE_SUBCLASS_CDC_NCM,
		-1
	);
	if (!cdcControlDesc) {
		LOG_INFO("Failed to get interface for cdc control desc");
		return STATUS_NOT_SUPPORTED;
	}

	//search for associated data descriptor
	PUSB_COMMON_DESCRIPTOR desc = NULL;
	SHORT dataInterfaceNum = -1;
	PUCHAR startP = (PUCHAR)cdcControlDesc;
	while ((desc = USBD_ParseDescriptors(
		Dev->Config.Descriptor,
		Dev->Config.Descriptor->wTotalLength,
		startP,
		CDC_CLASS_DESCRIPTOR_TYPE
	)) != NULL) {
		PUCHAR tmp = (PUCHAR)desc;
		if (tmp[2] == CDC_CLASS_UNION_TYPE) {
			if (tmp[3] != cdcControlDesc->bInterfaceNumber) {
				LOG_ERROR("Found union descriptor is not for found cdc control descriptor");
				return STATUS_UNSUCCESSFUL;
			}
			dataInterfaceNum = tmp[4]; //subordinate interface
			LOG_INFO("Found data interface for cdc ncm");
			break;
		}
		startP = (PUCHAR)desc + desc->bLength;
	}
	
	if (dataInterfaceNum < 0) {
		LOG_ERROR("could not find data interface num");
		return STATUS_UNSUCCESSFUL;
	}
	

	IU_CHILD_IDENTIFIER cdcNcmId;
	RtlZeroMemory(&cdcNcmId, sizeof(cdcNcmId));
	WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
		(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&cdcNcmId,
		sizeof(IU_CHILD_IDENTIFIER)
	);
	cdcNcmId.FunctionType = APPLE_FUNCTION_NETWORK;
	cdcNcmId.CurrentParentConfig = Dev->Config.Descriptor->bConfigurationValue;
	cdcNcmId.CurrentParentMode = Dev->AppleMode;
	cdcNcmId.NumberOfInterfaces = 2;
	cdcNcmId.NumberOfCompatibleIds = 6;
	cdcNcmId.InterfacesUsed[0] = cdcControlDesc->bInterfaceNumber;
	cdcNcmId.InterfacesUsed[1] = (UCHAR)dataInterfaceNum;
	RtlInitUnicodeString(&cdcNcmId.HardwareId, L"USB\\VID_05AC&PID_12AB&MI_99");
	RtlInitUnicodeString(&cdcNcmId.CompatibleIds[0], L"USB\\COMPAT_VID_05ac&Class_02&SubClass_0D&Prot_00");
	RtlInitUnicodeString(&cdcNcmId.CompatibleIds[1], L"USB\\COMPAT_VID_05ac&Class_02&SubClass_0D");
	RtlInitUnicodeString(&cdcNcmId.CompatibleIds[2], L"USB\\COMPAT_VID_05ac&Class_02");
	RtlInitUnicodeString(&cdcNcmId.CompatibleIds[3], L"USB\\Class_02&SubClass_0D&Prot_00");
	RtlInitUnicodeString(&cdcNcmId.CompatibleIds[4], L"USB\\Class_02&SubClass_0D");
	RtlInitUnicodeString(&cdcNcmId.CompatibleIds[5], L"USB\\Class_02");
	RtlInitUnicodeString(&cdcNcmId.FunctionalDescription, L"CDC NCM Device");

	status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
		ChildList,
		(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&cdcNcmId,
		NULL
	);
	if (status == STATUS_OBJECT_NAME_EXISTS) {
		LOG_INFO("Updating cdc ncm instead");
		status = STATUS_SUCCESS;
	}
	else if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not add cdc ncm");
	}
	else {
		LOG_INFO("Activated cdc ncm");
	}
	return status;
}
