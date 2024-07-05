#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>
#include <wdfusb.h>
#include <initguid.h>

#include "device.h"


#define IU_ALLOC_CONFIG_POOL 'fnoc'

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD KeystoneEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP KeystoneEvtDriverContextCleanup;
