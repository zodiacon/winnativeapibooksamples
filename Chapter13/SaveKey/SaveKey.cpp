#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>

#pragma comment(lib, "ntdll")

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 3) {
		printf("Usage: SaveKey <key> <file>\n");
		return 0;
	}

	BOOLEAN wasEnabled;
	auto status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &wasEnabled);
	if (!NT_SUCCESS(status)) {
		printf("Failed to enable Backup privilege. Launch with admin access may help.\n");
		return status;
	}

	HANDLE hKey;
	UNICODE_STRING keyName;
	RtlInitUnicodeString(&keyName, argv[1]);
	OBJECT_ATTRIBUTES keyAttr;
	InitializeObjectAttributes(&keyAttr, &keyName, 0, nullptr, nullptr);
	status = NtOpenKey(&hKey, KEY_QUERY_VALUE, &keyAttr);
	if (!NT_SUCCESS(status)) {
		printf("Failed to open key (0x%X)\n", status);
		return status;
	}

	HANDLE hFile;
	OBJECT_ATTRIBUTES fileAttr;
	UNICODE_STRING filename;
	RtlInitUnicodeString(&filename, argv[2]);
	InitializeObjectAttributes(&fileAttr, &filename, 0, nullptr, nullptr);
	IO_STATUS_BLOCK ioStatus;
	status = NtCreateFile(&hFile, FILE_WRITE_DATA | SYNCHRONIZE, &fileAttr, &ioStatus,
		nullptr, 0, 0, FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0);
	if (!NT_SUCCESS(status)) {
		printf("Failed to create file (0x%X)\n", status);
		return status;
	}

	status = NtSaveKey(hKey, hFile);
	if (!NT_SUCCESS(status)) {
		printf("Failed to save key (0x%X)\n", status);
		return status;
	}
	printf("Save successful.\n");

	NtClose(hFile);
	NtClose(hKey);

	return 0;
}
