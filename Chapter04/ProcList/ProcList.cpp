// ProcList.cpp : This file contains the 'main' function. Program execution begins and ends there.
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

std::wstring TimeSpanToString(LONGLONG ts) {
	TIME_FIELDS tf;
	RtlTimeToElapsedTimeFields((PLARGE_INTEGER)&ts, &tf);
	return std::format(L"{:02d}:{:02d}:{:02d}.{:03d}",
		tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

int main() {
	ULONG size = 0;
	auto status = NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, &size);
	std::unique_ptr<BYTE[]> buffer;
	while(status == STATUS_INFO_LENGTH_MISMATCH) {
		size += 1024;
		buffer = std::make_unique<BYTE[]>(size);
		status = NtQuerySystemInformation(SystemProcessInformation, buffer.get(), size, &size);
	}

	auto p = (SYSTEM_PROCESS_INFORMATION*)buffer.get();

	do {
		printf("PID: %6u PPID: %6u T: %4u H: %6u CPU Time: %ws Created: %ws Name: %wZ\n",
			HandleToULong(p->UniqueProcessId), HandleToULong(p->InheritedFromUniqueProcessId),
			p->NumberOfThreads, p->HandleCount,
			TimeSpanToString(p->UserTime.QuadPart + p->KernelTime.QuadPart).c_str(),
			TimeToString(p->CreateTime).c_str(), p->ImageName);
		p = (SYSTEM_PROCESS_INFORMATION*)((PBYTE)p + p->NextEntryOffset);
	} while (p->NextEntryOffset);

	return 0;
}

