#pragma once

#include "public.h"

#define IU_MAX_NUMBER_OF_ENDPOINTS 16
#define IU_MAX_NUMBER_OF_INTERFACES 16
#define IU_MAX_CONFIGURATION_BUFFER_SIZE 512

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
	} WDM;

	struct {
		UCHAR Buffer[IU_MAX_CONFIGURATION_BUFFER_SIZE];
		PUSB_CONFIGURATION_DESCRIPTOR Descriptor;
		USBD_CONFIGURATION_HANDLE Handle;

		//use UpdateConfigurationFromInterfaceList and UpdateInterfaceFromInterfaceEntry for below
		UCHAR InterfaceAltsetting[IU_MAX_NUMBER_OF_INTERFACES + 1];
		USBD_INTERFACE_HANDLE InterfaceHandles[IU_MAX_NUMBER_OF_INTERFACES + 1]; //store by num
		USBD_PIPE_INFORMATION Pipes[IU_MAX_NUMBER_OF_ENDPOINTS + 1]; //pipe info could change
	} Config;

	// flags for closing things
	BOOLEAN WDMIsInitialized;
} IU_DEVICE, * PIU_DEVICE, DEVICE_CONTEXT, * PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext);

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


EXTERN_C_END
