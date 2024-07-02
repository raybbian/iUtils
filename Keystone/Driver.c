#include "driver.h"
#include "log.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, KeystoneEvtDeviceAdd)
#pragma alloc_text (PAGE, KeystoneEvtDriverContextCleanup)
#endif


NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT  DriverObject,
	IN PUNICODE_STRING RegistryPath
) {
	NTSTATUS status;

	LOG_INFO("Driver entry");

	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = KeystoneEvtDriverContextCleanup;

	WDF_DRIVER_CONFIG config;
	WDF_DRIVER_CONFIG_INIT(&config, KeystoneEvtDeviceAdd);

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,
		&config,
		WDF_NO_HANDLE
	);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to create WDF driver");
		return status;
	}

	return status;
}

NTSTATUS KeystoneEvtDeviceAdd(
	IN WDFDRIVER Driver,
	INOUT PWDFDEVICE_INIT DeviceInit
) {
	NTSTATUS status;
	UNREFERENCED_PARAMETER(Driver);
	PAGED_CODE();

	LOG_INFO("Device add");

	status = KeystoneCreateDevice(DeviceInit);

	return status;
}

VOID KeystoneEvtDriverContextCleanup(
	_In_ WDFOBJECT DriverObject
) {
	UNREFERENCED_PARAMETER(DriverObject);
	PAGED_CODE();

	LOG_INFO("Driver Context cleanup");
}
