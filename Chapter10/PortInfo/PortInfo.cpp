// PortInfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#pragma comment(lib, "ntdll")

// missing flags

#define ALPC_PORFLG_ALLOW_IMPERSONATION     0x00010000
#define ALPC_PORFLG_ACCEPT_REQUESTS         0x00020000
//#define ALPC_PORFLG_WAITABLE_PORT           0x00040000
#define ALPC_PORFLG_ACCEPT_DUP_HANDLES      0x00080000
//#define ALPC_PORFLG_SYSTEM_PROCESS          0x00100000
#define ALPC_PORFLG_SUPPRESS_WAKE           0x00200000
#define ALPC_PORFLG_ALWAYS_WAKE             0x00400000
#define ALPC_PORFLG_DO_NOT_DISTURB          0x00800000
#define ALPC_PORFLG_NO_SHARED_SECTION       0x01000000
#define ALPC_PORFLG_ACCEPT_INDIRECT_HANDLES 0x02000000

std::string PortFlagsToString(ULONG flags) {
	static const struct {
		ULONG flag;
		PCSTR text;
	} details[] = {
		{ ALPC_PORFLG_ALLOW_IMPERSONATION, "Allow Impersonation" },
		{ ALPC_PORFLG_ACCEPT_REQUESTS, "Accept Requests" },
		{ ALPC_PORFLG_WAITABLE_PORT, "Waitable Port" },
		{ ALPC_PORFLG_ACCEPT_DUP_HANDLES, "Accept Dup Handles" },
		{ ALPC_PORFLG_SYSTEM_PROCESS, "System Process" },
		{ ALPC_PORFLG_SUPPRESS_WAKE, "Suppress Wake" },
		{ ALPC_PORFLG_ALWAYS_WAKE, "Always Wake" },
		{ ALPC_PORFLG_DO_NOT_DISTURB, "Do Not Disturb" },
		{ ALPC_PORFLG_NO_SHARED_SECTION, "No Shared Section" },
		{ ALPC_PORFLG_ACCEPT_INDIRECT_HANDLES, "Accept Indirect Handles" },
	};

	std::string sflags;
	for (auto& d : details)
		if (flags & d.flag)
			(sflags += d.text) += ", ";
	if (sflags.empty())
		sflags = "None, ";
	return sflags.substr(0, sflags.length() - 2);
}

void DisplayInfo(HANDLE hPort) {
	if (ALPC_BASIC_INFORMATION bi; 
		NT_SUCCESS(NtAlpcQueryInformation(hPort, AlpcBasicInformation, &bi, sizeof(bi), nullptr))) {
		printf("Context: 0x%p Seq: 0x%X Flags: 0x%08X (%s)\n",
			bi.PortContext, bi.SequenceNo, bi.Flags, PortFlagsToString(bi.Flags).c_str());
	}

	if (ALPC_SERVER_SESSION_INFORMATION info; 
		NT_SUCCESS(NtAlpcQueryInformation(hPort, AlpcServerSessionInformation, &info, sizeof(info), nullptr))) {
		printf("Server PID: %u Session: %u\n",
			info.ProcessId, info.SessionId);
	}
}

