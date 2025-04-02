// ObjectTypes.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

int main() {
	auto size = 1 << 16;
	auto buffer = std::make_unique<BYTE[]>(size);
	if (NT_SUCCESS(NtQueryObject(nullptr, ObjectTypesInformation, buffer.get(), size, nullptr))) {
		auto types = (OBJECT_TYPES_INFORMATION*)buffer.get();
		//
		// first type starts at offset of pointer-size bytes 
		//
		auto type = (OBJECT_TYPE_INFORMATION*)((PBYTE)types + sizeof(PVOID));
		for (ULONG i = 0; i < types->NumberOfTypes; i++) {
			printf("%-33wZ (%2d) O: %7luH: %7lu PO: %7lu PH: %7lu\n",
				&type->TypeName, type->TypeIndex,
				type->TotalNumberOfObjects, type->TotalNumberOfHandles,
				type->HighWaterNumberOfObjects, type->HighWaterNumberOfHandles);

			// move to next type object

			auto temp = (PBYTE)type + sizeof(OBJECT_TYPE_INFORMATION) + type->TypeName.MaximumLength;
			//
			// round up to the next pointer-size address
			//
			type = (OBJECT_TYPE_INFORMATION*)(((ULONG_PTR)temp + sizeof(PVOID) - 1) / sizeof(PVOID) * sizeof(PVOID));
		}
        printf("Total Types: %lu\n", types->NumberOfTypes);
	}
	return 0;
}

