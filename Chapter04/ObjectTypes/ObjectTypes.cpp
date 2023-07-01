// ObjectTypes.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

enum class PoolType {
	PagedPool = 1,
	NonPagedPool = 0,
	NonPagedPoolNx = 0x200,
	NonPagedPoolSessionNx = NonPagedPoolNx + 32,
	PagedPoolSessionNx = NonPagedPoolNx + 33
};

const char* PoolTypeToString(ULONG poolType) {
	switch ((PoolType)poolType) {
		case PoolType::PagedPool: return "Paged";
		case PoolType::NonPagedPool: return "Non Paged";
		case PoolType::NonPagedPoolNx: return "Non Paged NX";
		case PoolType::NonPagedPoolSessionNx: return "Session Non Paged NX";
		case PoolType::PagedPoolSessionNx: return "Session Paged NX";
	}
	return "";
}

void DisplayType(OBJECT_TYPE_INFORMATION* type) {
	printf("%2d %-34wZ H: %6u O: %6u PH: %6u PO: %6u Pool: %s\n",
		type->TypeIndex, &type->TypeName,
		type->TotalNumberOfHandles, type->TotalNumberOfObjects,
		type->HighWaterNumberOfHandles, type->HighWaterNumberOfObjects,
		PoolTypeToString(type->PoolType));
}

int main() {
	ULONG size = 1 << 14;
	auto buffer = std::make_unique<BYTE[]>(size);
	if (!NT_SUCCESS(NtQueryObject(nullptr, ObjectTypesInformation, buffer.get(), size, nullptr))) {
		return 1;
	}
	auto p = (OBJECT_TYPES_INFORMATION*)buffer.get();
	auto type = (OBJECT_TYPE_INFORMATION*)((PBYTE)p + sizeof(ULONG_PTR));
	long long handles = 0, objects = 0;
	for (ULONG i = 0; i < p->NumberOfTypes; i++) {
		DisplayType(type);
		handles += type->TotalNumberOfHandles;
		objects += type->TotalNumberOfObjects;

		auto offset = sizeof(OBJECT_TYPE_INFORMATION) + type->TypeName.MaximumLength;
		if (offset % sizeof(ULONG_PTR))
			offset += sizeof(ULONG_PTR) - ((ULONG_PTR)type + offset) % sizeof(ULONG_PTR);
		type = (OBJECT_TYPE_INFORMATION*)((PBYTE)type + offset);
	}
	printf("Types: %u Handles: %llu Objects: %llu\n",
		p->NumberOfTypes, handles, objects);
	return 0;
}

