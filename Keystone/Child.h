#pragma once

#include "driver.h"

//Child devices
VOID KeystoneChildListInitialize(
	IN PWDFDEVICE_INIT DeviceInit
);

#define IU_FUNCTION_MAX_NUMBER_OF_INTERFACES 4
#define IU_FUNCTION_MAX_COMPATIBLE_IDS 6

// modeled after usbccgp
typedef struct _IU_CHILD_IDENTIFIER {
	WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdHeader;
	APPLE_FUNCTION_TYPE FunctionType;
	UCHAR CurrentParentConfig;
	UCHAR NumberOfInterfaces;
	UCHAR NumberOfCompatibleIds;
	UCHAR InterfacesUsed[IU_FUNCTION_MAX_NUMBER_OF_INTERFACES];
	UNICODE_STRING HardwareId;
	UNICODE_STRING CompatibleIds[IU_FUNCTION_MAX_COMPATIBLE_IDS];
	UNICODE_STRING FunctionalDescription;
} IU_CHILD_IDENTIFIER, * PIU_CHILD_IDENTIFIER;

//only store things that are unique to child
typedef struct _IU_CHILD_DEVICE {
	PIU_DEVICE Parent;
	USB_DEVICE_DESCRIPTOR DeviceDescriptor;

	struct {
		UCHAR Buffer[IU_MAX_CONFIGURATION_BUFFER_SIZE];
		PUSB_CONFIGURATION_DESCRIPTOR Descriptor;
		//reach for parent interface handles, pipe information
	} Config;
} IU_CHILD_DEVICE, * PIU_CHILD_DEVICE, CHILD_DEVICE_CONTEXT, *PCHILD_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CHILD_DEVICE_CONTEXT, ChildDeviceGetContext);

EVT_WDF_CHILD_LIST_SCAN_FOR_CHILDREN KeystoneEvtChildListScanForChildren;
EVT_WDF_CHILD_LIST_CREATE_DEVICE KeystoneEvtChildListCreateDevice;
EVT_WDF_CHILD_LIST_IDENTIFICATION_DESCRIPTION_COMPARE KeystoneEvtWdfChildListIdentificationDescriptionCompare;

NTSTATUS ExtractChildDeviceDescriptor(
	IN PIU_DEVICE Dev,
	IN PIU_CHILD_IDENTIFIER ChildId,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG Received
);

NTSTATUS ExtractChildConfigurationDescriptor(
	IN PIU_DEVICE Dev,
	IN PIU_CHILD_IDENTIFIER ChildId,
	OUT PVOID Buffer,
	IN ULONG BufferSize,
	OUT PULONG Received
);
