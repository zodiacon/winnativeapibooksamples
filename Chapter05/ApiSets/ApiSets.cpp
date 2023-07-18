// ApiSets.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#define API_SET_SCHEMA_ENTRY_FLAGS_SEALED 1

int main() {
	auto apiSetMap = NtCurrentPeb()->ApiSetMap;
	auto apiSetMapAsNumber = ULONG_PTR(apiSetMap);

	auto nsEntry = PAPI_SET_NAMESPACE_ENTRY(apiSetMap->EntryOffset + apiSetMapAsNumber);

	for (ULONG i = 0; i < apiSetMap->Count; i++) {
		auto isSealed = (nsEntry->Flags & API_SET_SCHEMA_ENTRY_FLAGS_SEALED) != 0;

		std::wstring name(PWCHAR(apiSetMapAsNumber + nsEntry->NameOffset), nsEntry->NameLength / sizeof(WCHAR));
		printf("%56ws.dll -> %s{", name.c_str(), (isSealed ? "s" : ""));

		auto valueEntry = PAPI_SET_VALUE_ENTRY(apiSetMapAsNumber + nsEntry->ValueOffset);
		for (ULONG j = 0; j < nsEntry->ValueCount; j++) {
			//
			// host name
			//
			name.assign(PWCHAR(apiSetMapAsNumber + valueEntry->ValueOffset), valueEntry->ValueLength / sizeof(WCHAR));
			printf("%ws", name.c_str());

			//
			// If there's more than one, add a comma
			//
			if ((j + 1) != nsEntry->ValueCount) {
				printf(", ");
			}

			//
			// If there's an alias
			//
			if (valueEntry->NameLength != 0) {
				name.assign(PWCHAR(apiSetMapAsNumber + valueEntry->NameOffset), valueEntry->NameLength / sizeof(WCHAR));
				printf(" [%ws]", name.c_str());
			}

			valueEntry++;
		}
		printf("}\n");
		nsEntry++;
	}
	return 0;
}

