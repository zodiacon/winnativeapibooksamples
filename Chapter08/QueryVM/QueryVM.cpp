// QueryVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

const char* StateToString(DWORD state) {
	switch (state) {
		case MEM_FREE: return "Free";
		case MEM_COMMIT: return "Committed";
		case MEM_RESERVE: return "Reserved";
	}
	return "";
}

const char* TypeToString(DWORD type) {
	switch (type) {
		case MEM_IMAGE: return "Image";
		case MEM_MAPPED: return "Mapped";
		case MEM_PRIVATE: return "Private";
	}
	return "";
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

std::wstring Details(HANDLE hProcess, MEMORY_BASIC_INFORMATION const& mbi) {
	if (mbi.State == MEM_COMMIT) {
		//
		// get working set details
		//
		static auto hProcess2 = INVALID_HANDLE_VALUE;
		if (hProcess2 == INVALID_HANDLE_VALUE) {
			NtDuplicateObject(NtCurrentProcess(), hProcess, NtCurrentProcess(), &hProcess2, PROCESS_QUERY_INFORMATION, 0, 0);
		}
		if (hProcess2) {
		}
	}
	return L"";
}

void DisplayMemoryInfo(HANDLE hProcess, MEMORY_BASIC_INFORMATION const& mi) {
	printf("%p %16zX %-10s %-8s %-15s %-15s %ws\n",
		mi.BaseAddress, mi.RegionSize,
		StateToString(mi.State), TypeToString(mi.Type),
		ProtectionToString(mi.Protect).c_str(), ProtectionToString(mi.AllocationProtect).c_str(),
		Details(hProcess, mi).c_str());
}

void QueryVM(HANDLE hProcess) {
	MEMORY_BASIC_INFORMATION mbi;
	BYTE* address = nullptr;

	int count = printf("%-16s %-16s %-10s %-8s %-15s %-15s Details\n",
		"Address", "Size", "State", "Type", "Protect", "Alloc Prot");
	printf("%s\n", std::string(count, '-').c_str());

	while (NT_SUCCESS(NtQueryVirtualMemory(hProcess, address, MemoryBasicInformation, &mbi, sizeof(mbi), nullptr))) {
		DisplayMemoryInfo(hProcess, mbi);
		address += mbi.RegionSize;
	}
}

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Usage: QueryVM <pid>\n");
		return 0;
	}

	HANDLE hProcess;
	CLIENT_ID cid{ ULongToHandle(atoi(argv[1])) };
	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	if (auto status = NtOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &procAttr, &cid); !NT_SUCCESS(status)) {
		printf("Error opening process (status: 0x%X)\n", status);
		return status;
	}

	QueryVM(hProcess);

	NtClose(hProcess);
	return 0;
}

