// sd.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <string>

#pragma comment(lib, "ntdll")

using namespace std;

wstring SidToString(const PSID sid) {
	UNICODE_STRING ssid{};
	wstring result;
	if (NT_SUCCESS(RtlConvertSidToUnicodeString(&ssid, sid, TRUE))) {
		result = wstring(ssid.Buffer, ssid.Length / sizeof(WCHAR));
		RtlFreeUnicodeString(&ssid);
	}
	return result;
}

wstring GetUserNameFromSid(const PSID sid, SID_NAME_USE* puse = nullptr) {
	WCHAR name[64], domain[64];
	DWORD lname = _countof(name), ldomain = _countof(domain);
	SID_NAME_USE use;
	if (LookupAccountSid(nullptr, sid, name, &lname, domain, &ldomain, &use)) {
		if (puse)
			*puse = use;
		return wstring(domain) + L"\\" + name;
	}
	return L"";
}

string SDControlToString(SECURITY_DESCRIPTOR_CONTROL control) {
	string result;
	static const struct {
		DWORD flag;
		PCSTR text;
	} attributes[] = {
		{ SE_OWNER_DEFAULTED, "Owner Defaulted" },
		{ SE_GROUP_DEFAULTED, "Group Defaulted" },
		{ SE_DACL_PRESENT, "DACL Present" },
		{ SE_DACL_DEFAULTED, "DACL Defaulted" },
		{ SE_SACL_PRESENT, "SACL Present" },
		{ SE_SACL_DEFAULTED, "SACL Defaulted" },
		{ SE_DACL_AUTO_INHERIT_REQ, "DACL Auto Inherit Required" },
		{ SE_SACL_AUTO_INHERIT_REQ, "SACL Auto Inherit Required" },
		{ SE_DACL_AUTO_INHERITED, "DACL Auto Inherited" },
		{ SE_SACL_AUTO_INHERITED, "SACL Auto Inherited" },
		{ SE_DACL_PROTECTED, "DACL Protected" },
		{ SE_SACL_PROTECTED, "SACL Protected" },
		{ SE_RM_CONTROL_VALID, "RM Control Valid" },
		{ SE_SELF_RELATIVE, "Self Relative" },
	};

	for (const auto& attr : attributes)
		if ((attr.flag & control) == attr.flag)
			(result += attr.text) += ", ";

	if (!result.empty())
		result = result.substr(0, result.size() - 2);
	else
		result = "(none)";
	return result;
}

const char* AceTypeToString(BYTE type) {
	switch (type) {
		case ACCESS_ALLOWED_ACE_TYPE: return "ALLOW";
		case ACCESS_DENIED_ACE_TYPE: return "DENY";
		case ACCESS_ALLOWED_CALLBACK_ACE_TYPE: return "ALLOW CALLBACK";
		case ACCESS_DENIED_CALLBACK_ACE_TYPE: return "DENY CALLBACK";
		case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE: return "ALLOW CALLBACK OBJECT";
		case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE: return "DENY CALLBACK OBJECT";
	}
	return "<unknown>";
}

void DisplayAce(PACE_HEADER header, int index) {
	printf("ACE %2d: Size: %2d bytes, Flags: 0x%02X Type: %s\n",
		index, header->AceSize, header->AceFlags, AceTypeToString(header->AceType));
	switch (header->AceType) {
		case ACCESS_ALLOWED_ACE_TYPE:
		case ACCESS_DENIED_ACE_TYPE:	// have the same binary layout
		{
			auto data = (ACCESS_ALLOWED_ACE*)header;
			printf("\tAccess: 0x%08X %ws (%ws)\n", data->Mask,				
				SidToString((PSID)&data->SidStart).c_str(),
				GetUserNameFromSid((PSID)&data->SidStart).c_str());
		}
		break;
	}
}

void DisplaySD(const PSECURITY_DESCRIPTOR sd) {
	auto len = RtlLengthSecurityDescriptor(sd);
	printf("SD Length: %u (0x%X) bytes\n", len, len);

	SECURITY_DESCRIPTOR_CONTROL control;
	DWORD revision;
	if (NT_SUCCESS(RtlGetControlSecurityDescriptor(sd, &control, &revision))) {
		printf("Revision: %u Control: 0x%X (%s)\n", revision,
			control, SDControlToString(control).c_str());
	}
	PSID sid;
	BOOLEAN defaulted;
	if (NT_SUCCESS(RtlGetOwnerSecurityDescriptor(sd, &sid, &defaulted))) {
		if (sid)
			printf("Owner: %ws (%ws) Defaulted: %s\n", 
				SidToString(sid).c_str(), GetUserNameFromSid(sid).c_str(),
				defaulted ? "Yes" : "No");
		else
			printf("No owner\n");
	}

	BOOLEAN present;
	PACL dacl;
	if (NT_SUCCESS(RtlGetDaclSecurityDescriptor(sd, &present, &dacl, &defaulted))) {
		if (!present)
			printf("NULL DACL - object is unprotected\n");
		else {
			printf("DACL: ACE count: %d\n", (int)dacl->AceCount);
			PACE_HEADER header;
			for (int i = 0; i < dacl->AceCount; i++) {
				if (NT_SUCCESS(RtlGetAce(dacl, i, (PVOID*)&header))) {
					DisplayAce(header, i);
				}
			}
		}
	}
}

