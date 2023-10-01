// QueryVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

//
// extended with flags not present in phnt
//
typedef struct _MEMORY_REGION_INFORMATION_EX {
	PVOID AllocationBase;
	ULONG AllocationProtect;
	union {
		ULONG RegionType;
		struct {
			ULONG Private : 1;
			ULONG MappedDataFile : 1;
			ULONG MappedImage : 1;
			ULONG MappedPageFile : 1;
			ULONG MappedPhysical : 1;
			ULONG DirectMapped : 1;
			ULONG SoftwareEnclave : 1;
			ULONG PageSize64K : 1;
			ULONG PlaceholderReservation : 1;
			ULONG MappedWriteWatch : 1;
			ULONG PageSizeLarge : 1;
			ULONG PageSizeHuge : 1;
			ULONG Reserved : 19;
		};
	};
	SIZE_T RegionSize;
	SIZE_T CommitSize;
	ULONG_PTR PartitionId;
	ULONG_PTR NodePreference;
} MEMORY_REGION_INFORMATION_EX;

std::wstring MemoryRegionFlagsToString(ULONG flags) {
	std::wstring result;
	static PCWSTR text[] = {
		L"Private", L"Data File", L"Image", L"Page File",
		L"Physical", L"Direct", L"Enclave", L"64KB Page",
		L"Placeholder Reserve", L"Write Watch",
		L"Large Page", L"Huge Page",
	};

	for(int i = 0; i < _countof(text); i++)
		if (flags & (1 << i)) {
			result += text[i];
			result += L", ";
		}

	if (!result.empty())
		return result.substr(0, result.length() - 2);
	return L"";
}

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

std::wstring VirtualAttributesToString(MEMORY_WORKING_SET_EX_BLOCK const& attr) {
	std::wstring result;
	result += attr.Valid ? L"(Valid, " : L"(Invalid, ";
	if (attr.Valid) {
		result += L"Prot: ";
		auto prot = ProtectionToString(attr.Win32Protection);
		result.insert(result.end(), prot.begin(), prot.end());
		result += L", ";
		if (attr.Shared) {
			result += std::format(L"Share: {}, ", attr.ShareCount);
		}
		if (attr.Locked)
			result += L"Locked, ";
		if (attr.LargePage)
			result += L"Large, ";
		result += std::format(L"Node: {}, ", attr.Node);
		if (attr.Win32GraphicsProtection > 0) {
			result += L"GfxProt: ";
			prot = ProtectionToString(attr.Win32GraphicsProtection);
			result.insert(result.end(), prot.begin(), prot.end());
			result += L", ";
		}
	}
	else {
		if (attr.Invalid.PageTable)
			result += L"PT, ";
		if (attr.Invalid.ModifiedList)
			result += L"Modified, ";
	}
	result += std::format(L"Pri: {}, ", attr.Priority);
	if (attr.Bad)
		result += L"Bad, ";
	if (attr.SharedOriginal)
		result += L"Proto, ";

	return result.substr(0, result.length() - 2) + L")";
}

std::wstring SigningLevelToString(int level) {
	std::wstring text;
	switch (level) {
		case SE_SIGNING_LEVEL_UNCHECKED:	text = L"Unchecked"; break;
		case SE_SIGNING_LEVEL_UNSIGNED:		text = L"Unsigned"; break;
		case SE_SIGNING_LEVEL_ENTERPRISE:	text = L"Exterprise"; break;
		case SE_SIGNING_LEVEL_DEVELOPER:	text = L"Developer"; break;
		case SE_SIGNING_LEVEL_AUTHENTICODE: text = L"Authenticode"; break;
		case SE_SIGNING_LEVEL_STORE:		text = L"Store"; break;
		case SE_SIGNING_LEVEL_ANTIMALWARE:	text = L"Antimalware"; break;
		case SE_SIGNING_LEVEL_MICROSOFT:	text = L"Microsoft"; break;
		case SE_SIGNING_LEVEL_DYNAMIC_CODEGEN: text = L"Codegen"; break;
		case SE_SIGNING_LEVEL_WINDOWS:		text = L"Windows"; break;
		case SE_SIGNING_LEVEL_WINDOWS_TCB:	text = L"WinTCB"; break;
		default: text = L"Custom"; break;
	}

	return std::format(L"{} ({})", text, level);
}

