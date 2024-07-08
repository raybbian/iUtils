#include "driver.h"
#include "queue.h"
#include "configuration.h"
#include "apple.h"
#include "log.h"
#include "child.h"
#include <devpkey.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, KeystoneCreateDevice)
#pragma alloc_text (PAGE, KeystoneEvtDevicePrepareHardware)
#pragma alloc_text (PAGE, KeystoneEvtDeviceReleaseHardware)
#endif

NTSTATUS KeystoneCreateDevice(
	IN WDFDRIVER Driver,
	INOUT PWDFDEVICE_INIT DeviceInit
) {
	NTSTATUS status;
	PAGED_CODE();

	//get udid of device
	WDF_DEVICE_PROPERTY_DATA data;
	WDF_DEVICE_PROPERTY_DATA_INIT(&data, &DEVPKEY_Device_InstanceId);
	WCHAR udid[IU_MAX_UDID_LENGTH];
	ULONG udidLength = 0;
	DEVPROPTYPE propType;
	status = WdfFdoInitQueryPropertyEx(DeviceInit, &data, sizeof(udid), (PVOID)udid, &udidLength, &propType);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to get device udid: need size %d", udidLength);
		return status;
	}
	LOG_INFO("Device udid is %ws", udid);

	// this device is root enum persistent device
	if (wcsncmp(udid, L"ROOT", 4) == 0) { 
		LOG_INFO("added persistent root device");
		WDFDEVICE device;
		status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Failed to create persistent device");
			return status;
		}
		return STATUS_SUCCESS;
	}

	//register power callbacks
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
	pnpPowerCallbacks.EvtDevicePrepareHardware = KeystoneEvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = KeystoneEvtDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = KeystoneEvtDeviceD0Exit;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = KeystoneEvtDeviceReleaseHardware;
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	//create child list
	KeystoneChildListInitialize(DeviceInit);

	//find device num
	PIU_DEVICE_STORE deviceStore = DriverGetContext(Driver);
	UCHAR deviceNum = 0;
	for (; deviceNum < IU_MAX_NUMBER_OF_DEVICES; deviceNum++) {
		if (deviceStore->Devices[deviceNum].Udid == NULL)
			continue;
		if (wcscmp(deviceStore->Devices[deviceNum].Udid, udid) == 0) {
			LOG_INFO("Device was reconnected");
			break;
		}
		LOG_INFO("Found spot for device: %d", deviceNum);
		wcscpy(deviceStore->Devices[deviceNum].Udid, udid);
		break;
	}

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
	dev->Driver = Driver;
	dev->DeviceNum = deviceNum;
	wcscpy(dev->Udid, udid);

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
	PIU_DEVICE_STORE deviceStore = DriverGetContext(dev->Driver);
	LOG_INFO("%ws Entering D0", deviceStore->Devices[dev->DeviceNum].Udid);

	//get the device descriptor
	WdfUsbTargetDeviceGetDeviceDescriptor(
		dev->Handle,
		&dev->DeviceDescriptor
	);

	UCHAR deviceNum = dev->DeviceNum;
	IU_DEVICE_STATE prevState = InterlockedAdd(&deviceStore->Devices[deviceNum].DeviceState, 0);
	if (prevState == IU_DEVICE_DISCONNECTED) {
		LOG_INFO("First enabling super network mode (4)");
		SetAppleMode(dev, 4);
		InterlockedExchange(&deviceStore->Devices[deviceNum].DeviceState, IU_DEVICE_AWAITING_RECONNECT_PENDING_VALERIA);
		return STATUS_SUCCESS;
	}
	if (prevState == IU_DEVICE_AWAITING_RECONNECT_PENDING_VALERIA) {
		LOG_INFO("Adding valeria functionality");
		SetAppleMode(dev, 2);
		InterlockedExchange(&deviceStore->Devices[deviceNum].DeviceState, IU_DEVICE_AWAITING_RECONNECT);
		return STATUS_SUCCESS;
	}

	// Get current apple mode
	APPLE_CONNECTION_MODE curAppleMode = GetAppleMode(dev);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to get apple mode");
		return status;
	}
	LOG_INFO("cur apple mode %d", curAppleMode);

	//set configuration mode
	status = SetConfigurationByValue(dev, 6);
	if (!NT_SUCCESS(status)) {
		LOG_ERROR("Failed to set configuration");
		return status;
	}

	InterlockedExchange(&deviceStore->Devices[deviceNum].DeviceState, IU_DEVICE_OPERATIONAL);

	//debug
	LOG_INFO("Device has %d configurations", dev->DeviceDescriptor.bNumConfigurations);
	for (UCHAR i = 1; i <= dev->DeviceDescriptor.bNumConfigurations; i++) {
		ULONG configLen;
		UCHAR buf[IU_MAX_CONFIGURATION_BUFFER_SIZE];
		status = GetConfigDescriptorByValue(dev, buf, sizeof(buf), i, &configLen);
		if (!NT_SUCCESS(status)) {
			LOG_ERROR("Could not get configuration descriptor %d for debug", i);
			continue;
		}
		if (((PUSB_CONFIGURATION_DESCRIPTOR)buf)->wTotalLength > IU_MAX_CONFIGURATION_BUFFER_SIZE) {
			LOG_ERROR("Device config is too big!");
			continue;
		}
		for (USHORT j = 0; j < ((PUSB_CONFIGURATION_DESCRIPTOR)buf)->wTotalLength; j++) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%02X ", buf[j]);
		}
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "\n");
	}


	return status;
}

NTSTATUS KeystoneEvtDeviceD0Exit(
	WDFDEVICE Device,
	WDF_POWER_DEVICE_STATE PreviousState
) {
	UNREFERENCED_PARAMETER(PreviousState);
	PIU_DEVICE Dev = DeviceGetContext(Device);
	PIU_DEVICE_STORE deviceStore = DriverGetContext(Dev->Driver);
	LOG_INFO("%ws Exiting D0", deviceStore->Devices[Dev->DeviceNum].Udid);

	IU_DEVICE_STATE curState = InterlockedAdd(&deviceStore->Devices[Dev->DeviceNum].DeviceState, 0);
	//if was awaiting reconnect, then do not mark as disconnected
	if (curState == IU_DEVICE_AWAITING_RECONNECT || curState == IU_DEVICE_AWAITING_RECONNECT_PENDING_VALERIA) {
		LOG_INFO("Device is reconnecting, not marking disc");
		return STATUS_SUCCESS;
	}

	InterlockedExchange(&deviceStore->Devices[Dev->DeviceNum].DeviceState, IU_DEVICE_DISCONNECTED);
	LOG_INFO("Marking device as disconnected");
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
