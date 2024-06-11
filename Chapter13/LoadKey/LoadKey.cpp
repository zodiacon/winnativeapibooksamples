#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>

#pragma comment(lib, "ntdll")

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 3) {
		printf("Usage: LoadKey <key> <file>\n");
		return 0;
	}

	BOOLEAN wasEnabled;
	auto status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &wasEnabled);
	if (!NT_SUCCESS(status)) {
		printf("Failed to enable Restore privilege. Launching with admin rights may help.\n");
		return status;
	}

	OBJECT_ATTRIBUTES fileAttr;
	UNICODE_STRING filename;
	RtlInitUnicodeString(&filename, argv[2]);
	InitializeObjectAttributes(&fileAttr, &filename, 0, nullptr, nullptr);

	UNICODE_STRING keyName;
	RtlInitUnicodeString(&keyName, argv[1]);
	OBJECT_ATTRIBUTES keyAttr;
	InitializeObjectAttributes(&keyAttr, &keyName, 0, nullptr, nullptr);

	status = NtLoadKey(&keyAttr, &fileAttr);
	if (!NT_SUCCESS(status)) {
		printf("Failed to load key (0x%X)\n", status);
		return status;
	}

	printf("Load successful.\n");

	return 0;
}
