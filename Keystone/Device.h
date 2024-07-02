#pragma once

#include "public.h"

//Device extension
typedef struct _IU_DEVICE {
	WDFDEVICE Self;
	WDFUSBDEVICE Handle; //this even gonna be used? can't set alternate configurations with it...
	USB_DEVICE_DESCRIPTOR DeviceDescriptor;

	APPLE_CONNECTION_MODE AppleMode;

	struct {
		PDEVICE_OBJECT Self;
		PDEVICE_OBJECT NextStackDevice;
		PDEVICE_OBJECT PhysicalDeviceObject;
		USBD_HANDLE Handle;
		BOOLEAN HandleOpen;
	} WDM;

	struct {
		PUSB_CONFIGURATION_DESCRIPTOR Descriptor;
		USBD_CONFIGURATION_HANDLE Handle;
		UCHAR Value;
		UCHAR Index;
		ULONG TotalSize;
		UCHAR DescriptorBuffer[512];
		BOOLEAN IsConfigured;
	} Config;

} IU_DEVICE, * PIU_DEVICE, DEVICE_CONTEXT, * PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

// Creation
NTSTATUS KeystoneCreateDevice(
	INOUT PWDFDEVICE_INIT DeviceInit
);

// PNP
EVT_WDF_DEVICE_PREPARE_HARDWARE KeystoneEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE KeystoneEvtDeviceReleaseHardware;
// TODO:
EVT_WDF_DEVICE_D0_ENTRY KeystoneEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT KeystoneEvtDeviceD0Exit;

// OTHERS


EXTERN_C_END
