// Beep.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <ntddbeep.h>

#pragma comment(lib, "ntdll")

int main(int argc, const char* argv[]) {
	if(argc < 2) {
		printf("Usage: Beep.exe <frequency> [duration]\n");
		return 0;
	}

	HANDLE hFile;
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, L"\\Device\\Beep");
	OBJECT_ATTRIBUTES attr = RTL_CONSTANT_OBJECT_ATTRIBUTES(&name, 0);
	IO_STATUS_BLOCK ioStatus;
	auto status = NtOpenFile(&hFile, FILE_WRITE_DATA | SYNCHRONIZE, &attr, &ioStatus, 
		FILE_SHARE_WRITE | FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
	if(!NT_SUCCESS(status)) {
		printf("Error opening beep device: 0x%08X\n", status);
		return 1;
	}

	BEEP_SET_PARAMETERS params;
	params.Duration = argc == 2 ? 1000 : strtol(argv[2], nullptr, 0);
	params.Frequency = strtol(argv[1], nullptr, 0);

	status = NtDeviceIoControlFile(hFile, nullptr, nullptr, nullptr, &ioStatus,
		IOCTL_BEEP_SET, &params, sizeof(params), nullptr, 0);

	//
	// prove sound is async
	//
	LARGE_INTEGER ticks, delay;
	NtQuerySystemTime(&ticks);
	delay.QuadPart = -250000;	// 25msec
	auto target = ticks.QuadPart + params.Duration * 10000;

	while (ticks.QuadPart < target) {
		printf(".");
		NtDelayExecution(FALSE, &delay);
		NtQuerySystemTime(&ticks);
	}

	NtClose(hFile);
	return 0;
}

