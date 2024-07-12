// nativerun.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "ntdll")

int Error(NTSTATUS status) {
	printf("Error (status=0x%08X)\n", status);
	return 1;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: nativerun [-d] <executable> [arguments...]\n");
		return 0;
	}

	int start = 1;
	bool debug = _wcsicmp(argv[1], L"-d") == 0;
	if (debug)
		start = 2;

	//
	// build command line arguments
	//
	std::wstring args;
	for (int i = start + 1; i < argc; i++) {
		args += argv[i];
		args += L" ";
	}
	UNICODE_STRING cmdline;
	RtlInitUnicodeString(&cmdline, args.c_str());

	UNICODE_STRING name;
	RtlInitUnicodeString(&name, argv[start]);

	PRTL_USER_PROCESS_PARAMETERS params;
	auto status = RtlCreateProcessParameters(&params, &name, nullptr, nullptr, &cmdline,
		nullptr, nullptr, nullptr, nullptr, nullptr);
	if (!NT_SUCCESS(status))
		return Error(status);

	RTL_USER_PROCESS_INFORMATION info;
	status = RtlCreateUserProcess(&name, 0, params, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &info);
	if (!NT_SUCCESS(status))
		return Error(status);

	RtlDestroyProcessParameters(params);

	auto pid = HandleToULong(info.ClientId.UniqueProcess);
	printf("Process 0x%X (%u) created successfully.\n", pid, pid);

	if (debug) {
		printf("Attach with a debugger. Press ENTER to resume thread...\n");
		char dummy[3];
		gets_s(dummy);
	}
	ResumeThread(info.ThreadHandle);
	CloseHandle(info.ThreadHandle);
	CloseHandle(info.ProcessHandle);

	return 0;
}

