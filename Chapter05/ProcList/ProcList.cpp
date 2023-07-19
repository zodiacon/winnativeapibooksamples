// ProcList.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

std::string ProcessFlagsToString(PROCESS_EXTENDED_BASIC_INFORMATION const& ebi) {
	std::string flags;
	if (ebi.IsProtectedProcess)
		flags += "Protected, ";
	if (ebi.IsFrozen)
		flags += "Frozen, ";
	if (ebi.IsSecureProcess)
		flags += "Secure, ";
	if (ebi.IsCrossSessionCreate)
		flags += "Cross Session, ";
	if (ebi.IsBackground)
		flags += "Background, ";
	if (ebi.IsSubsystemProcess)
		flags += "WSL, ";
	if (ebi.IsStronglyNamed)
		flags += "Strong Name, ";
	if (ebi.IsProcessDeleting)
		flags += "Deleting, ";
	if (ebi.IsWow64Process)
		flags += "Wow64, ";

	if (!flags.empty())
		return flags.substr(0, flags.length() - 2);
	return "";
}

int main() {
	HANDLE hProcess = nullptr;
	for (;;) {
		HANDLE hNewProcess;
		auto status = NtGetNextProcess(hProcess, PROCESS_QUERY_LIMITED_INFORMATION, 0, 0, &hNewProcess);
		//
		// close previous handle
		//
		if(hProcess)
			NtClose(hProcess);

		if (!NT_SUCCESS(status))
			break;
		
		PROCESS_EXTENDED_BASIC_INFORMATION ebi;
		if (NT_SUCCESS(NtQueryInformationProcess(hNewProcess, ProcessBasicInformation, &ebi, sizeof(ebi), nullptr))) {
			auto& bi = ebi.BasicInfo;
			printf("PID: %6u PPID: %6u Pri: %2u PEB: 0x%p %s\n",
				HandleToULong(bi.UniqueProcessId),
				HandleToULong(bi.InheritedFromUniqueProcessId),
				bi.BasePriority, bi.PebBaseAddress,
				ProcessFlagsToString(ebi).c_str());
		}
		
		hProcess = hNewProcess;
	}
	return 0;
}

