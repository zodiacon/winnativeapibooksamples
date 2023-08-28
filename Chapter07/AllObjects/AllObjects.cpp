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
	}
	if (!NT_SUCCESS(status)) {
		printf("Object tracking is not turned on.\n");
		return status;
	}

	auto type = (SYSTEM_OBJECTTYPE_INFORMATION*)buffer.get();
	for(;;) {
		printf("%wZ O: %u H: %u\n",
			&type->TypeName, type->NumberOfObjects, type->NumberOfHandles);

		auto obj = (SYSTEM_OBJECT_INFORMATION*)((PBYTE)type + sizeof(*type) + type->TypeName.MaximumLength);
		for (ULONG i = 0; i < type->NumberOfObjects; i++) {
			printf("0x%p (%wZ) P: 0x%X H: %u PID: %u EPID: %u\n",
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

