#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <memory>
#include <string>
#include <format>

#pragma comment(lib, "ntdll")

std::string FormatTime(LARGE_INTEGER& dt) {
	TIME_FIELDS tf;
	RtlTimeToTimeFields(&dt, &tf);
	return std::format("{:04}/{:02}/{:02} {:02}:{:02}:{:02}.{:03}",
		tf.Year, tf.Month, tf.Day, tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

void DisplayInfo(HANDLE hKey) {
	BYTE buffer[1024];
	ULONG len;
	if (NT_SUCCESS(NtQueryKey(hKey, KeyNameInformation, buffer, sizeof(buffer), &len))) {
		auto info = (KEY_NAME_INFORMATION*)buffer;
		printf("Full Name: %S\n", std::wstring(info->Name, info->NameLength / sizeof(WCHAR)).c_str());
	}
	if (NT_SUCCESS(NtQueryKey(hKey, KeyBasicInformation, buffer, sizeof(buffer), &len))) {
		auto info = (KEY_BASIC_INFORMATION*)buffer;
		printf("Name: %S\n", std::wstring(info->Name, info->NameLength / sizeof(WCHAR)).c_str());
		printf("Write time: %s\n", FormatTime(info->LastWriteTime).c_str());
	}
	if (NT_SUCCESS(NtQueryKey(hKey, KeyFullInformation, buffer, sizeof(buffer), &len))) {
		auto info = (KEY_FULL_INFORMATION*)buffer;
		if (info->ClassLength) {
			printf("Class: %S\n", std::wstring(info->Class, info->ClassLength / sizeof(WCHAR)).c_str());
		}
		printf("Subkeys: %lu\n", info->SubKeys);
		printf("Values: %lu\n", info->Values);
		printf("Max class length: %lu\n", info->MaxClassLength);
		printf("Max name length: %lu\n", info->MaxNameLength);
		printf("Max value name length: %lu\n", info->MaxValueNameLength);
		printf("Max data length: %lu\n", info->MaxValueDataLength);
	}
	else if (NT_SUCCESS(NtQueryKey(hKey, KeyCachedInformation, buffer, sizeof(buffer), &len))) {
		auto info = (KEY_CACHED_INFORMATION*)buffer;
		printf("Subkeys: %lu\n", info->SubKeys);
		printf("Values: %lu\n", info->Values);
		printf("Max name length: %lu\n", info->MaxNameLength);
		printf("Max value name length: %lu\n", info->MaxValueNameLength);
		printf("Max data length: %lu\n", info->MaxValueDataLength);
	}
	if (NT_SUCCESS(NtQueryKey(hKey, KeyFlagsInformation, buffer, sizeof(buffer), &len))) {
		auto info = (KEY_FLAGS_INFORMATION*)buffer;
		printf("Wow64 flags: 0x%lX\n", info->Wow64Flags);
		printf("Key flags: 0x%lX\n", info->KeyFlags);
		printf("Control flags: 0x%lX\n", info->ControlFlags);
	}
	if (NT_SUCCESS(NtQueryKey(hKey, KeyTrustInformation, buffer, sizeof(buffer), &len))) {
		auto info = (KEY_TRUST_INFORMATION*)buffer;
		printf("Trusted: %s\n", info->TrustedKey ? "Yes" : "No");
	}
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: keyinfo <key>\n");
		return 0;
	}

	HANDLE hKey;
	UNICODE_STRING keyName;
	RtlInitUnicodeString(&keyName, argv[1]);
	OBJECT_ATTRIBUTES keyAttr;
	InitializeObjectAttributes(&keyAttr, &keyName, 0, nullptr, nullptr);
	auto status = NtOpenKey(&hKey, KEY_QUERY_VALUE, &keyAttr);
	if (!NT_SUCCESS(status)) {
		printf("Failed to open key (0x%lX)\n", status);
		return status;
	}

	DisplayInfo(hKey);
	NtClose(hKey);
}

