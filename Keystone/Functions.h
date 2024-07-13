#pragma once

#include "driver.h"

//for ptp interface
#define USB_DEVICE_SUBCLASS_STILL_IMAGE_CAPTURE 0x01
#define USB_DEVICE_PROTOCOL_PTP 0x01
//usbmux interface
#define APPLE_USBMUX_SUBCLASS 0xFE
#define APPLE_USBMUX_PROTOCOL 0x02
//valeria
#define APPLE_VALERIA_SUBCLASS 0x2A
#define APPLE_VALERIA_PROTOCOL 0xFF
//for network interface
#define USB_DEVICE_SUBCLASS_CDC_NCM 0x0D
#define USB_DEVICE_SUBCLASS_CDC_DATA 0x00

#define APPLE_TETHER_SUBCLASS 0xFD
#define APPLE_TETHER_PROTOCOL 0x01

NTSTATUS ActivatePTPFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
);

NTSTATUS ActivateAudioFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
);

NTSTATUS ActivateUsbMuxFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
);

NTSTATUS ActivateCdcNcmFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
);

NTSTATUS ActivateTetherFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
);

NTSTATUS ActivateValeriaFunction(
	PIU_DEVICE Dev,
	WDFCHILDLIST ChildList
);
