#include "driver.h"
#include "queue.h"
#include "configuration.h"
#include "apple.h"
#include "log.h"
#include "child.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, KeystoneCreateDevice)
#pragma alloc_text (PAGE, KeystoneEvtDevicePrepareHardware)
#pragma alloc_text (PAGE, KeystoneEvtDeviceReleaseHardware)
#endif

NTSTATUS KeystoneCreateDevice(
	INOUT PWDFDEVICE_INIT DeviceInit
) {
	NTSTATUS status;
	PAGED_CODE();

	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
	pnpPowerCallbacks.EvtDevicePrepareHardware = KeystoneEvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = KeystoneEvtDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = KeystoneEvtDeviceD0Exit;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = KeystoneEvtDeviceReleaseHardware;
	//TODO: add other pnpcallbacks
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	//TODO: other callbacks?
	//TODO: set device characteristics
	//TODO: access registry key?

	//create child list
	KeystoneChildListInitialize(DeviceInit);

	//create WDF device
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
	WDFDEVICE device;
	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to create wdf device");
		return status;
	}
	PIU_DEVICE dev = DeviceGetContext(device);
	RtlZeroMemory(dev, sizeof(IU_DEVICE));
	dev->Self = device;

	// init queue (has side effect of setting correct stack size for child pdo forwarding ?)
	// also to handle user app ioctl
	status = KeystoneQueueInitialize(device);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to create queue!");
		return status;
	}

	//create interface for talking with user app
	status = WdfDeviceCreateDeviceInterface(
		device,
		&GUID_DEVINTERFACE_Keystone,
		NULL //reference string?
	);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Create device interface failed");
		return status;
	}

	//TODO: WMI registration?

	return status;
}

NTSTATUS KeystoneEvtDevicePrepareHardware(
	IN WDFDEVICE Device,
	IN WDFCMRESLIST ResourceList,
	IN WDFCMRESLIST ResourceListTranslated
) {
	NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(ResourceList);
	UNREFERENCED_PARAMETER(ResourceListTranslated);
	PAGED_CODE();

	PIU_DEVICE dev = DeviceGetContext(Device);

	// Initialize WDM fields
	dev->WDM.Self = WdfDeviceWdmGetDeviceObject(Device);
	dev->WDM.PhysicalDeviceObject = WdfDeviceWdmGetPhysicalDevice(Device);
	dev->WDM.NextStackDevice = WdfDeviceWdmGetAttachedDevice(Device);
	status = USBD_CreateHandle(
		dev->WDM.Self,
		dev->WDM.NextStackDevice, 
		USBD_CLIENT_CONTRACT_VERSION_602, 
		'usbd', 
		&dev->WDM.Handle
	);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to get WDM handle, status=%X", status);
		return status;
	}
	dev->WDMIsInitialized = TRUE;

	// Initialize WDF Handle
	if (dev->Handle == NULL) { //could be same handle after stop (?)
		WDF_USB_DEVICE_CREATE_CONFIG createParams;
		WDF_USB_DEVICE_CREATE_CONFIG_INIT(
			&createParams,
			USBD_CLIENT_CONTRACT_VERSION_602
		);
		status = WdfUsbTargetDeviceCreateWithParameters(
			Device,
			&createParams,
			WDF_NO_OBJECT_ATTRIBUTES,
			&dev->Handle
		);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Failed to get WDF handle, status=%X", status);
			return status;
		}

	}

	return status;
}

NTSTATUS KeystoneEvtDeviceD0Entry(
	WDFDEVICE Device,
	WDF_POWER_DEVICE_STATE PreviousState
) {
	NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(PreviousState);
	PIU_DEVICE dev = DeviceGetContext(Device);

	//get the device descriptor
	WdfUsbTargetDeviceGetDeviceDescriptor(
		dev->Handle, 
		&dev->DeviceDescriptor
	);

	// Get current apple mode
	APPLE_CONNECTION_MODE curAppleMode = GetAppleMode(dev);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to get apple mode");
		return status;
	}

	// Assumes best configurations for apple mode, may need changes if apple changes
	UCHAR desiredConfig = GetDesiredConfigurationFromAppleMode(curAppleMode);

	//Set the best configuration for that mode
	status = SetConfigurationByValue(dev, desiredConfig);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to set configuration");
		return status;
	}

	return status;
}

NTSTATUS KeystoneEvtDeviceD0Exit(
	WDFDEVICE Device,
	WDF_POWER_DEVICE_STATE PreviousState
) {
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(PreviousState);

	LOG_INFO("Exiting D0");
	return STATUS_SUCCESS;
}

NTSTATUS KeystoneEvtDeviceReleaseHardware(
	WDFDEVICE Device,
	WDFCMRESLIST ResourceList
) {
	NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(ResourceList);
	PAGED_CODE();

	PIU_DEVICE dev = DeviceGetContext(Device);

	if (dev->WDMIsInitialized) 
		USBD_CloseHandle(dev->WDM.Handle);
	RtlZeroMemory(&dev->WDM, sizeof(dev->WDM));
	dev->WDMIsInitialized = FALSE;

	return status;
}
