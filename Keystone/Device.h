#pragma once

#include "public.h"

#define IU_MAX_INTERFACE_NUMBER 8
#define IU_MAX_NUMBER_OF_ENDPOINTS_PER_INTERFACE 4
#define IU_MAX_CONFIGURATION_BUFFER_SIZE 512
#define IU_MAX_UDID_LENGTH 60

//Device extension
typedef struct _IU_DEVICE {
	WDFDEVICE Self;
	WCHAR Udid[IU_MAX_UDID_LENGTH];
	WDFDRIVER Driver;
	UCHAR DeviceNum;

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
		struct {
			USHORT Length;
			UCHAR InterfaceNumber;
			UCHAR AlternateSetting;
			UCHAR Class;
			UCHAR Protocol;
			UCHAR _Reserved; //unused, here to match USBD_INTERFACE_INFORMATION
			USBD_INTERFACE_HANDLE InterfaceHandle;
			ULONG NumberOfPipes;
			USBD_PIPE_INFORMATION Pipes[IU_MAX_NUMBER_OF_ENDPOINTS_PER_INTERFACE];
		} Interfaces[IU_MAX_INTERFACE_NUMBER + 1];
	} Config;

	// flags for closing things
	BOOLEAN WDMIsInitialized;
} IU_DEVICE, * PIU_DEVICE, DEVICE_CONTEXT, * PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext);

// Creation
NTSTATUS KeystoneCreateDevice(
	IN WDFDRIVER Driver,
	INOUT PWDFDEVICE_INIT DeviceInit
);

// PNP
EVT_WDF_DEVICE_PREPARE_HARDWARE KeystoneEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE KeystoneEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY KeystoneEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT KeystoneEvtDeviceD0Exit;


EXTERN_C_END
