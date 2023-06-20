// ModList.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>

int main() {
	PPEB peb = NtCurrentPeb();
	auto& head = peb->Ldr->InLoadOrderModuleList;

	for (auto next = head.Flink; next != &head; next = next->Flink) {
		auto mod = CONTAINING_RECORD(next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		printf("0x%p: %wZ\n", mod->DllBase, mod->BaseDllName);
	}
	return 0;
}