std::wstring Details(HANDLE hProcess, MEMORY_BASIC_INFORMATION const& mbi) {
	static auto hProcess2 = INVALID_HANDLE_VALUE;
	if (hProcess2 == INVALID_HANDLE_VALUE) {
		if (!NT_SUCCESS(NtDuplicateObject(NtCurrentProcess(), hProcess, NtCurrentProcess(), &hProcess2, PROCESS_QUERY_INFORMATION, 0, 0)))
			hProcess2 = nullptr;
	}

	std::wstring details;

	MEMORY_WORKING_SET_EX_INFORMATION ws;
	ws.VirtualAddress = mbi.BaseAddress;
	if (NT_SUCCESS(NtQueryVirtualMemory(hProcess, nullptr, MemoryWorkingSetExInformation, &ws, sizeof(ws), nullptr))) {
		details += VirtualAttributesToString(ws.u1.VirtualAttributes);
	}

	//MEMORY_PHYSICAL_CONTIGUITY_INFORMATION pm{};
	//pm.VirtualAddress = mbi.BaseAddress;
	//pm.Size = mbi.RegionSize;
	//pm.ContiguityUnitSize = 1 << 12;
	//if (NT_SUCCESS(NtQueryVirtualMemory(hProcess, nullptr, MemoryPhysicalContiguityInformation, &pm, sizeof(pm), nullptr))) {
	//}

	if (mbi.State == MEM_COMMIT) {
		MEMORY_REGION_INFORMATION_EX ri;
		if (NT_SUCCESS(NtQueryVirtualMemory(hProcess, mbi.BaseAddress, MemoryRegionInformationEx, &ri, sizeof(ri), nullptr))) {
			details += MemoryRegionFlagsToString(ri.RegionType);
		}

		if (mbi.Type == MEM_IMAGE) {
			MEMORY_IMAGE_INFORMATION info;
			if (NT_SUCCESS(NtQueryVirtualMemory(hProcess, mbi.BaseAddress, MemoryImageInformation, &info, sizeof(info), nullptr))) {
				details += std::format(L" (Image Base: 0x{:X} Size: 0x{:X} {} Sign: {}) ",
					(SIZE_T)info.ImageBase, info.SizeOfImage, info.ImageNotExecutable ? L"NX" : L"",
					SigningLevelToString(info.ImageSigningLevel));
			}
		}
		if (mbi.Type == MEM_MAPPED || mbi.Type == MEM_IMAGE) {
			BYTE buffer[1 << 10];
			if (NT_SUCCESS(NtQueryVirtualMemory(hProcess, mbi.BaseAddress, MemoryMappedFilenameInformation, buffer, sizeof(buffer), nullptr))) {
				auto us = (PUNICODE_STRING)buffer;
				if (!details.empty())
					details += L" ";
				details += std::wstring(us->Buffer, us->Length / sizeof(WCHAR));
			}
		}
	}
	return details;
}

void DisplayMemoryInfo(HANDLE hProcess, MEMORY_BASIC_INFORMATION const& mi) {
	printf("%p %16zX %-10s %-8s %-19s %-19s %ws\n",
		mi.BaseAddress, mi.RegionSize,
		StateToString(mi.State), TypeToString(mi.Type),
		ProtectionToString(mi.Protect).c_str(), ProtectionToString(mi.AllocationProtect).c_str(),
		Details(hProcess, mi).c_str());
}

void QueryVM(HANDLE hProcess) {
	SIZE_T committed;
	if (NT_SUCCESS(NtQueryVirtualMemory(hProcess, nullptr, MemorySharedCommitInformation, &committed, sizeof(committed), nullptr))) {
		printf("Shared commit: %llu KB\n", committed << 2);
	}

	MEMORY_BASIC_INFORMATION mbi;
	BYTE* address = nullptr;

	int count = printf("%-16s %-16s %-10s %-8s %-19s %-19s Details\n",
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

