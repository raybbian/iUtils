#pragma once

#define LOG_LOG(a, b, ...)									\
	do {													\
		DbgPrintEx(a, b, "%s:%i: ", __FILE__, __LINE__);	\
		DbgPrintEx(a, b, __VA_ARGS__);						\
		DbgPrintEx(a, b, "\n");								\
	} while (0)

#define LOG_ERROR(...) LOG_LOG(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)
#define LOG_INFO(...) LOG_LOG(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)