int GetAlpcTypeIndex() {
	static int index = -1;
	if (index < 0) {
		PVOID buffer = nullptr;
		SIZE_T size = 1 << 20;
		if (!NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(), &buffer, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
			return -1;
		auto info = (OBJECT_TYPES_INFORMATION*)buffer;
		if (NT_SUCCESS(NtQueryObject(nullptr, ObjectTypesInformation, buffer, (ULONG)size, nullptr))) {
			UNICODE_STRING portTypeName;
			RtlInitUnicodeString(&portTypeName, L"ALPC Port");
			auto type = (OBJECT_TYPE_INFORMATION*)((PBYTE)info + sizeof(PVOID));
			for (ULONG i = 0; i < info->NumberOfTypes; i++) {
				if (RtlEqualUnicodeString(&portTypeName, &type->TypeName, TRUE)) {
					index = type->TypeIndex;
					break;
				}
				auto next = (PBYTE)type + sizeof(*type) + type->TypeName.MaximumLength;
				next += sizeof(PVOID) - 1;
				type = (OBJECT_TYPE_INFORMATION*)((ULONG_PTR)next / sizeof(PVOID) * sizeof(PVOID));
			}
		}
		size = 0;
		NtFreeVirtualMemory(NtCurrentProcess(), &buffer, &size, MEM_RELEASE);
	}
	return index;
}

HANDLE DupHandle(DWORD pid, HANDLE hSrc, ACCESS_MASK access = READ_CONTROL) {
	HANDLE hProcess;
	HANDLE hDup = nullptr;
	CLIENT_ID cid{ ULongToHandle(pid) };
	OBJECT_ATTRIBUTES procAttr;
	InitializeObjectAttributes(&procAttr, nullptr, 0, nullptr, nullptr);
	auto status = NtOpenProcess(&hProcess, PROCESS_DUP_HANDLE, &procAttr, &cid);
	if (NT_SUCCESS(status)) {
		NtDuplicateObject(hProcess, hSrc, NtCurrentProcess(), &hDup, access, 0, 0);
		NtClose(hProcess);
	}
	if (hDup) {
		//
		// make sure it's an ALPC port
		//
		BYTE buffer[512];
		auto info = (OBJECT_TYPE_INFORMATION*)buffer;
		if (!NT_SUCCESS(NtQueryObject(hDup, ObjectTypeInformation, buffer, sizeof(buffer), nullptr))
			|| info->TypeIndex != GetAlpcTypeIndex()) {
			NtClose(hDup);
			hDup = nullptr;
		}
	}
	return hDup;
}

HANDLE FindPort(PCWSTR name) {
	UNICODE_STRING uname;
	RtlInitUnicodeString(&uname, name);
	auto index = GetAlpcTypeIndex();
	PVOID buffer = nullptr;
	SIZE_T size = 1 << 24;
	HANDLE hPort = nullptr;
	if (!NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(), &buffer, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)))
		return nullptr;

	if (NT_SUCCESS(NtQuerySystemInformation(SystemExtendedHandleInformation, buffer, (ULONG)size, nullptr))) {
		auto info = (SYSTEM_HANDLE_INFORMATION_EX*)buffer;
		BYTE nameBuffer[2048];
		for (ULONG_PTR i = 0; i < info->NumberOfHandles; i++) {
			auto& p = info->Handles[i];
			if (p.ObjectTypeIndex != index)
				continue;

			hPort = DupHandle((DWORD)p.UniqueProcessId, (HANDLE)p.HandleValue);
			if (hPort) {
				if (NT_SUCCESS(NtQueryObject(hPort, ObjectNameInformation, nameBuffer, sizeof(nameBuffer), nullptr))) {
					auto oname = (OBJECT_NAME_INFORMATION*)nameBuffer;
					if (RtlEqualUnicodeString(&uname, &oname->Name, TRUE))
						break;
				}
				NtClose(hPort);
				hPort = nullptr;
			}
		}
		size = 0;
		NtFreeVirtualMemory(NtCurrentProcess(), &buffer, &size, MEM_RELEASE);
	}
	return hPort;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: PortInfo <name> | <pid> <handle>\n");
		return 0;
	}

	HANDLE hPort = nullptr;
	auto pid = wcstoul(argv[1], nullptr, 0);
	if (pid == 0) {
		hPort = FindPort(argv[1]);
		if (hPort) {
			DisplayInfo(hPort);
			return 0;
		}
		printf("Failed to locate port name.\n");
		return 1;
	}

	if (argc < 3) {
		printf("Missing handle.\n");
		return 1;
	}

	auto hSrc = UlongToHandle(wcstoul(argv[2], nullptr, 0));
	if (hSrc == nullptr) {
		printf("Invalid handle.\n");
		return 1;
	}
	hPort = DupHandle(pid, hSrc);
	if(hPort) {
		DisplayInfo(hPort);
		return 0;
	}
	printf("Failed to open port handle.\n");
	return 1;
}