HANDLE OpenNamedObject(PCWSTR path) {
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, path);
	OBJECT_ATTRIBUTES objAttr;
	InitializeObjectAttributes(&objAttr, &name, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
	HANDLE hObject = nullptr;

	//
	// try all "classic" objects with some kind of name
	//

	// event
	NtOpenEvent(&hObject, READ_CONTROL, &objAttr);
	if (hObject)
		return hObject;

	// mutex
	NtOpenMutant(&hObject, READ_CONTROL, &objAttr);
	if (hObject)
		return hObject;

	// job
	NtOpenJobObject(&hObject, READ_CONTROL, &objAttr);
	if (hObject)
		return hObject;

	// section
	NtOpenSection(&hObject, READ_CONTROL, &objAttr);
	if (hObject)
		return hObject;

	// registry key
	NtOpenKey(&hObject, READ_CONTROL, &objAttr);
	if (hObject)
		return hObject;

	// semaphore
	NtOpenSemaphore(&hObject, READ_CONTROL, &objAttr);
	if (hObject)
		return hObject;

	// timer
	NtOpenTimer(&hObject, READ_CONTROL, &objAttr);
	if (hObject)
		return hObject;

	// object manager directory
	NtOpenDirectoryObject(&hObject, READ_CONTROL, &objAttr);
	if (hObject)
		return hObject;

	// file/directory (file system)
	IO_STATUS_BLOCK ioStatus;
	NtOpenFile(&hObject, FILE_GENERIC_READ, &objAttr, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE, 0);
	if (hObject)
		return hObject;

	//
	// special handling for a drive letter
	//
	if (name.Length > 4 && path[1] == L':') {
		UNICODE_STRING name2;
		name2.MaximumLength = name.Length + sizeof(WCHAR) * 4;
		name2.Buffer = (PWSTR)RtlAllocateHeap(RtlProcessHeap(), 0, name2.MaximumLength);
		UNICODE_STRING prefix;
		RtlInitUnicodeString(&prefix, L"\\??\\");
		RtlCopyUnicodeString(&name2, &prefix);
		RtlAppendUnicodeStringToString(&name2, &name);
		InitializeObjectAttributes(&objAttr, &name2, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
		NtOpenFile(&hObject, FILE_GENERIC_READ, &objAttr, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE, 0);
		RtlFreeHeap(RtlProcessHeap(), 0, name2.Buffer);
	}

	return hObject;
}

int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2) {
		printf("Usage: sd [[-p pid [handle] | [-t tid] | [object_name]]\n");
		printf("If no arguments are specified, shows the current process security descriptor\n");
	}

	BOOLEAN enabled;
	RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &enabled);


	OBJECT_ATTRIBUTES emptyAttr;
	InitializeObjectAttributes(&emptyAttr, nullptr, 0, nullptr, nullptr);

	bool thisProcess = argc == 1;
	HANDLE hObject = argc == 1 ? NtCurrentProcess() : nullptr;

	if (argc > 2) {
		if (_wcsicmp(argv[1], L"-p") == 0) {
			CLIENT_ID cid{ ULongToHandle(wcstol(argv[2], nullptr, 0)) };
			HANDLE hProcess = nullptr;
			NtOpenProcess(&hProcess, argc > 3 ? PROCESS_DUP_HANDLE : READ_CONTROL, &emptyAttr, &cid);
			if (hProcess && argc > 3) {
				NtDuplicateObject(hProcess, ULongToHandle(wcstoul(argv[3], nullptr, 0)), 
					NtCurrentProcess(), &hObject, READ_CONTROL, 0, 0);
				NtClose(hProcess);
			}
			else {
				hObject = hProcess;
			}
		}
		else if (_wcsicmp(argv[1], L"-t") == 0) {
			CLIENT_ID cid{ nullptr, ULongToHandle(wcstol(argv[2], nullptr, 0)) };
			NtOpenThread(&hObject, READ_CONTROL, &emptyAttr, &cid);
		}
	}
	else if (argc == 2) {
		hObject = OpenNamedObject(argv[1]);
	}

	if (!hObject) {
		printf("Error opening object\n");
		return 1;
	}

	PSECURITY_DESCRIPTOR sd = nullptr;
	BYTE buffer[1 << 12];
	ULONG needed;
	auto status = NtQuerySecurityObject(hObject, 
		OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, 
		buffer, sizeof(buffer), &needed);
	if (!NT_SUCCESS(status)) {
		printf("No security descriptor available (0x%X)\n", status);
	}
	else {
		DisplaySD((PSECURITY_DESCRIPTOR)buffer);
	}
	
	if (hObject && !thisProcess)
		NtClose(hObject);

	return 0;
}