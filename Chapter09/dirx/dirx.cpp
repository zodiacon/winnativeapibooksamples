// dirx.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

std::wstring TimeToString(LARGE_INTEGER const& t) {
    SYSTEMTIME st;
    FileTimeToSystemTime((const FILETIME*)&t, &st);
    WCHAR buffer[64];
    GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, buffer, sizeof(buffer), nullptr);
    WCHAR buffer2[64];
    GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, buffer2, sizeof(buffer2));
    return std::wstring(buffer) + L" " + buffer2;
}

std::string AttributesToString(ULONG attr) {
    static struct {
        ULONG attr;
        PCSTR text;
    } attrs[] = {
        { FILE_ATTRIBUTE_ARCHIVE, "A" },
        { FILE_ATTRIBUTE_ENCRYPTED, "E" },
        { FILE_ATTRIBUTE_HIDDEN, "H" },
        { FILE_ATTRIBUTE_READONLY, "R" },
        { FILE_ATTRIBUTE_NORMAL, "" },
        { FILE_ATTRIBUTE_COMPRESSED, "C" },
        { FILE_ATTRIBUTE_DIRECTORY, "D" },
        { FILE_ATTRIBUTE_SYSTEM, "S" }
    };

    std::string sattr;
    for (auto& a : attrs)
        if ((a.attr & attr) == a.attr)
            sattr += a.text;
    return sattr;
}

NTSTATUS ListDirectory(PCWSTR path, PCWSTR filter = nullptr) {
    std::wstring spath(path);
    if (path[1] == L':')
        spath = L"\\??\\" + spath;

    UNICODE_STRING name;
    RtlInitUnicodeString(&name, spath.c_str());
    UNICODE_STRING filterName;
    if (filter)
        RtlInitUnicodeString(&filterName, filter);
    OBJECT_ATTRIBUTES attr = RTL_CONSTANT_OBJECT_ATTRIBUTES(&name, 0);
    HANDLE hDir;
    IO_STATUS_BLOCK ioStatus;
    auto status = NtOpenFile(&hDir, FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &attr, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status))
        return status;

    BYTE buffer[1 << 12];
    printf("%-30s %-13s %-15s %-15s %s\n", "Name", "Size", "Modified", "Created", "Attr");

    for (;;) {
        status = NtQueryDirectoryFile(hDir, nullptr, nullptr, nullptr,
            &ioStatus, buffer, sizeof(buffer), FileDirectoryInformation,
            FALSE, filter ? &filterName : nullptr, FALSE);
        if (!NT_SUCCESS(status))
            break;

        auto info = (FILE_DIRECTORY_INFORMATION*)buffer;
        while (true) {
            printf("%-30.*ws", (int)(info->FileNameLength / sizeof(WCHAR)), info->FileName);
            if (info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                printf(" %-13s", "<DIR>");
            else
                printf(" %9llu KB ", info->EndOfFile.QuadPart >> 10);
            printf("%-15ws %-15ws %s", 
                TimeToString(info->ChangeTime).c_str(), TimeToString(info->CreationTime).c_str(),
                AttributesToString(info->FileAttributes).c_str());
            printf("\n");
            if (info->NextEntryOffset == 0)
                break;
            info = (FILE_DIRECTORY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);
        }
    }

    NtClose(hDir);
    return status;
}


int wmain(int argc, const wchar_t* argv[]) {
    if (argc < 2) {
        printf("Usage: dirx <dir> [filter]\n");
        return 0;
    }

    ListDirectory(argv[1], argc > 2 ? argv[2] : nullptr);

	return 0;
}

