// Heaps.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")


#define HEAP_RANGE_TYPE_COMMITTED 1
#define HEAP_RANGE_TYPE_RESERVED  2

#define HEAP_BLOCK_BUSY              1
#define HEAP_BLOCK_EXTRA_INFORMATION 2
#define HEAP_BLOCK_LARGE_BLOCK       4
#define HEAP_BLOCK_LFH_BLOCK         8

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

const char* RangeTypeToString(ULONG type) {
	switch (type) {
		case HEAP_RANGE_TYPE_COMMITTED: return "Committed";
		case HEAP_RANGE_TYPE_RESERVED: return "Reserved";
	}
	return "<Unknown>";
}

std::string ProtectionToString(DWORD protect) {
	std::string text;
	if (protect & PAGE_GUARD) {
		text += "Guard/";
		protect &= ~PAGE_GUARD;
	}
	if (protect & PAGE_WRITECOMBINE) {
		text += "Write Combine/";
		protect &= ~PAGE_WRITECOMBINE;
	}

	switch (protect) {
		case 0: break;
		case PAGE_NOACCESS: text += "No Access"; break;
		case PAGE_READONLY: text += "RO"; break;
		case PAGE_READWRITE: text += "RW"; break;
		case PAGE_EXECUTE_READWRITE: text += "RWX"; break;
		case PAGE_WRITECOPY: text += "Write Copy"; break;
		case PAGE_EXECUTE: text += "Execute"; break;
		case PAGE_EXECUTE_READ: text += "RX"; break;
		case PAGE_EXECUTE_WRITECOPY: text += "Execute/Write Copy"; break;
		default: text += "<other>";
	}
	return text;
}

std::string BlockFlagsToString(ULONG flags) {
	static const struct {
		ULONG flag;
		PCSTR text;
	} data[] = {
		{ HEAP_BLOCK_BUSY, "Busy" },
		{ HEAP_BLOCK_EXTRA_INFORMATION, "Extra" },
		{ HEAP_BLOCK_LARGE_BLOCK, "Large" },
		{ HEAP_BLOCK_LFH_BLOCK, "LFH" },
	};

	std::string text;
	for (auto& d : data) {
		if ((d.flag & flags) == d.flag)
			(text += d.text) += ", ";
	}
	return text.empty() ? "" : text.substr(0, text.length() - 2);
}

void DisplayBlocks(PBYTE buffer, HEAP_RANGE_INFORMATION* range) {
	if (range->FirstBlockInformationOffset == 0)
		return;

	auto block = (HEAP_BLOCK_INFORMATION*)(buffer + range->FirstBlockInformationOffset);
	for (;;) {
		printf("    Block Addr: 0x%p Size: 0x%04zX Overhead: %2zd %s\n",
			(PVOID)block->Address, block->DataSize, block->OverheadSize,
			BlockFlagsToString(block->Flags).c_str());
		if (block->NextBlockInformationOffset == 0)
			break;

		block = (HEAP_BLOCK_INFORMATION*)(buffer + block->NextBlockInformationOffset);
	}
}

void DisplayRanges(PBYTE buffer, HEAP_REGION_INFORMATION* region) {
	if (region->FirstRangeInformationOffset == 0)
		return;

	auto range = (HEAP_RANGE_INFORMATION*)(buffer + region->FirstRangeInformationOffset);
	for (;;) {
		printf("  Range Addr: 0x%p Size: 0x%08zX Type: %s Prot: %s\n",
			(PVOID)range->Address, range->Size,
			RangeTypeToString(range->Type),
			ProtectionToString(range->Protection).c_str());
		DisplayBlocks(buffer, range);
		if (range->NextRangeInformationOffset == 0)
			break;
		range = (HEAP_RANGE_INFORMATION*)(buffer + range->NextRangeInformationOffset);
	}
}

void DisplayRegions(PBYTE buffer, HEAP_INFORMATION* hi) {
	if (hi->FirstRegionInformationOffset == 0)
		return;

	auto region = (HEAP_REGION_INFORMATION*)(buffer + hi->FirstRegionInformationOffset);
	for (;;) {
		printf(" Region Addr: 0x%p Commit: 0x%08zX Reserve: 0x08%zX\n",
			(PVOID)region->Address, region->CommitSize, region->ReserveSize);
		DisplayRanges(buffer, region);
		if (region->NextRegionInformationOffset == 0)
			break;
		region = (HEAP_REGION_INFORMATION*)(buffer + region->NextRegionInformationOffset);
	}
}

bool DumpHeaps(HANDLE hProcess, ULONG level) {
	SIZE_T size = 1 << 16;
	std::unique_ptr<BYTE[]> buffer;
	NTSTATUS status;
	SIZE_T len;
	HEAP_EXTENDED_INFORMATION* info;
	for (;;) {
		buffer = std::make_unique<BYTE[]>(size);
		info = (HEAP_EXTENDED_INFORMATION*)buffer.get();
		info->ProcessHandle = hProcess;
		info->Level = level;
		status = RtlQueryHeapInformation(nullptr, (HEAP_INFORMATION_CLASS)HeapExtendedInformation, buffer.get(), size, &len);
		if (NT_SUCCESS(status))
			break;
		if (status == STATUS_BUFFER_TOO_SMALL) {
			size = len + (1 << 12);
			continue;
		}
		break;
	}
	if (!NT_SUCCESS(status))
		return false;

	auto hi = (HEAP_INFORMATION*)(buffer.get() + info->ProcessHeapInformation.FirstHeapInformationOffset);
	auto heaps = info->ProcessHeapInformation.NumberOfHeaps;
	printf("Total heaps: %lu Commit: 0x%zX Reserved: 0x%zX\n",
		heaps, info->ProcessHeapInformation.CommitSize, info->ProcessHeapInformation.ReserveSize);

	for (ULONG i = 0; i < heaps; i++) {
		printf("Heap %2d: Addr: 0x%p Commit: 0x%08zX Reserve: 0x%08zX %s\n", i + 1,
			hi->Address, hi->CommitSize, hi->ReserveSize, hi->Mode ? "(LFH)" : "");
		DisplayRegions(buffer.get(), hi);
		hi = (HEAP_INFORMATION*)(buffer.get() + hi->NextHeapInformationOffset);
	}

	return true;
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: Heaps <pid> [level]\n");
		printf(" Level: 0 (default): Heaps 1: +Regions 2: +Ranges 3: +blocks\n");
		return 0;
	}

	auto pid = strtol(argv[1], nullptr, 0);
	if (pid == 0)
		pid = ::GetCurrentProcessId();

	HANDLE hProcess;
	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	CLIENT_ID cid{ ULongToHandle(pid) };
	auto status = NtOpenProcess(&hProcess, MAXIMUM_ALLOWED, &procAttr, &cid);
	if (!NT_SUCCESS(status)) {
		printf("Failed to open process (0x%X)\n", status);
		return status;
	}

	ULONG level = HEAP_INFORMATION_LEVEL_HEAP + (argc > 2 ? atoi(argv[2]) : 0);

	if (!DumpHeaps(hProcess, level))
		printf("Failed to enumerate heaps\n");
	NtClose(hProcess);

	return 0;
}

