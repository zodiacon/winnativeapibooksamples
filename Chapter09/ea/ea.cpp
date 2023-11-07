// ea.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

int Usage() {
	printf("Usage: ea.exe <file> [name value]\n");
	return 0;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2)
		return Usage();

	bool write = argc > 2;
	if (write && argc < 4)
		return Usage();

	UNICODE_STRING name;
	std::wstring path(argv[1]);
	if (argv[1][1] == L':')
		path = L"\\??\\" + path;
	RtlInitUnicodeString(&name, path.c_str());

	HANDLE hFile;
	IO_STATUS_BLOCK ioStatus;
	OBJECT_ATTRIBUTES attr = RTL_CONSTANT_OBJECT_ATTRIBUTES(&name, 0);
	auto status = NtOpenFile(&hFile, (write ? FILE_WRITE_EA : FILE_READ_EA) | SYNCHRONIZE, &attr,
		&ioStatus, FILE_SHARE_WRITE | FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(status)) {
		printf("Error opening file: %08X\n", status);
		return 0;
	}

	if (write) {
		union {
			FILE_FULL_EA_INFORMATION info{};
			BYTE buffer[1 << 10];
		};
		std::wstring name(argv[2]);
		std::string sname;
		std::transform(name.begin(), name.end(), std::back_inserter(sname), [](wchar_t c) { return (char)c; });
		strcpy_s(info.EaName, sname.length() + 1, sname.c_str());
		info.EaNameLength = (UCHAR)sname.length();
		info.NextEntryOffset = 0;
		info.EaValueLength = USHORT(1 + wcslen(argv[3])) * sizeof(WCHAR);
		wcscpy_s((PWSTR)(info.EaName + info.EaNameLength + 1), info.EaValueLength / sizeof(WCHAR), argv[3]);
		status = NtSetEaFile(hFile, &ioStatus, &info, sizeof(buffer));
	}
	else {
		ULONG size = 0;
		status = NtQueryInformationFile(hFile, &ioStatus, &size, sizeof(size), FileEaInformation);
		if (size == 0) {
			printf("No EA in file\n");
			return 0;
		}
		printf("Total EA size: %u bytes\n", size);

		BYTE buffer[1 << 10];
		for (;;) {
			status = NtQueryEaFile(hFile, &ioStatus, buffer, sizeof(buffer), FALSE,
				nullptr, 0, nullptr, FALSE);
			if (!NT_SUCCESS(status))
				break;

			auto info = (FILE_FULL_EA_INFORMATION*)buffer;
			for (;;) {
				printf("Name: %.*s Value: %.*ws\n",
					info->EaNameLength, info->EaName, 
					(int)(info->EaValueLength / sizeof(WCHAR)),
					(PCWSTR)(info->EaName + info->EaNameLength + 1));
				if (info->NextEntryOffset == 0)
					break;

				info = (PFILE_FULL_EA_INFORMATION)((PBYTE)info + info->NextEntryOffset);
			}
		}
	}
}

