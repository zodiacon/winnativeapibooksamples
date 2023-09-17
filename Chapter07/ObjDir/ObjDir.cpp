#include "pch.h"

#pragma comment(lib, "ntdll")

struct ObjectInfo {
	std::wstring Name;
	std::wstring TypeName;
	std::wstring LinkTarget;
};

std::wstring GetSymbolicLinkTarget(std::wstring const& directory, std::wstring const& name) {
	auto fullName = directory == L"\\" ? (L"\\" + name) : (directory + L"\\" + name);
	UNICODE_STRING linkName;
	RtlInitUnicodeString(&linkName, fullName.c_str());
	OBJECT_ATTRIBUTES attr;
	InitializeObjectAttributes(&attr, &linkName, 0, nullptr, nullptr);
	HANDLE hLink;
	auto status = NtOpenSymbolicLinkObject(&hLink, GENERIC_READ, &attr);
	if (!NT_SUCCESS(status))
		return L"";

	WCHAR buffer[512]{};
	UNICODE_STRING target;
	RtlInitUnicodeString(&target, buffer);
	target.MaximumLength = sizeof(buffer);
	status = NtQuerySymbolicLinkObject(hLink, &target, nullptr);
	NtClose(hLink);
	return buffer;
}

std::vector<ObjectInfo> EnumDirectoryObjects(PCWSTR directory, NTSTATUS& status) {
	HANDLE hDirectory;
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, directory);
	OBJECT_ATTRIBUTES attr;
	InitializeObjectAttributes(&attr, &name, 0, nullptr, nullptr);
	status = NtOpenDirectoryObject(&hDirectory, DIRECTORY_QUERY, &attr);
	if (!NT_SUCCESS(status))
		return {};

	ULONG index = 0;
	bool first = true;

	ULONG size = 1 << 12;
	auto buffer = std::make_unique<BYTE[]>(size);
	auto data = (POBJECT_DIRECTORY_INFORMATION)buffer.get();
	int start = 0;
	std::vector<ObjectInfo> objects;
	for(;;) {
		status = NtQueryDirectoryObject(hDirectory, buffer.get(), size, FALSE, first, &index, nullptr);
		if (!NT_SUCCESS(status))
			break;
		first = false;
		
		for (ULONG i = 0; i < index - start; i++) {
			ObjectInfo info;
			info.Name.assign(data[i].Name.Buffer, data[i].Name.Length / sizeof(WCHAR));
			info.TypeName.assign(data[i].TypeName.Buffer, data[i].TypeName.Length / sizeof(WCHAR));
			if (info.TypeName == L"SymbolicLink")
				info.LinkTarget = GetSymbolicLinkTarget(directory, info.Name);
			objects.push_back(std::move(info));
		}
		start = index;
		if (status != STATUS_MORE_ENTRIES)
			break;
	}
	NtClose(hDirectory);
	return objects;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: ObjDir <directory>\n");
		return 0;
	}

	NTSTATUS status;
	auto objects = EnumDirectoryObjects(argv[1], status);
	if (objects.empty() && !NT_SUCCESS(status)) {
		printf("Error: 0x%X\n", status);
	}
	else {
		printf("Directory: %ws\n", argv[1]);
		printf("%-20s %-50s Link Target\n", "Type", "Name");
		printf("%-20s %-50s -----------\n", "----", "----");
		for (auto& obj : objects) {
			printf("%-20ws %-50ws %ws\n",
				obj.TypeName.c_str(), obj.Name.c_str(),
				obj.LinkTarget.c_str());
		}
	}
	return 0;
}
