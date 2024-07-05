#pragma once

#include "driver.h"
#include "child.h"

VOID UrbCompleteDispatch(
	WDFREQUEST Request
);

VOID UrbCompleteGetDescriptorfromDevice(
	WDFREQUEST Request
);

VOID UrbCompleteSetConfiguration(
	WDFREQUEST Request
);

VOID UrbCompleteSetInterface(
	WDFREQUEST Request
);

VOID UrbCompleteGetInterface(
	WDFREQUEST Request
);
