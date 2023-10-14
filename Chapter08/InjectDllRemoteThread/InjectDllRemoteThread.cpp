// InjectDllRemoteThread.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 3) {
		printf("Usage: InjectDllRemoteThread <pid> <dllpath>\n");
		return 0;
	}

	HANDLE hProcess;
	CLIENT_ID cid{ ULongToHandle(wcstol(argv[1], nullptr, 0)) };
	OBJECT_ATTRIBUTES procAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
	auto status = NtOpenProcess(&hProcess, 
		PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD, 
		&procAttr, &cid); 
	if(!NT_SUCCESS(status)) {
		printf("Error opening process (status: 0x%X)\n", status);
		return status;
	}

	PVOID buffer = nullptr;
	SIZE_T size = 1 << 12;	// 4 KB should be more than enough
	status = NtAllocateVirtualMemory(hProcess, &buffer, 0, &size, 
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!NT_SUCCESS(status)) {
		printf("Error allocating memory (status: 0x%X)\n", status);
		return status;
	}

	status = NtWriteVirtualMemory(hProcess, buffer, (PVOID)argv[2], sizeof(WCHAR) * (wcslen(argv[2]) + 1), nullptr);
	if (!NT_SUCCESS(status)) {
		printf("Error writing to process (status: 0x%X)\n", status);
		return status;
	}

	UNICODE_STRING kernel32Name;
	RtlInitUnicodeString(&kernel32Name, L"kernel32");
	PVOID hK32Dll;
	status = LdrGetDllHandle(nullptr, nullptr, &kernel32Name, &hK32Dll);
	if (!NT_SUCCESS(status)) {
		printf("Error getting kernel32 module (status: 0x%X)\n", status);
		return status;
	}

	ANSI_STRING fname;
	RtlInitAnsiString(&fname, "LoadLibraryW");
	PVOID pLoadLibrary;
	status = LdrGetProcedureAddress(hK32Dll, &fname, 0, &pLoadLibrary);
	if (!NT_SUCCESS(status)) {
		printf("Error locating LoadLibraryW (status: 0x%X)\n", status);
		return status;
	}

	HANDLE hThread;
	status = RtlCreateUserThread(hProcess, nullptr, FALSE, 0, 0, 0,
		(PUSER_THREAD_START_ROUTINE)pLoadLibrary, buffer, 
		&hThread, nullptr);
	if (!NT_SUCCESS(status)) {
		printf("Error creating thread (status: 0x%X)\n", status);
		return status;
	}

	printf("Success!\n");

	NtWaitForSingleObject(hThread, FALSE, nullptr);
	size = 0;
	NtFreeVirtualMemory(hProcess, &buffer, &size, MEM_RELEASE);

	NtClose(hThread);
	NtClose(hProcess);
	return 0;
}

