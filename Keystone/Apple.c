#include "driver.h"
#include "apple.h"
#include "configuration.h"
#include "log.h"
#include "urbsend.h"
#include "functions.h"

NTSTATUS SetAppleMode(
	INOUT PIU_DEVICE Dev,
	IN APPLE_CONNECTION_MODE Mode
) {
	NTSTATUS status = STATUS_SUCCESS;
	if (Mode < 1 || Mode > 4) {
		LOG_ERROR("trying to set invalid mode: %d", Mode);
		return STATUS_INVALID_PARAMETER;
	}

	LOG_INFO("Trying to set apple device into new mode: %d", Mode);
	APPLE_CONNECTION_MODE curMode = GetAppleMode(Dev);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to get current mode");
		return STATUS_UNSUCCESSFUL;
	}
	LOG_INFO("Cur mode is %d", curMode);

	if (curMode == Mode) {
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
		LOG_INFO("Unexpected response from device.");
		return STATUS_UNSUCCESSFUL;
	}
	Dev->AppleMode = Mode;
	return status;
}

APPLE_CONNECTION_MODE GetAppleMode(
	IN PIU_DEVICE Dev
) {
	NTSTATUS status = STATUS_SUCCESS;
	APPLE_CONNECTION_MODE out = APPLE_MODE_UNKNOWN;

	if (Dev->DeviceDescriptor.bNumConfigurations < 4) {
		goto Cleanup;
	}
	if (Dev->DeviceDescriptor.bNumConfigurations == 4) {
		out = APPLE_MODE_BASE;
		goto Cleanup;
	}
	if (Dev->DeviceDescriptor.bNumConfigurations == 7) {
		out = APPLE_MODE_BASE_NETWORK_TETHER_VALERIA;
		goto Cleanup;
	}

	ULONG configLen;
	UCHAR buf[IU_MAX_CONFIGURATION_BUFFER_SIZE];
	//get largest configurtaion descriptor (its value matches number of configs)
	status = GetConfigDescriptorByValue(Dev, buf, sizeof(buf), Dev->DeviceDescriptor.bNumConfigurations, &configLen);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Could not get configuration descriptor 5 for query apple mode");
		goto Cleanup;
	}
	if (((PUSB_CONFIGURATION_DESCRIPTOR)buf)->wTotalLength > IU_MAX_CONFIGURATION_BUFFER_SIZE) {
		LOG_ERROR("Device config is too big!");
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

	PUSB_INTERFACE_DESCRIPTOR tether = USBD_ParseConfigurationDescriptorEx(
		confDesc,
		confDesc,
		-1,
		-1,
		USB_DEVICE_CLASS_VENDOR_SPECIFIC,
		APPLE_TETHER_SUBCLASS,
		APPLE_TETHER_PROTOCOL
	);

	switch (Dev->DeviceDescriptor.bNumConfigurations) {
	case 5: 
		if (valeria && usbMux) {
			out = APPLE_MODE_BASE_VALERIA;
			goto Cleanup;
		}
		if (cdcNcm && usbMux) {
			out = APPLE_MODE_BASE_NETWORK;
			goto Cleanup;
		}
		break;
	case 6: 
		if (valeria && usbMux) {
			out = APPLE_MODE_BASE_NETWORK_VALERIA;
			goto Cleanup;
		}
		if (tether && cdcNcm && usbMux) {
			out = APPLE_MODE_BASE_NETWORK_TETHER;
			goto Cleanup;
		}
	}

Cleanup:
	Dev->AppleMode = out;
	LOG_INFO("Device is in apple mode %d", out);
	return out;
}

UCHAR BestConfigurationForMode(APPLE_CONNECTION_MODE Mode) {
	switch (Mode) {
	case APPLE_MODE_BASE_NETWORK_TETHER_VALERIA:
	case APPLE_MODE_BASE_NETWORK_TETHER:
		return 6;
	case APPLE_MODE_BASE_NETWORK_VALERIA:
	case APPLE_MODE_BASE_NETWORK:
	case APPLE_MODE_BASE_VALERIA:
		return 5;
	case APPLE_MODE_BASE:
		return 4;
	default: 
		return 1;
	}
}

const unsigned char APPLE_MODE_CAPABILITIES[IU_NUMBER_OF_APPLE_MODES][IU_MAX_NUMBER_OF_CONFIGURATIONS + 1][IU_NUMBER_OF_FEATURES] = {
	{ // APPLE_MODE_UNKNOWN
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
	},
	{ // APPLE_MODE_BASE
		{0, 0, 0, 0, 0, 0}, 
		{1, 0, 0, 0, 0, 0}, //PTP
		{0, 1, 0, 0, 0, 0}, //Audio
		{1, 0, 1, 0, 0, 0}, //PTP + USBMUX
		{1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
	},
	{ // APPLE_MODE_BASE_VALERIA
		{0, 0, 0, 0, 0, 0}, 
		{1, 0, 0, 0, 0, 0}, //PTP
		{0, 1, 0, 0, 0, 0}, //Audio
		{1, 0, 1, 0, 0, 0}, //PTP + USBMUX
		{1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
		{1, 0, 1, 0, 0, 1}, //PTP + USBMUX + VALERIA
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
	},
	{ // APPLE_MODE_BASE_NETWORK
		{0, 0, 0, 0, 0, 0}, 
		{1, 0, 0, 0, 0, 0}, //PTP
		{0, 1, 0, 0, 0, 0}, //Audio
		{1, 0, 1, 0, 0, 0}, //PTP + USBMUX
		{1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
		{1, 0, 1, 1, 0, 0}, //PTP + USBMUX + NETWORK
		{0, 0, 0, 0, 0, 0}, 
		{0, 0, 0, 0, 0, 0}, 
	},
	{ // APPLE_MODE_BASE_NETWORK_TETHER
		{0, 0, 0, 0, 0, 0}, 
		{1, 0, 0, 0, 0, 0}, //PTP
		{0, 1, 0, 0, 0, 0}, //Audio
		{1, 0, 1, 0, 0, 0}, //PTP + USBMUX
		{1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
		{1, 0, 1, 1, 0, 0}, //PTP + USBMUX + NETWORK
		{1, 0, 1, 1, 1, 0}, //PTP + USBMUX + NETWORK + TETHER
		{0, 0, 0, 0, 0, 0}, 
	},
	{ // APPLE_MODE_BASE_NETWORK_VALERIA
		{0, 0, 0, 0, 0, 0}, 
		{1, 0, 0, 0, 0, 0}, //PTP
		{0, 1, 0, 0, 0, 0}, //Audio
		{1, 0, 1, 0, 0, 0}, //PTP + USBMUX
		{1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
		{1, 0, 1, 1, 0, 0}, //PTP + USBMUX + NETWORK
		{1, 0, 1, 0, 0, 1}, //PTP + USBMUX + VALERIA
		{0, 0, 0, 0, 0, 0}, 
	},
	{ // APPLE_MODE_BASE_NETWORK_TETHER_VALERIA
		{0, 0, 0, 0, 0, 0}, 
		{1, 0, 0, 0, 0, 0}, //PTP
		{0, 1, 0, 0, 0, 0}, //Audio
		{1, 0, 1, 0, 0, 0}, //PTP + USBMUX
		{1, 0, 1, 0, 1, 0}, //PTP + USBMUX + TETHER
		{1, 0, 1, 1, 0, 0}, //PTP + USBMUX + NETWORK
		{1, 0, 1, 1, 1, 0}, //PTP + USBMUX + NETWORK + TETHER
		{1, 0, 1, 0, 0, 1}, //PTP + USBMUX + VALERIA
	},
};

