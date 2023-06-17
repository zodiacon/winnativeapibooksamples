// HelloNative.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION {
	ULONG Reserved;
	ULONG TimerResolution;
	ULONG PageSize;
	ULONG NumberOfPhysicalPages;
	ULONG LowestPhysicalPageNumber;
	ULONG HighestPhysicalPageNumber;
	ULONG AllocationGranularity;
	ULONG_PTR MinimumUserModeAddress;
	ULONG_PTR MaximumUserModeAddress;
	ULONG_PTR ActiveProcessorsAffinityMask;
	CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, * PSYSTEM_BASIC_INFORMATION;

typedef NTSTATUS (NTAPI *PNtQuerySystemInformation)(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength);

int main() {
	auto NtQuerySystemInformation = (PNtQuerySystemInformation)GetProcAddress(
		GetModuleHandle(L"ntdll"), "NtQuerySystemInformation");

	if (NtQuerySystemInformation) {
		SYSTEM_BASIC_INFORMATION sysInfo;
		NTSTATUS status = NtQuerySystemInformation(SystemBasicInformation, &sysInfo, sizeof(sysInfo), nullptr);
		if (status == 0) {
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
	}
	return 0;
}
