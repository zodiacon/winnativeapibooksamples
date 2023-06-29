// SysInfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <string>
#include <shlwapi.h>

#pragma comment(lib, "ntdll")
#pragma comment(lib, "Shlwapi")

#define NAME_FMT " %-25s"

const char* ProcessorArchToString(USHORT arch) {
	switch (arch) {
		case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
		case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
		case PROCESSOR_ARCHITECTURE_ARM: return "ARM";
		case PROCESSOR_ARCHITECTURE_ARM64: return "ARM64";
	}
	return "<Unknown>";
}

std::wstring TimeToString(LARGE_INTEGER const& time) {
	auto st(time);
	WCHAR text[128];
	DWORD fmt = FDTF_SHORTDATE | FDTF_LONGTIME | FDTF_NOAUTOREADINGORDER;
	return ::SHFormatDateTime((const FILETIME*)&st, &fmt, text, _countof(text)) ? text : L"";
}

void ShowSystemInfo() {
	if (SYSTEM_BASIC_INFORMATION sbi;
		NT_SUCCESS(NtQuerySystemInformation(SystemBasicInformation, &sbi, sizeof(sbi), nullptr))) {
		printf("\nSystemBasicInformation (0)\n");
		printf(NAME_FMT "%u KB\n", "Page Size:", sbi.PageSize >> 10);
		printf(NAME_FMT "%u KB\n", "Allocation Granularity:", sbi.AllocationGranularity >> 10);
		printf(NAME_FMT "%u\n", "Lowest Physical Page:", sbi.LowestPhysicalPageNumber);
		printf(NAME_FMT "%u\n", "Highest Physical Page:", sbi.HighestPhysicalPageNumber);
		printf(NAME_FMT "0x%zX\n", "Min User Mode Address:", sbi.MinimumUserModeAddress);
		printf(NAME_FMT "0x%zX\n", "Max User Mode Address:", sbi.MaximumUserModeAddress);
		printf(NAME_FMT "%u (%u MB)\n", "Physical Pages:", sbi.NumberOfPhysicalPages, sbi.NumberOfPhysicalPages >> 8);
		printf(NAME_FMT "%u (%.3f msec)\n", "Timer Resolution:", sbi.TimerResolution, sbi.TimerResolution / 10000.0);
	}

	if (ULONG maxTime, minTime, curTime;
		NT_SUCCESS(NtQueryTimerResolution(&maxTime, &minTime, &curTime))) {
		printf(NAME_FMT "%u (%.3f msec)\n", "Max Timer Resolution:", maxTime, maxTime / 10000.0);
		printf(NAME_FMT "%u (%.3f msec)\n", "Min Timer Resolution:", minTime, minTime / 10000.0);
		printf(NAME_FMT "%u (%.3f msec)\n", "Cur Timer Resolution:", curTime, curTime / 10000.0);
	}

	if (SYSTEM_PROCESSOR_INFORMATION info;
		NT_SUCCESS(NtQuerySystemInformation(SystemProcessorInformation, &info, sizeof(info), nullptr))) {
		printf("\nSystemProcessorInformation (1)\n");
		printf(NAME_FMT "%u\n", "Processors:", info.MaximumProcessors);
		printf(NAME_FMT "%s\n", "Arch:", ProcessorArchToString(info.ProcessorArchitecture));
		printf(NAME_FMT "%u\n", "Level:", info.ProcessorLevel);
		printf(NAME_FMT "0x%X\n", "Revision:", info.ProcessorRevision);
		printf(NAME_FMT "0x%X\n", "Feature Bits:", info.ProcessorFeatureBits);
	}
	if (SYSTEM_PERFORMANCE_INFORMATION info;
		NT_SUCCESS(NtQuerySystemInformation(SystemPerformanceInformation, &info, sizeof(info), nullptr))) {
		printf("\nSystemPerformanceInformation (2)\n");
		printf(NAME_FMT "%u (%u MB)\n", "Available Pages:", info.AvailablePages, info.AvailablePages >> 8);
		printf(NAME_FMT "%u (%u MB)\n", "Committed Pages:", info.CommittedPages, info.CommittedPages >> 8);
		printf(NAME_FMT "%u (%u MB)\n", "Commit Limit:", info.CommitLimit, info.CommitLimit >> 8);
	}
	if (SYSTEM_TIMEOFDAY_INFORMATION info;
		NT_SUCCESS(NtQuerySystemInformation(SystemTimeOfDayInformation, &info, sizeof(info), nullptr))) {
		printf("\nSystemTimeOfDayInformation (3)\n");
		printf(NAME_FMT "%ws\n", "Boot Time:", TimeToString(info.BootTime).c_str());
		printf(NAME_FMT "%ws\n", "Current Time:", TimeToString(info.CurrentTime).c_str());
		printf(NAME_FMT "%lld minutes\n", "Time Zone Bias:", info.TimeZoneBias.QuadPart / 10000000 / 60);
		printf(NAME_FMT "%llu\n", "Boot Time Bias:", info.BootTimeBias);
		printf(NAME_FMT "%llu\n", "Sleep Time Bias:", info.SleepTimeBias);
		RTL_TIME_ZONE_INFORMATION tzinfo;
		RtlQueryTimeZoneInformation(&tzinfo);
		WCHAR text[64];
		auto hr = ::SHLoadIndirectString(tzinfo.StandardName, text, _countof(text), nullptr);
		printf(NAME_FMT "%ws (%u)\n", "Time Zone:", SUCCEEDED(hr) ? text : tzinfo.StandardName, info.TimeZoneId);
	}
}

int main() {
	ShowSystemInfo();
	return 0;
}

