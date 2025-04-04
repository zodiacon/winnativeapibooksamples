// AllObjects.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

int main() {
	ULONG size = 1 << 22;
	auto buffer = std::make_unique<BYTE[]>(size);
	NTSTATUS status;
	while ((status = NtQuerySystemInformation(SystemObjectInformation, buffer.get(), size, nullptr)) == STATUS_INFO_LENGTH_MISMATCH) {
		size *= 2;
		buffer = std::make_unique<BYTE[]>(size);
		if (!buffer) {
			printf("Out of memory!\n");
			return -1;
		}
	}
	if (!NT_SUCCESS(status)) {
		printf("Object tracking is not turned on.\n");
		return status;
	}

	auto type = (SYSTEM_OBJECTTYPE_INFORMATION*)buffer.get();
	for(;;) {
		printf("%wZ O: %lu H: %lu\n",
			&type->TypeName, type->NumberOfObjects, type->NumberOfHandles);

		auto obj = (SYSTEM_OBJECT_INFORMATION*)((PBYTE)type + sizeof(*type) + type->TypeName.MaximumLength);
		for (;;) {
			printf("0x%p (%wZ) P: %ld H: %ld PID: %ld EPID: %ld\n",
				obj->Object, &obj->NameInfo, obj->PointerCount, obj->HandleCount,
				HandleToULong(obj->CreatorUniqueProcess), HandleToULong(obj->ExclusiveProcessId));
			if (obj->NextEntryOffset == 0)
				break;
			obj = (SYSTEM_OBJECT_INFORMATION*)(buffer.get() + obj->NextEntryOffset);
		}
		if (type->NextEntryOffset == 0)
			break;
		type = (SYSTEM_OBJECTTYPE_INFORMATION*)(buffer.get() + type->NextEntryOffset);
	}

	return 0;
}

