// Handles.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

USHORT g_FileTypeIndex;

struct EnumHandleOptions {
	std::vector<ULONG> ProcessIds;
	std::vector<std::wstring> TypeNames;
	bool NamedOnly;
};

struct HandleInfo : SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX {
	std::wstring Name;
};

std::vector<HandleInfo> EnumHandles(EnumHandleOptions const& options) {
	return {};
}

std::wstring GetFileObjectName(HANDLE hFile) {
	struct Data {
		std::wstring Name;
		HANDLE hFile;
	} data;

	HANDLE hThread;
	OBJECT_ATTRIBUTES threadAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	data.hFile = hFile;
	auto status = RtlCreateUserThread(NtCurrentProcess(), nullptr, FALSE, 0, 0, 0, [](auto p) -> NTSTATUS {
		auto data = (Data*)p;
		auto buffer = std::make_unique<BYTE[]>(MAX_PATH * 4);
		auto status = NtQueryObject(data->hFile, ObjectNameInformation, buffer.get(), MAX_PATH * 4, nullptr);
		if (!NT_SUCCESS(status))
			return 1;

		auto uname = (UNICODE_STRING*)buffer.get();
		data->Name.assign(uname->Buffer, uname->Length / sizeof(WCHAR));
		return 0;
		}, &data, &hThread, nullptr);
	if (!NT_SUCCESS(status))
		return L"";

	LARGE_INTEGER timeout;
	timeout.QuadPart = -10 * 10000;		// 10 msec
	if (STATUS_TIMEOUT == NtWaitForSingleObject(hThread, FALSE, &timeout))
		NtTerminateThread(hThread, 1);
	NtClose(hThread);
	return data.Name;
}

std::wstring GetObjectName(HANDLE hObject, ULONG pid, USHORT type) {
	static const WCHAR accessDeniedName[] = L"<access denied>";
	HANDLE hProcess;
	CLIENT_ID cid{ ULongToHandle(pid) };
	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	auto status = NtOpenProcess(&hProcess, PROCESS_DUP_HANDLE, &procAttr, &cid);
	if (!NT_SUCCESS(status))
		return accessDeniedName;

	HANDLE hDup = nullptr;
	std::wstring name;
	do {
		status = NtDuplicateObject(hProcess, hObject, NtCurrentProcess(), &hDup, 0, 0, 0);
		if (!NT_SUCCESS(status)) {
			name = accessDeniedName;
			break;
		}

		if (type == g_FileTypeIndex) {
			//
			// special handling for file objects
			//
			name = GetFileObjectName(hDup);
			break;
		}

		OBJECT_BASIC_INFORMATION info;
		status = NtQueryObject(hDup, ObjectBasicInformation, &info, sizeof(info), nullptr);
		if (!NT_SUCCESS(status)) {
			name = status == STATUS_ACCESS_DENIED ? accessDeniedName : L"";
			break;
		}

		ULONG size = info.NameInfoSize ? info.NameInfoSize : 512;
		auto buffer = std::make_unique<BYTE[]>(size);
		status = NtQueryObject(hDup, ObjectNameInformation, buffer.get(), size, nullptr);
		if (!NT_SUCCESS(status))
			break;

		auto uname = (UNICODE_STRING*)buffer.get();
		name.assign(uname->Buffer, uname->Length / sizeof(WCHAR));
	} while (false);

	if (hProcess)
		NtClose(hProcess);
	if (hDup)
		NtClose(hDup);

	return name;
}

void DisplayHandleInfo(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX const& info) {
	printf("H: 0x%06zX PID: %6zu Obj: 0x%p Type: %2d Access: 0x%08X Attr: 0x%02X Name: %ws\n",
		info.HandleValue, info.UniqueProcessId, info.Object,
		info.ObjectTypeIndex, info.GrantedAccess, info.HandleAttributes,
		GetObjectName((HANDLE)info.HandleValue, (ULONG)info.UniqueProcessId, info.ObjectTypeIndex).c_str());
}

USHORT GetFileObjectTypeIndex() {
	auto size = 1 << 16;
	auto buffer = std::make_unique<BYTE[]>(size);
	if (NT_SUCCESS(NtQueryObject(nullptr, ObjectTypesInformation, buffer.get(), size, nullptr))) {
		auto types = (OBJECT_TYPES_INFORMATION*)buffer.get();
		auto type = (OBJECT_TYPE_INFORMATION*)((PBYTE)types + sizeof(PVOID));
		for (ULONG i = 0; i < types->NumberOfTypes; i++) {
			if (wcsncmp(type->TypeName.Buffer, L"File", 4) == 0)
				return type->TypeIndex;

			auto temp = (PBYTE)type + sizeof(OBJECT_TYPE_INFORMATION) + type->TypeName.MaximumLength;
			type = (OBJECT_TYPE_INFORMATION*)(((ULONG_PTR)temp + sizeof(PVOID) - 1) / sizeof(PVOID) * sizeof(PVOID));
		}
	}
	return 0;
}

int SimpleEnumHandles() {
	ULONG size = 0;
	std::unique_ptr<BYTE[]> buffer;
	auto status = STATUS_SUCCESS;
	do {
		if (size)
			buffer = std::make_unique<BYTE[]>(size);
		status = NtQuerySystemInformation(SystemExtendedHandleInformation, buffer.get(), size, &size);
		if (status == STATUS_INFO_LENGTH_MISMATCH) {
			size += 1 << 12;
			continue;
		}
	} while (!NT_SUCCESS(status));

	if (!NT_SUCCESS(status)) {
		printf("Error: 0x%X\n", status);
		return 1;
	}

	g_FileTypeIndex = GetFileObjectTypeIndex();
	auto handles = (SYSTEM_HANDLE_INFORMATION_EX*)buffer.get();
	for (ULONG i = 0; i < handles->NumberOfHandles; i++) {
		DisplayHandleInfo(handles->Handles[i]);
	}
	return 0;
}

int main(int argc, const char* argv[]) {
	return SimpleEnumHandles();
}
