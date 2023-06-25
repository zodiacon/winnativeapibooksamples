#include <phnt_windows.h>
#include <phnt.h>

extern "C" int swprintf_s(
	wchar_t* _Buffer, USHORT size,
	wchar_t const* _Format, ...);

NTSTATUS NtProcessStartup(PPEB peb) {
	RTL_OSVERSIONINFOEXW osvi = { sizeof(osvi) };
	RtlGetVersion(&osvi);
	WCHAR text[256];
	swprintf_s(text, ARRAYSIZE(text), L"Windows version: %d.%d.%d\n",
		osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

	UNICODE_STRING str;
	RtlInitUnicodeString(&str, text);
	NtDrawText(&str);

	LARGE_INTEGER li;
	li.QuadPart = -10000000 * 10;
	NtDelayExecution(FALSE, &li);

	return STATUS_SUCCESS;
}

