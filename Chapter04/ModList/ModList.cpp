// ModList.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

int main() {
	ULONG size;
	NtQuerySystemInformation(SystemModuleInformationEx, nullptr, 0, &size);
	std::unique_ptr<BYTE[]> buffer;
	for (;;) {
		buffer = std::make_unique<BYTE[]>(size);
		auto status = NtQuerySystemInformation(SystemModuleInformationEx, buffer.get(), size, &size);
		if (NT_SUCCESS(status))
			break;
	}

	//
	// weirdly enough, the last entry with NextOffset of zero is not valid
	//
	auto mod = (RTL_PROCESS_MODULE_INFORMATION_EX*)buffer.get();
	while (mod->NextOffset) {
		printf("Name: %s (%s) Base: 0x%p Checksum: 0x%08X Size: 0x%X\n",
			mod->BaseInfo.FullPathName + mod->BaseInfo.OffsetToFileName,
			mod->BaseInfo.FullPathName,
			mod->BaseInfo.ImageBase, mod->ImageChecksum,
			mod->BaseInfo.ImageSize);

		mod = (RTL_PROCESS_MODULE_INFORMATION_EX*)((PBYTE)mod + mod->NextOffset);
	}

	return 0;
}

