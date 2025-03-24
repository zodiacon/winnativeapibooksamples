// Threads.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>

#pragma comment(lib, "ntdll")

NTSTATUS NTAPI DoWork(PVOID param) {
	LARGE_INTEGER time;
	NtQuerySystemTime(&time);
	ULONG tid = HandleToULong(NtCurrentThreadId());
	printf("Worker thread %u started at %llu ms\n", tid, time.QuadPart / 10000);
	int data = *(int*)param;
	LARGE_INTEGER interval;
	interval.QuadPart = -4LL * 10000000;	// 4 sec
	NtDelayExecution(FALSE, &interval);
	NtQuerySystemTime(&time);
	printf("Worker thread %u done    at %llu ms\n", tid, time.QuadPart / 10000);
	return data * 2;
}

int main() {
	HANDLE hThread;
	int x = 10;
	auto status = RtlCreateUserThread(NtCurrentProcess(), nullptr, FALSE, 0, 
		256 << 10, 16 << 10, DoWork, &x, &hThread, nullptr);
	if (!NT_SUCCESS(status))
		return status;

	NtWaitForSingleObject(hThread, FALSE, nullptr);
	THREAD_BASIC_INFORMATION info;
	if (NT_SUCCESS(NtQueryInformationThread(hThread, ThreadBasicInformation, &info, sizeof(info), nullptr))) {
		printf("Result: %d\n", info.ExitStatus);
	}
	NtClose(hThread);

	return 0;
}

