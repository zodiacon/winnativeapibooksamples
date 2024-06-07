// MemZones.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>

#pragma comment(lib, "ntdll")

extern "C" NTSTATUS NTAPI RtlExtendMemoryZone(
	_In_ PVOID MemoryZone,
	_In_ SIZE_T Increment);

int main() {
	PVOID zone;
	auto status = RtlCreateMemoryZone(&zone, 1 << 12, 0);
	if (!NT_SUCCESS(status)) {
		printf("Failed to create memory zone (0x%X)\n", status);
		return status;
	}

	PCHAR buffers[100]{};

	for (int i = 0; i < _ARRAYSIZE(buffers); i++) {
		PVOID buffer;
		status = RtlAllocateMemoryZone(zone, 128 + i * 2, &buffer);
		if (!NT_SUCCESS(status)) {
			printf("Failed to allocate block %d (0x%X). Extending...\n", i, status);
			RtlExtendMemoryZone(zone, 256);
			i--;
			continue;
		}
		else {
			buffers[i] = (PCHAR)buffer;
			sprintf_s(buffers[i], 128, "Data stored in block %d", i);
		}
	}

	for (int i = 0; i < _ARRAYSIZE(buffers); i++) {
		if (buffers[i]) {
			printf("%3d: %s\n", i, buffers[i]);
		}
	}

	auto const pRtlResetMemoryZone = (decltype(RtlResetMemoryZone)*)GetProcAddress(GetModuleHandle(L"ntdll"), "RtlResetMemoryZone");
	pRtlResetMemoryZone(zone);

	RtlDestroyMemoryZone(zone);
	return 0;
}
