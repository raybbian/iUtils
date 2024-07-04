//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_Keystone,
    0xf6390607,0x789d,0x4764,0xb5,0xa7,0x05,0x8e,0xfb,0x8f,0xdc,0x5d);
// {f6390607-789d-4764-b5a7-058efb8fdc5d}

#undef IN
#define IN _In_
#undef OUT
#define OUT _Out_
#define INOUT _Inout_
#define INOPT _In_opt_

//TODO: maybe programatically generate?
//for ptp interface
#define USB_DEVICE_SUBCLASS_STILL_IMAGE_CAPTURE 1
#define USB_DEVICE_PROTOCOL_PTP 1
//usbmux interface
#define APPLE_USBMUX_SUBCLASS 254
#define APPLE_USBMUX_PROTOCOL 2
//valeria
#define APPLE_VALERIA_SUBCLASS 42
#define APPLE_VALERIA_PROTOCOL 255
//for network interface
#define USB_DEVICE_SUBCLASS_CDC_NCM 0xD
#define USB_DEVICE_SUBCLASS_CDC_DATA 0

typedef enum {
	APPLE_MODE_UNKNOWN = 0,
	APPLE_MODE_INITIAL = 1,
	APPLE_MODE_VALERIA = 2,
	APPLE_MODE_NETWORK = 3
} APPLE_CONNECTION_MODE, * PAPPLE_CONNECTION_MODE;

typedef enum {
	APPLE_FUNCTION_UNKNOWN,
	APPLE_FUNCTION_PTP,
	APPLE_FUNCTION_USB_MUX,
	APPLE_FUNCTION_CDC_NCM,
	APPLE_FUNCTION_VALERIA
} APPLE_FUNCTION_TYPE, * PAPPLE_FUNCTION_TYPE;