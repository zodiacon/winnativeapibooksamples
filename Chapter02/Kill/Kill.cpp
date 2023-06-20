// Kill.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ntdll")

NTSTATUS KillNative(int pid) {
	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	CLIENT_ID cid{};	// zero-out structure
	cid.UniqueProcess = ULongToHandle(pid);
	HANDLE hProcess;
	auto status = NtOpenProcess(&hProcess, PROCESS_TERMINATE, &procAttr, &cid);
	if (!NT_SUCCESS(status))
		return status;

	status = NtTerminateProcess(hProcess, 1);
	NtClose(hProcess);
	return status;
}

//
// for reference only
//
bool KillWin32(int pid) {
	auto hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (!hProcess)
		return false;

	auto success = TerminateProcess(hProcess, 1);
	CloseHandle(hProcess);
	return success;
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: Kill <pid>\n");
		return 0;
	}

	auto pid = strtol(argv[1], nullptr, 0);

	auto status = KillNative(pid);
	if (NT_SUCCESS(status))
		printf("Success!\n");
	else
		printf("Error: 0x%X\n", status);
	return 0;
}
