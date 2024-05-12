// CreateToken.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <phnt_windows.h>
#define PHNT_VERSION PHNT_THRESHOLD
#include <phnt.h>
#include <assert.h>
#include <stdio.h>
#include <WtsApi32.h>

#pragma comment(lib, "ntdll")
#pragma comment(lib, "wtsapi32")

template<int N>
struct MultiGroups : TOKEN_GROUPS {
	MultiGroups() {
		GroupCount = N;
	}
	SID_AND_ATTRIBUTES _Additional[N - 1]{};
};

template<int N>
struct MultiPrivileges : TOKEN_PRIVILEGES {
	MultiPrivileges() {
		PrivilegeCount = N;
	}
	LUID_AND_ATTRIBUTES _Additional[N - 1]{};
};

void DisplaySid(PSID sid) {
	UNICODE_STRING name{};
	if (NT_SUCCESS(RtlConvertSidToUnicodeString(&name, sid, TRUE))) {
		printf("SID: %wZ\n", &name);
		RtlFreeUnicodeString(&name);
	}
}

HANDLE FindLsass() {
	WCHAR path[MAX_PATH];
	GetSystemDirectory(path, ARRAYSIZE(path));
	wcscat_s(path, L"\\lsass.exe");
	UNICODE_STRING lsassPath;
	RtlInitUnicodeString(&lsassPath, path);

	BYTE buffer[256];
	HANDLE hProcess = nullptr, hOld;
	while (true) {
		hOld = hProcess;
		auto status = NtGetNextProcess(hProcess, PROCESS_QUERY_LIMITED_INFORMATION, 0, 0, &hProcess);
		if (hOld)
			NtClose(hOld);
		if (!NT_SUCCESS(status))
			break;

		if (NT_SUCCESS(NtQueryInformationProcess(hProcess, ProcessImageFileNameWin32, buffer, sizeof(buffer), nullptr))) {
			auto name = (UNICODE_STRING*)buffer;
			if (RtlEqualUnicodeString(&lsassPath, name, TRUE))
				return hProcess;
		}
	}
	return nullptr;
}

HANDLE DuplicateLsassToken() {
	auto hProcess = FindLsass();
	if (hProcess == nullptr)
		return nullptr;

	HANDLE hToken = nullptr;
	NtOpenProcessToken(hProcess, TOKEN_DUPLICATE, &hToken);
	if (!hToken)
		return nullptr;

	HANDLE hNewToken = nullptr;
	OBJECT_ATTRIBUTES tokenAttr;
	InitializeObjectAttributes(&tokenAttr, nullptr, 0, nullptr, nullptr);
	SECURITY_QUALITY_OF_SERVICE qos{ sizeof(qos) };
	qos.ImpersonationLevel = SecurityImpersonation;
	tokenAttr.SecurityQualityOfService = &qos;
	NtDuplicateToken(hToken, TOKEN_ALL_ACCESS, &tokenAttr, FALSE, TokenImpersonation, &hNewToken);
	NtClose(hToken);
	return hNewToken;
}

