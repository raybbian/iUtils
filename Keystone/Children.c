#include "children.h"
#include "apple.h"
#include "log.h"

VOID KeystoneEvtChildListScanForChildren(
	IN WDFCHILDLIST ChildList
) {
	NTSTATUS status;

	WDFDEVICE ParentDevice = WdfChildListGetDevice(ChildList);
	PIU_DEVICE Dev = DeviceGetContext(ParentDevice);

	LOG_INFO("Scanning for new children");

	if (Dev->AppleMode == APPLE_MODE_UNKNOWN) {
		LOG_INFO("Cannot scan for child devices with unknown apple mode");
		return;
	}
	if (Dev->Config.Value != GetDesiredConfigurationFromAppleMode(Dev->AppleMode)) {
		LOG_INFO("Device not in correct configuration for this apple mode");
		return;
	}

	WdfChildListBeginScan(ChildList);

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
		LOG_INFO("Failed to get interface for apple PTP, continuing");
	}
	else {
		IU_CHILD_DEVICE_ID ptpId;
		RtlZeroMemory(&ptpId, sizeof(ptpId)); //ids are compared byte by byte (no callback is supplied to compare), so need to make sure zero for consistency
		WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
			(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&ptpId,
			sizeof(IU_CHILD_DEVICE_ID)
		);
		ptpId.CurrentParentConfig = Dev->Config.Value;
		ptpId.InterfaceType = APPLE_INTERFACE_PTP;
		ptpId.NumberOfInterfaces = 1;
		ptpId.InterfaceDescriptorList[0] = ptpDesc;
		status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
			ChildList,
			(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&ptpId,
			NULL
		);
		if (status == STATUS_OBJECT_NAME_EXISTS) {
			LOG_INFO("Updating ptp instead");
		}
		else if (!NT_SUCCESS(status)) {
			LOG_ERROR("Could not add ptp");
		}
		else {
			LOG_INFO("Activated ptp");
		}
	}

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
		LOG_INFO("Failed to get interface for apple usbmux, continuing");
	}
	else {
		IU_CHILD_DEVICE_ID usbmuxId;
		RtlZeroMemory(&usbmuxId, sizeof(usbmuxId));
		WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
			(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&usbmuxId,
			sizeof(IU_CHILD_DEVICE_ID)
		);
		usbmuxId.CurrentParentConfig = Dev->Config.Value;
		usbmuxId.InterfaceType = APPLE_INTERFACE_USB_MUX;
		usbmuxId.NumberOfInterfaces = 1;
		usbmuxId.InterfaceDescriptorList[0] = usbmuxDesc;
		status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
			ChildList,
			(PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER)&usbmuxId,
			NULL
		);
		if (status == STATUS_OBJECT_NAME_EXISTS) {
			LOG_INFO("Updating usbmux instead");
		}
		else if (!NT_SUCCESS(status)) {
			LOG_ERROR("Could not add usbmux");
		}
		else {
			LOG_INFO("Activated usbmux");
		}
	}

	//TODO: init other two interfaces if allowed

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

	//WDFDEVICE DeviceObject = WdfChildListGetDevice(ChildList);
	//PIU_DEVICE Dev = DeviceGetContext(DeviceObject);
	PIU_CHILD_DEVICE_ID ChildId = (PIU_CHILD_DEVICE_ID)IdentificationDescription;

	ULONG numCompatIds = 0;
	CONST WCHAR* deviceId;
	CONST PWCHAR* compatIds;
	switch (ChildId->InterfaceType) {
	case APPLE_INTERFACE_PTP:
		numCompatIds = sizeof(APPLE_PTP_COMPAT_IDS) / sizeof(*APPLE_PTP_COMPAT_IDS);
		deviceId = APPLE_PTP_DEVICE_ID;
		compatIds = APPLE_PTP_COMPAT_IDS;
		break;
	case APPLE_INTERFACE_USB_MUX:
		numCompatIds = sizeof(APPLE_USBMUX_COMPAT_IDS) / sizeof(*APPLE_USBMUX_COMPAT_IDS);
		deviceId = APPLE_USBMUX_DEVICE_ID;
		compatIds = APPLE_USBMUX_COMPAT_IDS;
		break;
	default:
		LOG_ERROR("Interface type not supported yet");
		return STATUS_UNSUCCESSFUL;
	}

	UNICODE_STRING tmp;
	RtlInitUnicodeString(&tmp, deviceId);
	status = WdfPdoInitAssignDeviceID(ChildInit, &tmp);
	if (!NT_SUCCESS(status)) 
		LOG_INFO("Failed to set device id");

	for (ULONG i = 0; i < numCompatIds; i++) {
		RtlInitUnicodeString(&tmp, compatIds[i]);
		WdfPdoInitAddCompatibleID(ChildInit, &tmp);
		if (!NT_SUCCESS(status))
			LOG_INFO("Failed to add compat id");
	}
	//TODO: other PDO properties
	//TODO: child context?
	WDFDEVICE childDevice;
	status = WdfDeviceCreate(&ChildInit, WDF_NO_OBJECT_ATTRIBUTES, &childDevice);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to create wdf CHILD device");
		return status;
	}

	LOG_INFO("Created child device");
	return status;
}

