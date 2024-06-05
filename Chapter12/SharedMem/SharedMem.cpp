// SharedMem.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#define PHNT_VERSION PHNT_REDSTONE5

#include <phnt.h>
#include <stdio.h>
#include <string>
#include <format>

#pragma comment(lib, "ntdll")

std::wstring GetBaseDirectory() {
	ULONG session;
	if (!NT_SUCCESS(NtQueryInformationProcess(NtCurrentProcess(), ProcessSessionInformation, 
		&session, sizeof(session), nullptr)))
		return L"";

	return std::format(L"\\Sessions\\{}\\BaseNamedObjects\\", session);
}

int main() {
	auto baseDir = GetBaseDirectory();
	if (baseDir.empty())
		return 1;

	HANDLE hSection;
	OBJECT_ATTRIBUTES secAttr;
	UNICODE_STRING secName;
	auto fullSecName = baseDir + L"MySharedMem";
	RtlInitUnicodeString(&secName, fullSecName.c_str());
	InitializeObjectAttributes(&secAttr, &secName, OBJ_OPENIF, nullptr, nullptr);
	LARGE_INTEGER size;
	size.QuadPart = 1 << 13;	// 8KB

	auto status = NtCreateSection(&hSection, SECTION_ALL_ACCESS, 
		&secAttr, &size, PAGE_READWRITE, SEC_COMMIT, nullptr);
	if (!NT_SUCCESS(status)) {
		printf("Failed to create/open section (0x%X)\n", status);
		return status;
	}

	bool wait = false;
	if (status == STATUS_OBJECT_NAME_EXISTS) {
		//
		// already created by some other process
		//
		wait = true;
	}

	PVOID address = nullptr;
	SIZE_T viewSize = 0;
	status = NtMapViewOfSection(hSection, NtCurrentProcess(), &address, 
		0, 0, nullptr, &viewSize, ViewUnmap, 0, PAGE_READWRITE);
	if (!NT_SUCCESS(status)) {
		printf("Failed to map section (0x%X)\n", status);
		return status;
	}

	OBJECT_ATTRIBUTES evtAttr;
	UNICODE_STRING evtName;
	auto fullEvtName = baseDir + L"MySharedMemDataReady";
	RtlInitUnicodeString(&evtName, fullEvtName.c_str());
	InitializeObjectAttributes(&evtAttr, &evtName, OBJ_OPENIF, nullptr, nullptr);
	HANDLE hEvent;
	status = NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, &evtAttr, SynchronizationEvent, FALSE);
	if (!NT_SUCCESS(status)) {
		printf("Failed to create/open event (0x%X)\n", status);
		return status;
	}

	char text[128];
	for(;;) {
		if (wait) {
			printf("Waiting for data...\n");
			NtWaitForSingleObject(hEvent, FALSE, nullptr);
			printf("%s\n", (PCSTR)address);
		}
		else {
			printf("> ");
			gets_s(text);
			strcpy_s((PSTR)address, sizeof(text), text);
			NtSetEvent(hEvent, nullptr);
			if (strcmp(text, "quit") == 0)
				break;
		}
		wait = !wait;
	}

	NtUnmapViewOfSection(NtCurrentProcess(), address);
	NtClose(hEvent);
	NtClose(hSection);

	return 0;
}
