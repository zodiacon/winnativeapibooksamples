#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <memory>

#pragma comment(lib, "ntdll")

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: KeyHandles <key>\n");
		return 0;
	}

	UNICODE_STRING keyName;
	RtlInitUnicodeString(&keyName, argv[1]);
	OBJECT_ATTRIBUTES keyAttr;
	InitializeObjectAttributes(&keyAttr, &keyName, 0, nullptr, nullptr);

	ULONG count;
	auto status = NtQueryOpenSubKeys(&keyAttr, &count);
	if (NT_SUCCESS(status)) {
		printf("Handle count: %u\n", count);

		BOOLEAN wasEnabled;
		status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &wasEnabled);
		if (!NT_SUCCESS(status)) {
			printf("Failed to enable Restore privilege. Launching with admin rights may help.\n");
			return status;
		}

		ULONG needed;
		KEY_OPEN_SUBKEYS_INFORMATION dummy;
		status = NtQueryOpenSubKeysEx(&keyAttr, sizeof(dummy), &dummy, &needed);
		auto buffer = std::make_unique<BYTE[]>(needed += 500);
		status = NtQueryOpenSubKeysEx(&keyAttr, needed, buffer.get(), &needed);
		if (NT_SUCCESS(status)) {
			auto info = (KEY_OPEN_SUBKEYS_INFORMATION*)buffer.get();
			for (ULONG i = 0; i < info->Count; i++) {
				printf("PID: %7u Key: %wZ\n",
					HandleToULong(info->KeyArray[i].ProcessId), 
					&info->KeyArray[i].KeyName);
			}
		}
	}
	else if (status == STATUS_ACCESS_DENIED) {
		printf("Access denied.\n");
	}
	else {
		printf("Error: 0x%X\n", status);
	}
	return 0;
}

