#pragma once

DEFINE_GUID (GUID_DEVINTERFACE_Keystone,
    0xf6390607,0x789d,0x4764,0xb5,0xa7,0x05,0x8e,0xfb,0x8f,0xdc,0x5d);
// {f6390607-789d-4764-b5a7-058efb8fdc5d}

#undef IN
#define IN _In_
#undef OUT
#define OUT _Out_
#define INOUT _Inout_
#define INOPT _In_opt_
#define INOUTOPT _Inout_opt_

#define IU_MAX_NUMBER_OF_DEVICES 8
#define IU_NUMBER_OF_APPLE_MODES 7
#define IU_MAX_NUMBER_OF_CONFIGURATIONS 7
#define IU_NUMBER_OF_FEATURES 6

typedef enum {
	APPLE_MODE_UNKNOWN = 0,
	APPLE_MODE_BASE = 1, //4 configurations
	APPLE_MODE_BASE_VALERIA = 2, //5 configurations
	APPLE_MODE_BASE_NETWORK = 3, //5 configurations
	APPLE_MODE_BASE_NETWORK_TETHER = 4, //6 configurations
	APPLE_MODE_BASE_NETWORK_VALERIA = 5, //cannot be set directly (3->2) //6 configurations
	APPLE_MODE_BASE_NETWORK_TETHER_VALERIA = 6, //cannot be set directly (4->2) //7 configurations
	APPLE_MODE_RECOVERY, //do not use
} APPLE_CONNECTION_MODE, * PAPPLE_CONNECTION_MODE;

/*
in apple omni mode (the mode we are using), the configurations are as follows :

+--------+-----+-------+--------+---------+--------+---------+
| Config | PTP | AUDIO | USBMUX | NETWORK | TETHER | VALERIA |
+--------+-----+-------+--------+---------+--------+---------+
| 1      | X   |       |        |         |        |         |
| 2      |     | X     |        |         |        |         |
| 3      | X   |       | X      |         |        |         |
| 4      | X   |       | X      |         | X      |         |
| 5      | X   |       | X      | X       |        |         |
| 6      | X   |       | X      | X       | X      |         |
| 7      | X   |       | X      |         |        | X       |
+--------+-----+-------+--------+---------+--------+---------+

*/

typedef enum {
	APPLE_FUNCTION_PTP = 0,
	APPLE_FUNCTION_AUDIO = 1,
	APPLE_FUNCTION_USB_MUX = 2,
	APPLE_FUNCTION_NETWORK = 3,
	APPLE_FUNCTION_TETHER = 4,
	APPLE_FUNCTION_VALERIA = 5
} APPLE_FUNCTION_TYPE, * PAPPLE_FUNCTION_TYPE;

extern const unsigned char APPLE_MODE_CAPABILITIES[IU_NUMBER_OF_APPLE_MODES][IU_MAX_NUMBER_OF_CONFIGURATIONS + 1][IU_NUMBER_OF_FEATURES];

#define IU_IOCTL_GET_MODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IU_IOCTL_GET_CONFIGURATION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IU_IOCTL_SET_CONFIGURATION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
