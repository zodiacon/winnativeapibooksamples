// ProcList.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <assert.h>

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

std::wstring SidToString(PSID sid) {
	UNICODE_STRING str;
	if (NT_SUCCESS(RtlConvertSidToUnicodeString(&str, sid, TRUE))) {
		std::wstring result(str.Buffer, str.Length / sizeof(WCHAR));
		RtlFreeUnicodeString(&str);
		return result;
	}
	return L"";
}

void EnumProcesses() {
	ULONG size = 0;
	auto status = NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, &size);
	std::unique_ptr<BYTE[]> buffer;
	while (status == STATUS_INFO_LENGTH_MISMATCH) {
		size += 1024;
		buffer = std::make_unique<BYTE[]>(size);
		status = NtQuerySystemInformation(SystemProcessInformation, buffer.get(), size, &size);
	}

	auto p = (SYSTEM_PROCESS_INFORMATION*)buffer.get();

	for (;;) {
		printf("PID: %6u PPID: %6u T: %4u H: %6u CPU Time: %ws Created: %ws Name: %wZ\n",
			HandleToULong(p->UniqueProcessId), 
			HandleToULong(p->InheritedFromUniqueProcessId),
			p->NumberOfThreads, p->HandleCount,
			TimeSpanToString(p->UserTime.QuadPart + p->KernelTime.QuadPart).c_str(),
			TimeToString(p->CreateTime).c_str(), &p->ImageName);
		if (p->NextEntryOffset == 0)
			break;
		p = (SYSTEM_PROCESS_INFORMATION*)((PBYTE)p + p->NextEntryOffset);
	}
}

bool EnumProcessesFull() {
	//
	// check if running elevated
	//
	if (ULONG elevated, size; 
		!NT_SUCCESS(NtQueryInformationToken(NtCurrentProcessToken(), TokenElevation, &elevated, sizeof(elevated), &size))
			|| !elevated)
		return false;

	ULONG size = 0;
	auto status = NtQuerySystemInformation(SystemFullProcessInformation, nullptr, 0, &size);
	std::unique_ptr<BYTE[]> buffer;
	while (status == STATUS_INFO_LENGTH_MISMATCH) {
		size += 1024;
		buffer = std::make_unique<BYTE[]>(size);
		status = NtQuerySystemInformation(SystemFullProcessInformation, buffer.get(), size, &size);
	}
	if (!NT_SUCCESS(status))
		return false;

	auto p = (SYSTEM_PROCESS_INFORMATION*)buffer.get();

	for (;;) {
		auto px = (SYSTEM_PROCESS_INFORMATION_EXTENSION*)((PBYTE)p + sizeof(*p) - sizeof(SYSTEM_THREAD_INFORMATION)
			+ p->NumberOfThreads * sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION));
		printf("PID: %6u PPID: %6u T: %4u H: %6u Job: %4u CPU Time: %ws Created: %ws SID: %ws Path: %wZ Package: %ws App: %ws\n",
			HandleToULong(p->UniqueProcessId), HandleToULong(p->InheritedFromUniqueProcessId),
			p->NumberOfThreads, p->HandleCount,
			px->JobObjectId,
			TimeSpanToString(p->UserTime.QuadPart + p->KernelTime.QuadPart).c_str(),
			TimeToString(p->CreateTime).c_str(), 
			px->UserSidOffset ? SidToString((PSID)((PBYTE)px + px->UserSidOffset)).c_str() : L"",
			&p->ImageName, 
			px->PackageFullNameOffset ? (PCWSTR)((PBYTE)px + px->PackageFullNameOffset) : L"",
			px->AppIdOffset ? (PCWSTR)((PBYTE)px + px->AppIdOffset) : L"");

		assert(!px->HasStrongId || (px->HasStrongId && px->PackageFullNameOffset > 0));

		if (p->NextEntryOffset == 0)
			break;
		p = (SYSTEM_PROCESS_INFORMATION*)((PBYTE)p + p->NextEntryOffset);
	}
	return true;
}

int main(int argc, const char* argv[]) {
	if (!EnumProcessesFull())
		EnumProcesses();

	return 0;
}

