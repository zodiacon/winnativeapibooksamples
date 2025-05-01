#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <memory>
#include <string>
#include <utility>

#pragma comment(lib, "ntdll")

PCSTR RegistryTypeToString(DWORD type) {
	switch (type) {
		case REG_SZ: return "REG_SZ";
		case REG_DWORD: return "REG_DWORD";
		case REG_MULTI_SZ: return "REG_MULTI_SZ";
		case REG_QWORD: return "REG_QDWORD";
		case REG_EXPAND_SZ: return "REG_EXPAND_SZ";
		case REG_NONE: return "REG_NONE";
		case REG_LINK: return "REG_LINK";
		case REG_BINARY: return "REG_BINARY";
		case REG_RESOURCE_REQUIREMENTS_LIST: return "REG_RESOURCE_REQUIREMENTS_LIST";
		case REG_RESOURCE_LIST: return "REG_RESOURCE_LIST";
		case REG_FULL_RESOURCE_DESCRIPTOR: return "REG_FULL_RESOURCE_DESCRIPTOR";
	}
	return "<unknown>";
}

void DisplayData(KEY_VALUE_FULL_INFORMATION const* info) {
	auto p = (PBYTE)info + info->DataOffset;
	switch (info->Type) {
		case REG_SZ:
		case REG_EXPAND_SZ:
			printf("%ws\n", (PCWSTR)p);
			break;

		case REG_MULTI_SZ:
		{
			auto s = (PCWSTR)p;
			while (*s) {
				printf("%ws ", s);
				s += wcslen(s) + 1;
			}
			printf("\n");
			break;
		}

		case REG_DWORD:
			printf("%u (0x%X)\n", *(DWORD*)p, *(DWORD*)p);
			break;

		case REG_QWORD:
			printf("%llu (0x%llX)\n", *(ULONGLONG*)p, *(ULONGLONG*)p);
			break;

		case REG_BINARY:
		case REG_FULL_RESOURCE_DESCRIPTOR:
		case REG_RESOURCE_LIST:
		case REG_RESOURCE_REQUIREMENTS_LIST:
			auto len = std::min(64ul, info->DataLength);
			for (DWORD i = 0; i < len; i++) {
				printf("%02X ", p[i]);
			}
			printf("\n");
			break;
	}
}

NTSTATUS EnumerateKeyValues(HANDLE hKey) {
	ULONG len;
	for (ULONG i = 0; ; i++) {
		// first call to get size
		auto status = NtEnumerateValueKey(hKey, i, KeyValueFullInformation, nullptr, 0, &len);
		if (status == STATUS_NO_MORE_ENTRIES)
			break;

		auto buffer = std::make_unique<BYTE[]>(len);
		// make the real call
		status = NtEnumerateValueKey(hKey, i, KeyValueFullInformation, buffer.get(), len, &len);
		if (!NT_SUCCESS(status))
			continue;

		auto info = (KEY_VALUE_FULL_INFORMATION*)buffer.get();

		printf("Name: %ws Type: %s (%u) Data Size: %u bytes Data: ",
			std::wstring(info->Name, info->NameLength / sizeof(WCHAR)).c_str(),
			RegistryTypeToString(info->Type), info->Type,
			info->DataLength);

		DisplayData(info);
	}
	return STATUS_SUCCESS;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: enumkeyvalues <key>\n");
		return 0;
	}

	HANDLE hKey;
	UNICODE_STRING keyName;
	RtlInitUnicodeString(&keyName, argv[1]);
	OBJECT_ATTRIBUTES keyAttr;
	InitializeObjectAttributes(&keyAttr, &keyName, 0, nullptr, nullptr);
	auto status = NtOpenKey(&hKey, KEY_QUERY_VALUE, &keyAttr);
	if (!NT_SUCCESS(status)) {
		printf("Failed to open key (0x%X)\n", status);
		return status;
	}

	EnumerateKeyValues(hKey);
	NtClose(hKey);
}

