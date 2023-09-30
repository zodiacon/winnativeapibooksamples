// QueryWS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <assert.h>

#pragma comment(lib, "ntdll")

const char* ProtectionToString(DWORD protect) {
	switch (protect) {
		case 1: return "Read Only";
		case 2: return "Execute";
		case 3: return "Execute/Read";
		case 4: return "Read/Write";
		case 5: return "Copy on Write";
		case 6: return "RWX";
		case 7: return "Executable/Copy on Write";
		case 9: return "No Cache/Read";
		case 10: return "No Cache/Execute";
		case 11: return "No Cache/Read";
		case 12: return "No Cache/Read/Write";
		case 13: return "No Cache/Copy on Write";
		case 14: return "No Cache/RWX";
		case 15: return "No Cache/Execute/Copy on Write";
		case 17: return "Guard/Read";
		case 18: return "Guard/Execute";
		case 19: return "Guard/Execute/Read";
		case 20: return "Guard/Read/Write";
		case 21: return "Guard/Copy on Write";
		case 22: return "Gaurd/RWX";
		case 23: return "Guard/Execute/Copy on Write";
		case 25: return "No Cache/Guard/Read";
		case 26: return "No Cache/Guard/Execute";
		case 27: return "No Cache/Guard/Execute/Read";
		case 28: return "No Cache/Guard/Read/Write";
		case 29: return "No Cache/Guard/Copy on Write";
		case 30: return "No Cache/Guard/RWX";
		case 31: return "No Cache/Guard/Execute/Copy on Write";
	}
	return "No Access";
}

bool QueryWS(HANDLE hProcess) {
	SIZE_T size = 1 << 18;
	std::unique_ptr<BYTE[]> buffer;
	NTSTATUS status;

	do {
		buffer = std::make_unique<BYTE[]>(size);
		if (status = NtQueryVirtualMemory(hProcess, nullptr, MemoryWorkingSetInformation, buffer.get(), size, nullptr); NT_SUCCESS(status))
			break;
		size *= 2;
	} while (status == STATUS_INFO_LENGTH_MISMATCH);

	if (!NT_SUCCESS(status))
		return false;

	auto ws = (MEMORY_WORKING_SET_INFORMATION*)buffer.get();
	for (ULONG_PTR i = 0; i < ws->NumberOfEntries; i++) {
		auto& info = ws->WorkingSetInfo[i];
		printf("%16zX Prot: %-17s Share: %d Shareable: %s Node: %d\n",
			info.VirtualPage << 12, ProtectionToString(info.Protection),
			(int)info.ShareCount, (int)info.Shared ? "Y" : "N", (int)info.Node);
	}
	return true;
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: QueryVS <pid>\n");
		return 0;
	}

	HANDLE hProcess;
	CLIENT_ID cid{ ULongToHandle(atoi(argv[1])) };
	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	if (auto status = NtOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &procAttr, &cid); !NT_SUCCESS(status)) {
		printf("Error opening process (status: 0x%X)\n", status);
		return status;
	}

	if (!QueryWS(hProcess))
		printf("Failed to query working set\n");

	NtClose(hProcess);
	return 0;
}

