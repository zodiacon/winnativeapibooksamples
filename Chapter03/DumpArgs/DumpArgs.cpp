#include <phnt_windows.h>
#include <phnt.h>

#ifdef RTL_CONSTANT_STRING
#undef RTL_CONSTANT_STRING
#endif
#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof((s)[0]), sizeof(s), (PWSTR)s }

NTSTATUS NtProcessStartup(PPEB peb) {
	auto& pp = peb->ProcessParameters;

	HANDLE hFile;
	IO_STATUS_BLOCK ioStatus;
	UNICODE_STRING filename = RTL_CONSTANT_STRING(L"\\??\\c:\\Temp\\args.txt");
	OBJECT_ATTRIBUTES fileAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(&filename, OBJ_CASE_INSENSITIVE);
	auto status = NtCreateFile(&hFile, GENERIC_WRITE | SYNCHRONIZE, &fileAttr, &ioStatus, nullptr, 0, 0, FILE_SUPERSEDE,
		FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0);
	if (NT_SUCCESS(status)) {
		NtWriteFile(hFile, nullptr, nullptr, nullptr, &ioStatus, pp->ImagePathName.Buffer, pp->ImagePathName.Length, nullptr, nullptr);
		NtWriteFile(hFile, nullptr, nullptr, nullptr, &ioStatus, (PVOID)L" ", 2, nullptr, nullptr);
		NtWriteFile(hFile, nullptr, nullptr, nullptr, &ioStatus, pp->CommandLine.Buffer, pp->CommandLine.Length, nullptr, nullptr);
		NtClose(hFile);
	}

	return 0;
}
