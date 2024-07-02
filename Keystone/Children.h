#pragma once

#include "driver.h"

//Child devices

#define IU_FUNCTION_MAX_NUMBER_OF_INTERFACES 3

typedef struct _IU_CHILD_DEVICE_ID {
	WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdHeader;
	APPLE_INTERFACE_TYPE InterfaceType;
	UCHAR CurrentParentConfig; //descriptor list pointers could collide even on diff config, so keep track?
	UCHAR NumberOfInterfaces;
	PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptorList[IU_FUNCTION_MAX_NUMBER_OF_INTERFACES];
} IU_CHILD_DEVICE_ID, * PIU_CHILD_DEVICE_ID;

//TODO: child context?

EVT_WDF_CHILD_LIST_SCAN_FOR_CHILDREN KeystoneEvtChildListScanForChildren;
EVT_WDF_CHILD_LIST_CREATE_DEVICE KeystoneEvtChildListCreateDevice;
