// HelloNative3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>

#pragma comment(lib, "ntdll")

int main() {
	SYSTEM_BASIC_INFORMATION sysInfo;
	NTSTATUS status = NtQuerySystemInformation(SystemBasicInformation, &sysInfo, sizeof(sysInfo), nullptr);
	if (status == STATUS_SUCCESS) {
		//
		// call succeeded
		//
		printf("Page size: %u bytes\n", sysInfo.PageSize);
		printf("Processors: %u\n", (ULONG)sysInfo.NumberOfProcessors);
		printf("Physical pages: %u\n", sysInfo.NumberOfPhysicalPages);
		printf("Lowest  Physical page: %u\n", sysInfo.LowestPhysicalPageNumber);
		printf("Highest Physical page: %u\n", sysInfo.HighestPhysicalPageNumber);
	}
	else {
		printf("Error calling NtQuerySystemInformation (0x%X)\n", status);
	}
	return 0;
}
