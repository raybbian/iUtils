#pragma once

//Must get and parse configuration like this because wdf doesn't even have wdm shit

NTSTATUS GetCurrentConfiguration(
	IN PIU_DEVICE Dev,
	OUT PUCHAR Configuration,
	OUT PINT32 Ret
);

NTSTATUS GetConfigDescriptorByIndex(
	IN PIU_DEVICE Dev,
	OUT PVOID Buffer,
	IN ULONG Size,
	IN UCHAR Index, //0-indexed
	OUT PULONG Received
);

NTSTATUS GetConfigDescriptorByValue(
	IN PIU_DEVICE Dev,
	OUT PVOID Buffer,
	IN ULONG Size,
	IN UCHAR Value,
	OUT PULONG Received
);

NTSTATUS UnsetConfiguration(
	INOUT PIU_DEVICE Dev
);

NTSTATUS SetConfigurationByValue(
	INOUT PIU_DEVICE Dev,
	IN UCHAR Value
);

NTSTATUS SetInterface(
	PIU_DEVICE Dev,
	UCHAR InterfaceNumber,
	UCHAR Altsetting
);

NTSTATUS UpdateConfigurationFromInterfaceList(
	PIU_DEVICE Dev,
	PUSBD_INTERFACE_LIST_ENTRY InterfaceList //after calling set interface
);

NTSTATUS UpdateInterfaceFromInterfaceEntry(
	PIU_DEVICE Dev,
	PUSBD_INTERFACE_LIST_ENTRY InterfaceEntry
);
