// Heaps.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

typedef enum _HEAP_INFORMATION_LEVEL {
	HEAP_INFORMATION_LEVEL_PROCESS = 1,
	HEAP_INFORMATION_LEVEL_HEAP = 2,
	HEAP_INFORMATION_LEVEL_REGION = 3,
	HEAP_INFORMATION_LEVEL_RANGE = 4,
	HEAP_INFORMATION_LEVEL_BLOCK = 5,

	HEAP_INFORMATION_LEVEL_PERFORMANCECOUNTERS = 0x80000000,
	HEAP_INFORMATION_LEVEL_TAGGING = 0x40000000,
	HEAP_INFORMATION_LEVEL_STACK_DUMP = 0x20000000,
	HEAP_INFORMATION_LEVEL_STACK_CONTROL = 0x10000000,
	HEAP_INFORMATION_LEVEL_INTERNAL_PROCESS_INFO = 0x08000000,
} HEAP_INFORMATION_LEVEL;

bool DumpHeaps(HANDLE hProcess) {
	auto size = 128 * sizeof(HEAP_INFORMATION) + sizeof(HEAP_EXTENDED_INFORMATION);
	auto buffer = std::make_unique<BYTE[]>(size);
	auto info = (HEAP_EXTENDED_INFORMATION*)buffer.get();
	info->Process = hProcess;
	info->Level = HEAP_INFORMATION_LEVEL_HEAP;
	auto status = RtlQueryHeapInformation(nullptr, (HEAP_INFORMATION_CLASS)HeapExtendedInformation, info, size, nullptr);
	if (!NT_SUCCESS(status))
		return false;

	auto hi = (HEAP_INFORMATION*)(buffer.get() + info->ProcessHeapInformation.FirstHeapInformationOffset);
	auto heaps = info->ProcessHeapInformation.NumberOfHeaps;
	printf("Total heaps: %u Commit: 0x%zX Reserved: 0x%zX\n", 
		heaps, info->ProcessHeapInformation.CommitSize, info->ProcessHeapInformation.ReserveSize);

	for (ULONG i = 0; i < heaps; i++) {
		printf("Heap %2d: Addr: 0x%p Commit: 0x%08zX Reserve: 0x%08zX %s\n", i + 1,
			(PVOID)hi->Address, hi->CommitSize, hi->ReserveSize, hi->Mode ? "(LFH)" : "");
		hi = (HEAP_INFORMATION*)(buffer.get() + hi->NextHeapInformationOffset);
	}

	return true;
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: Heaps <pid>\n");
		return 0;
	}

	auto pid = strtol(argv[1], nullptr, 0);

	HANDLE hProcess;
	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	CLIENT_ID cid{ ULongToHandle(pid) };
	auto status = NtOpenProcess(&hProcess, MAXIMUM_ALLOWED,	&procAttr, &cid);
	if (!NT_SUCCESS(status)) {
		printf("Failed to open process (0x%X)\n", status);
		return status;
	}

	if (!DumpHeaps(hProcess))
		printf("Failed to enumerate heaps\n");

	return 0;
}