int main() {
	BOOLEAN enabled;
	auto status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &enabled);
	if (!NT_SUCCESS(status)) {
		printf("Failed to enable the debug privilege! (0x%X)\n", status);
		return status;
	}

	auto hDupToken = DuplicateLsassToken();
	if (!hDupToken) {
		printf("Failed to duplicate Lsass token\n");
		return 1;
	}

	SE_SID systemSid;
	DWORD size = sizeof(systemSid);
	CreateWellKnownSid(WinLocalSystemSid, nullptr, &systemSid, &size);
	DisplaySid(&systemSid);

	SE_SID adminSid;
	size = sizeof(adminSid);
	CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, &adminSid, &size);
	DisplaySid(&adminSid);

	SE_SID allUsersSid;
	size = sizeof(allUsersSid);
	CreateWellKnownSid(WinWorldSid, nullptr, &allUsersSid, &size);
	DisplaySid(&allUsersSid);

	SE_SID interactiveSid;
	size = sizeof(interactiveSid);
	CreateWellKnownSid(WinInteractiveSid, nullptr, &interactiveSid, &size);
	DisplaySid(&interactiveSid);

	SE_SID authUsers;
	size = sizeof(authUsers);
	CreateWellKnownSid(WinAuthenticatedUserSid, nullptr, &authUsers, &size);
	DisplaySid(&authUsers);

	PSID integritySid;
	SID_IDENTIFIER_AUTHORITY auth = SECURITY_MANDATORY_LABEL_AUTHORITY;
	status = RtlAllocateAndInitializeSid(&auth, 1, SECURITY_MANDATORY_MEDIUM_RID, 0, 0, 0, 0, 0, 0, 0, &integritySid);
	assert(SUCCEEDED(status));

	//
	// set up groups
	//
	MultiGroups<6> groups;
	groups.Groups[0].Sid = &adminSid;
	groups.Groups[0].Attributes = SE_GROUP_DEFAULTED | SE_GROUP_ENABLED | SE_GROUP_OWNER;
	groups.Groups[1].Sid = &allUsersSid;
	groups.Groups[1].Attributes = SE_GROUP_ENABLED | SE_GROUP_DEFAULTED;
	groups.Groups[2].Sid = &interactiveSid;
	groups.Groups[2].Attributes = SE_GROUP_ENABLED | SE_GROUP_DEFAULTED;
	groups.Groups[3].Sid = &systemSid;
	groups.Groups[3].Attributes = SE_GROUP_ENABLED | SE_GROUP_DEFAULTED;
	groups.Groups[4].Sid = integritySid;
	groups.Groups[4].Attributes = SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED;
	groups.Groups[5].Sid = &authUsers;
	groups.Groups[5].Attributes = SE_GROUP_ENABLED | SE_GROUP_DEFAULTED;

	//
	// set up privileges
	//
	MultiPrivileges<2> privs;
	privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT;
	privs.Privileges[0].Luid.LowPart = SE_CHANGE_NOTIFY_PRIVILEGE;
	privs.Privileges[1].Luid.LowPart = SE_TCB_PRIVILEGE;

	TOKEN_PRIMARY_GROUP primary;
	primary.PrimaryGroup = &adminSid;

	TOKEN_USER user{};
	user.User.Sid = &systemSid;

	//
	// impersonate
	//
	status = NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken, &hDupToken, sizeof(hDupToken));
	if (!NT_SUCCESS(status)) {
		printf("Failed to impersonate! (0x%X)\n", status);
		return status;
	}

	LUID authenticationId = RtlConvertUlongToLuid(999);
	TOKEN_SOURCE source{ "Ch11", 777 };
	LARGE_INTEGER expire{};
	HANDLE hToken;
	status = NtCreateToken(&hToken, TOKEN_ALL_ACCESS, nullptr, TokenPrimary,
		&authenticationId, &expire, &user, &groups, &privs, nullptr, &primary, nullptr, &source);

	if (NT_SUCCESS(status)) {
		printf("Token created successfully.\n");
		if (NT_SUCCESS(RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, TRUE, &enabled))) {
			ULONG session = WTSGetActiveConsoleSessionId();
			NtQueryInformationProcess(NtCurrentProcess(), ProcessSessionInformation, &session, sizeof(session), nullptr);
			NtSetInformationToken(hToken, TokenSessionId, &session, sizeof(session));
		}
		STARTUPINFO si{ sizeof(si) };
		PROCESS_INFORMATION pi;
		WCHAR desktop[] = L"winsta0\\Default";
		WCHAR name[] = L"notepad.exe";
		si.lpDesktop = desktop;

		status = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE, TRUE, &enabled);
		BOOL created = FALSE;
		if (NT_SUCCESS(status)) {
			//
			// option 1
			//
			created = CreateProcessAsUser(hToken, nullptr, name, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
		}

		if (!created) {
			//
			// option 2
			//
			created = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, nullptr, name, 0, nullptr, nullptr, &si, &pi);
		}

		if (!created) {
			printf("Failed to create process (%u)\n", GetLastError());
		}
		else {
			printf("Process created: %u\n", pi.dwProcessId);
			NtClose(pi.hProcess);
			NtClose(pi.hThread);
		}
		RtlFreeSid(integritySid);
	}
	else {
		printf("Failed to create token (0x%X)\n", status);
	}
	return status;
}

