#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>
#include <wdfusb.h>
#include <wdfdevice.h>
#include <initguid.h>
#include "public.h"

#include "device.h"

#define IU_ALLOC_CONFIG_POOL 'fnoc'

typedef enum _IU_DEVICE_STATE {
	IU_DEVICE_DISCONNECTED,
	IU_DEVICE_OPERATIONAL,
	IU_DEVICE_AWAITING_RECONNECT,
	IU_DEVICE_AWAITING_RECONNECT_PENDING_VALERIA,
} IU_DEVICE_STATE, *PIU_DEVICE_STATE;

typedef struct _IU_DEVICE_STORE {
	struct {
		WCHAR Udid[IU_MAX_UDID_LENGTH];
		APPLE_CONNECTION_MODE DesiredMode;
	} Devices[IU_MAX_NUMBER_OF_DEVICES];
} IU_DEVICE_STORE, *PIU_DEVICE_STORE, DRIVER_CONTEXT, *PDRIVER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_CONTEXT, DriverGetContext);

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD KeystoneEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP KeystoneEvtDriverContextCleanup;
