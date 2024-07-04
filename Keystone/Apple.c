#include "driver.h"
#include "apple.h"
#include "configuration.h"
#include "log.h"
#include "urbsend.h"


NTSTATUS SetAppleMode(
	INOUT PIU_DEVICE Dev,
	IN APPLE_CONNECTION_MODE Mode
) {
	NTSTATUS status = STATUS_SUCCESS;

	LOG_INFO("Trying to set apple device into new mode: %d", Mode);
	APPLE_CONNECTION_MODE curMode = GetAppleMode(Dev);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to get current mode, continuing anyways");
	}
	else if (curMode == Mode) {
		LOG_INFO("Device already set to mode %d", Mode);
		return status;
	}

	URB urb;
	CHAR ret = -1;
	RtlZeroMemory(&urb, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
	urb.UrbHeader.Function = URB_FUNCTION_VENDOR_DEVICE;
	urb.UrbHeader.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
	urb.UrbControlVendorClassRequest.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
	urb.UrbControlVendorClassRequest.TransferBufferLength = 1;
	urb.UrbControlVendorClassRequest.TransferBufferMDL = NULL;
	urb.UrbControlVendorClassRequest.TransferBuffer = &ret;
	urb.UrbControlVendorClassRequest.Request = (UCHAR)APPLE_VEND_SPECIFIC_SET_MODE;
	urb.UrbControlVendorClassRequest.Value = (USHORT)0;
	urb.UrbControlVendorClassRequest.Index = (USHORT)Mode;

	status = SendUrbSync(Dev, &urb);
	if (!NT_SUCCESS(status) || !USBD_SUCCESS(urb.UrbHeader.Status)) {
		LOG_ERROR("Request failed");
		return status;
	}

	LOG_INFO("%d bytes received", urb.UrbControlVendorClassRequest.TransferBufferLength);
	if (ret != 0) {
		LOG_INFO("Unexpected response from device. Is device already in current mode?");
		return STATUS_UNSUCCESSFUL;
	}
	Dev->AppleMode = Mode;
	return status;
}

APPLE_CONNECTION_MODE GetAppleMode(
	IN PIU_DEVICE Dev
) {
	NTSTATUS status = STATUS_SUCCESS;
	APPLE_CONNECTION_MODE out;
	if (Dev->DeviceDescriptor.bNumConfigurations <= 4) {
		out = APPLE_MODE_INITIAL;
		goto Cleanup;
	}
	if (Dev->DeviceDescriptor.bNumConfigurations > 5) {
		out = APPLE_MODE_UNKNOWN;
		goto Cleanup;
	}

	ULONG configLen;
	UCHAR buf[IU_MAX_CONFIGURATION_BUFFER_SIZE];
	status = GetConfigDescriptorByValue(Dev, buf, sizeof(buf), 5, &configLen);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not get configuration descriptor 5 for query apple mode");
		out = APPLE_MODE_UNKNOWN;
		goto Cleanup;
	}
	if (((PUSB_CONFIGURATION_DESCRIPTOR)buf)->wTotalLength > IU_MAX_CONFIGURATION_BUFFER_SIZE) {
		LOG_ERROR("Device config is too big!");
		out = APPLE_MODE_UNKNOWN;
		goto Cleanup;
	}

	PUSB_CONFIGURATION_DESCRIPTOR confDesc = (PUSB_CONFIGURATION_DESCRIPTOR)buf;
	PUSB_INTERFACE_DESCRIPTOR cdcNcm = USBD_ParseConfigurationDescriptorEx(
		confDesc,
		confDesc,
		-1,
		-1,
		USB_DEVICE_CLASS_COMMUNICATIONS,
		USB_DEVICE_SUBCLASS_CDC_NCM,
		-1
	);

	PUSB_INTERFACE_DESCRIPTOR usbMux = USBD_ParseConfigurationDescriptorEx(
		confDesc,
		confDesc,
		-1,
		-1,
		USB_DEVICE_CLASS_VENDOR_SPECIFIC,
		APPLE_USBMUX_SUBCLASS,
		APPLE_USBMUX_PROTOCOL
	);

	PUSB_INTERFACE_DESCRIPTOR valeria = USBD_ParseConfigurationDescriptorEx(
		confDesc,
		confDesc,
		-1,
		-1,
		USB_DEVICE_CLASS_VENDOR_SPECIFIC,
		APPLE_VALERIA_SUBCLASS,
		APPLE_VALERIA_PROTOCOL
	);

	if (valeria && usbMux)
		out = APPLE_MODE_VALERIA;
	else if (cdcNcm && usbMux)
		out = APPLE_MODE_NETWORK;
	else
		out = APPLE_MODE_UNKNOWN;

Cleanup:
	Dev->AppleMode = out;
	LOG_INFO("Device is in apple mode %d", out);
	return out;
}

UCHAR GetDesiredConfigurationFromAppleMode(
	APPLE_CONNECTION_MODE Mode
) {
	switch (Mode) {
	case APPLE_MODE_NETWORK:
	case APPLE_MODE_VALERIA:
		return 5;
	case APPLE_MODE_INITIAL:
		return 4;
	default:
		LOG_INFO("Getting desired configuration for bad mode");
		return 0;
	}
}
