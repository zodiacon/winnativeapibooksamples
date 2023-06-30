// ThreadList.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

std::wstring TimeToString(LARGE_INTEGER const& time) {
	auto st(time);
	RtlSystemTimeToLocalTime(&st, &st);
	TIME_FIELDS tf;
	RtlTimeToTimeFields(&st, &tf);
	return std::format(L"{:02d}:{:02d}:{:02d}.{:03d}",
		tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

const char* ThreadStateToString(KTHREAD_STATE state) {
	switch (state) {
		case Initialized: return "Initialized";
		case Ready: return "Ready";
		case Running: return "Running";
		case Standby: return "Standby";
		case Terminated: return "Terminated";
		case Waiting: return "Waiting";
		case Transition: return "Transition";
		case DeferredReady: return "Deferred Ready";
		case GateWaitObsolete: return "Gate Wait";
		case WaitingForProcessInSwap: return "Waiting for Process Inswap";
	}
	return "";
}

void EnumThreads(SYSTEM_PROCESS_INFORMATION* p) {
	printf("PID: %6u Threads: %u Name: %wZ\n",
		HandleToULong(p->UniqueProcessId),
		p->NumberOfThreads, p->ImageName);

	auto t = p->Threads;
	for (ULONG i = 0; i < p->NumberOfThreads; i++) {
		printf(" TID: %6u Pri: %2u Created: %ws Address: 0x%p State: %s\n",
			HandleToULong(t->ClientId.UniqueThread),
			t->Priority, TimeToString(t->CreateTime).c_str(),
			t->StartAddress, ThreadStateToString(t->ThreadState));
		t++;
	}
}

void EnumThreads(DWORD pid) {
	ULONG size = 0;
	auto status = NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, &size);
	std::unique_ptr<BYTE[]> buffer;
	while (status == STATUS_INFO_LENGTH_MISMATCH) {
		size += 1024;
		buffer = std::make_unique<BYTE[]>(size);
		status = NtQuerySystemInformation(SystemProcessInformation, buffer.get(), size, &size);
	}

	auto p = (SYSTEM_PROCESS_INFORMATION*)buffer.get();

	for(;;) {
		if (pid == 0 || pid == HandleToULong(p->UniqueProcessId)) {
			EnumThreads(p);
			if (pid)
				break;
		}
		if (p->NextEntryOffset == 0)
			break;
		p = (SYSTEM_PROCESS_INFORMATION*)((PBYTE)p + p->NextEntryOffset);
	}
}

int main(int argc, const char* argv[]) {
	long pid = 0;
	if (argc > 1)
		pid = strtol(argv[1], nullptr, 0);
	if (pid < 0)
		pid = HandleToLong(NtCurrentProcessId());

	EnumThreads(pid);
	return 0;
}
