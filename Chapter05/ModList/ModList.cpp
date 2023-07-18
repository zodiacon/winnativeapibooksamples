// ModList.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

void ListModules(LIST_ENTRY* head, int linkOffset) {
	for (auto next = head->Flink; next != head; next = next->Flink) {
		auto data = (PLDR_DATA_TABLE_ENTRY)((PBYTE)next - linkOffset);
		printf("0x%p: %wZ (%wZ) \n", data->DllBase, &data->BaseDllName, &data->FullDllName);
	}
}

int main(int argc, const char* argv[]) {
	auto peb = NtCurrentPeb();
	printf("Load Order\n");
	ListModules(&peb->Ldr->InLoadOrderModuleList, offsetof(LDR_DATA_TABLE_ENTRY, InLoadOrderLinks));

	printf("\nMemory Order\n");
	ListModules(&peb->Ldr->InMemoryOrderModuleList, offsetof(LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

	printf("\nInitialization Order\n");
	ListModules(&peb->Ldr->InInitializationOrderModuleList, offsetof(LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks));

	return 0;
}
