// Streams.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"


#pragma comment(lib, "ntdll")

NTSTATUS EnumStreams(PCWSTR filename) {
	std::wstring path(filename);
	if (filename[1] == L':')
		path = L"\\??\\" + path;

	UNICODE_STRING name;
	RtlInitUnicodeString(&name, path.c_str());
	HANDLE hFile;
	IO_STATUS_BLOCK ioStatus;
	OBJECT_ATTRIBUTES attr = RTL_CONSTANT_OBJECT_ATTRIBUTES(&name, 0);
	auto status = NtOpenFile(&hFile, FILE_READ_ACCESS | SYNCHRONIZE, &attr, &ioStatus,
		FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(status))
		return status;

	BYTE buffer[1 << 10];
	status = NtQueryInformationFile(hFile, &ioStatus, buffer, sizeof(buffer), FileStreamInformation);
	if (!NT_SUCCESS(status))
		return status;

	auto info = (FILE_STREAM_INFORMATION*)buffer;
	for (;;) {
		printf("Name: %.*ws Size: %llu bytes\n",
			int(info->StreamNameLength / sizeof(WCHAR)), info->StreamName, 
			info->StreamSize.QuadPart);
		if (info->NextEntryOffset == 0)
			break;

		info = (FILE_STREAM_INFORMATION*)((PBYTE)info + info->NextEntryOffset);
	}

	NtClose(hFile);
	return status;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: Streams <file>\n");
		return 0;
	}

	if (!NT_SUCCESS(EnumStreams(argv[1]))) {
		printf("Error enumerating streams\n");
	}
	return 0;
}
