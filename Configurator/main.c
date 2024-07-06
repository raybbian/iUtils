#include <Windows.h>
#include <SetupAPI.h>
#include <stdio.h>
#include <devpkey.h>
#include <initguid.h>
#include "../Keystone/Public.h"

#pragma comment (lib, "Setupapi.lib")

HANDLE FindDeviceInterface() {
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
    DWORD requiredSize = 0;
    DWORD detailDataSize;
    BOOL result;

    // Get the device interface information set
    deviceInfoSet = SetupDiGetClassDevs(
        &GUID_DEVINTERFACE_Keystone,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        printf("Failed to get device interface set: %d\n", GetLastError());
        return NULL;
    }

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    // Enumerate device interfaces
    result = SetupDiEnumDeviceInterfaces(
        deviceInfoSet,
        NULL,
        &GUID_DEVINTERFACE_Keystone,
        0,
        &deviceInterfaceData
    );
    if (!result) {
        printf("Failed to enumerate device interfaces: %d\n", GetLastError());
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return NULL;
    }

    // Get the required size for the device interface detail data
    SetupDiGetDeviceInterfaceDetail(
        deviceInfoSet,
        &deviceInterfaceData,
        NULL,
        0,
        &requiredSize,
        NULL
    );

    deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
    if (!deviceInterfaceDetailData) {
        printf("Failed to allocate memory\n");
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return NULL;
    }

    deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    detailDataSize = requiredSize;

    // Get the device interface detail data
    result = SetupDiGetDeviceInterfaceDetail(
        deviceInfoSet,
        &deviceInterfaceData,
        deviceInterfaceDetailData,
        detailDataSize,
        NULL,
        NULL
    );

    if (!result) {
        printf("Failed to get device interface detail: %d\n", GetLastError());
        free(deviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return NULL;
    }

    // The symbolic link is in deviceInterfaceDetailData->DevicePath
    printf("Device path: %ls\n", deviceInterfaceDetailData->DevicePath);

    // Use the device path to open the device
    HANDLE hDevice = CreateFileW(
        deviceInterfaceDetailData->DevicePath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to open device: %d\n", GetLastError());
		free(deviceInterfaceDetailData);
		SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return NULL;
    }

    free(deviceInterfaceDetailData);
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return hDevice;
}

BOOL GetAppleMode(
	IN HANDLE DeviceHandle, 
	OUT PAPPLE_CONNECTION_MODE Mode
) {
	UCHAR ret = 0;
	DWORD amt = 0;
	*Mode = 0;
	BOOL status = DeviceIoControl(
		DeviceHandle,			// handle
		IOCTL_QUERY_APPLE_MODE,	// ioctl
		NULL,					// in buffer
		0,						// in buffer len
		&ret,					// out buffer
		sizeof(ret),			// out buffer len
		&amt,					// bytes received
		NULL					// overlap
	);
	if (status) 
		*Mode = ret;
	return status;
}

BOOL SetAppleMode(
	IN HANDLE DeviceHandle, 
	IN APPLE_CONNECTION_MODE Mode
) {
	UCHAR set = Mode;
	DWORD amt = 0;
	return DeviceIoControl(
		DeviceHandle,			// handle
		IOCTL_SET_APPLE_MODE,	// ioctl
		&set,					// in buffer
		sizeof(set),			// in buffer len
		NULL,					// out buffer
		0,						// out buffer len
		&amt,					// bytes received
		NULL					// overlap
	);
}

int main() {
    HANDLE deviceHandle = FindDeviceInterface();
    if (deviceHandle == NULL) {
        printf("Failed to get device handle");
        getch();
        return 0;
    }

    APPLE_CONNECTION_MODE curMode = APPLE_MODE_UNKNOWN;
    if (!GetAppleMode(deviceHandle, &curMode)) {
        printf("Failed to get apple mode\n");
        goto Cleanup;
    }
    printf("Current apple mode is %d\n", curMode);

    printf("Input desired mode: \n");
    int desiredMode = 0;
    scanf_s("%d", &desiredMode);

    if (!SetAppleMode(deviceHandle, desiredMode)) {
        printf("Failed to set apple mode to %d\n", desiredMode);
        goto Cleanup;
    }
    printf("Set apple mode!\n");

Cleanup:
    CloseHandle(deviceHandle);
    getch();
	return 0;
}