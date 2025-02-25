// Handles.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

std::string HandleAttributesToString(ULONG attr) {
	std::string result;
	if (attr & OBJ_INHERIT)
		result += "I";
	if (attr & OBJ_PROTECT_CLOSE)
		result += "P";
	if (attr & OBJ_AUDIT_OBJECT_CLOSE)
		result += "A";
	return result;
}

NTSTATUS GetObjectName(HANDLE h, DWORD pid, std::wstring& name, bool file) {
	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	CLIENT_ID cid{};
	cid.UniqueProcess = ULongToHandle(pid);
	HANDLE hProcess;
	auto status = NtOpenProcess(&hProcess, PROCESS_DUP_HANDLE, &procAttr, &cid);
	if (!NT_SUCCESS(status))
		return status;

	HANDLE hDup;
	status = NtDuplicateObject(hProcess, h, NtCurrentProcess(), &hDup, READ_CONTROL, 0, 0);
	if (NT_SUCCESS(status)) {
		static BYTE buffer[1024];	// hopefully large enough :)
		auto sname = (UNICODE_STRING*)buffer;
		if (file) {
			//
			// special case for files
			// get name on a separate thread
			// kill thread if not available after some waiting
			//
			HANDLE hThread;
			status = NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, &procAttr, NtCurrentProcess(),
				(PUSER_THREAD_START_ROUTINE)[](auto param) -> NTSTATUS {
					auto h = (HANDLE)param;
					auto status = NtQueryObject(h, ObjectNameInformation, buffer, sizeof(buffer), nullptr);
					return status;
				}, hDup, 0, 0, 0, 0, nullptr);
			if (NT_SUCCESS(status)) {
				LARGE_INTEGER interval;
				interval.QuadPart = -50 * 10000;	// 50 msec
				status = NtWaitForSingleObject(hThread, FALSE, &interval);
				if (status == STATUS_TIMEOUT) {
					NtTerminateThread(hThread, 1);
				}
				else {
					THREAD_BASIC_INFORMATION tbi;
					NtQueryInformationThread(hThread, ThreadBasicInformation, &tbi, sizeof(tbi), nullptr);
					status = tbi.ExitStatus;
				}
				NtClose(hThread);
			}
		}
		else {
			status = NtQueryObject(hDup, ObjectNameInformation, sname, sizeof(buffer), nullptr);
		}
		if (NT_SUCCESS(status))
			name.assign(sname->Buffer, sname->Length / sizeof(WCHAR));
		NtClose(hDup);
	}
	NtClose(hProcess);
	return status;
}

std::wstring const& GetObjectTypeName(USHORT index) {
	static std::vector<std::wstring> typeNames;
	if (typeNames.empty()) {
		ULONG size = 1 << 14;
		auto buffer = std::make_unique<BYTE[]>(size);
		if (!NT_SUCCESS(NtQueryObject(nullptr, ObjectTypesInformation, buffer.get(), size, nullptr))) {
			static std::wstring empty;
			return empty;
		}
		auto p = (OBJECT_TYPES_INFORMATION*)buffer.get();
		auto type = (OBJECT_TYPE_INFORMATION*)((PBYTE)p + sizeof(ULONG_PTR));
		typeNames.resize(p->NumberOfTypes);
		for (ULONG i = 0; i < p->NumberOfTypes; i++) {
			if(type->TypeIndex >= typeNames.size())
				typeNames.resize(type->TypeIndex + 1);
			typeNames[type->TypeIndex] = std::wstring(type->TypeName.Buffer, type->TypeName.Length / sizeof(WCHAR));
			auto offset = sizeof(OBJECT_TYPE_INFORMATION) + type->TypeName.MaximumLength;
			if(offset % sizeof(ULONG_PTR))
				offset += sizeof(ULONG_PTR) - ((ULONG_PTR)type + offset) % sizeof(ULONG_PTR);
			type = (OBJECT_TYPE_INFORMATION*)((PBYTE)type + offset);
		}
	}

	return typeNames[index];
}

void DisplayHandle(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX const& h) {
	auto const& type = GetObjectTypeName(h.ObjectTypeIndex);
	printf("PID: %6u H: 0x%X Access: 0x%08X Att: %s Address: 0x%p Type: %ws",
		HandleToULong(h.UniqueProcessId), HandleToULong(h.HandleValue), h.GrantedAccess,
		HandleAttributesToString(h.HandleAttributes).c_str(),
		h.Object, type.c_str());
	std::wstring name;
	auto status = GetObjectName(h.HandleValue, HandleToULong(h.UniqueProcessId), name, type == L"File");
	if (NT_SUCCESS(status) && !name.empty())
		printf(" Name: %ws\n", name.c_str());
	else if (status == STATUS_ACCESS_DENIED)
		printf(" Name: <access denied>\n");
	else
		printf("\n");
}

void EnumHandles(DWORD pid, PCWSTR type) {
	std::unique_ptr<BYTE[]> buffer;
	ULONG size = 1 << 20;
	NTSTATUS status;
	do {
		buffer = std::make_unique<BYTE[]>(size);
		status = NtQuerySystemInformation(SystemExtendedHandleInformation, buffer.get(), size, nullptr);
		if (NT_SUCCESS(status))
			break;
		size *= 2;
	} while (status == STATUS_INFO_LENGTH_MISMATCH);

	if (!NT_SUCCESS(status))
		return;

	auto p = (SYSTEM_HANDLE_INFORMATION_EX*)buffer.get();
	for (ULONG_PTR i = 0; i < p->NumberOfHandles; i++) {
		auto& h = p->Handles[i];
		if (pid && HandleToULong(h.UniqueProcessId) != pid)
			continue;
		if (type && _wcsicmp(type, GetObjectTypeName(h.ObjectTypeIndex).c_str()) != 0)
			continue;

		DisplayHandle(h);
	}
}

int wmain(int argc, const wchar_t* argv[]) {
	PCWSTR type = nullptr;
	long pid = 0;
	if (argc > 1) {
		pid = wcstol(argv[1], nullptr, 0);
		if (pid == 0)
			type = argv[1];
	}
	if (argc > 2)
		type = argv[2];
	EnumHandles(pid, type);

	return 0;
}
