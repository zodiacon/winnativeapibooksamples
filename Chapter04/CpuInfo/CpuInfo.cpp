// CpuInfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

std::string TimeSpanToString(LARGE_INTEGER ts) {
    TIME_FIELDS tf;
    RtlTimeToElapsedTimeFields(&ts, &tf);
    return std::format("{:02d}.{:02d}:{:02d}:{:02d}.{:03d}",
        tf.Day, tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

std::string TimeSpanToString(LONGLONG const& ts) {
    LARGE_INTEGER li;
    li.QuadPart = ts;
    return TimeSpanToString(li);
}

int main() {
    ULONG len;
    USHORT group = 0;   // group 0
    NtQuerySystemInformationEx(SystemProcessorPerformanceInformation,
        &group, sizeof(group), nullptr, 0, &len);
    ULONG cpuCount = len / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
    auto pi = std::make_unique<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[]>(cpuCount);

    auto upcursor = "\033[" + std::to_string(cpuCount) + "A";
    LARGE_INTEGER sleep{ .QuadPart = -1000 * 10000 };
    for (;;) {
        auto status = NtQuerySystemInformationEx(SystemProcessorPerformanceInformation,
            &group, sizeof(group), pi.get(), len, nullptr);
        if (!NT_SUCCESS(status))
            break;
        for (ULONG i = 0; i < cpuCount; i++) {
            printf("CPU %2u Idle: %s Kernel: %s User: %s Total: %s DPC: %s Int: %s IntCnt: %u     \n",
                i, TimeSpanToString(pi[i].IdleTime).c_str(),
                TimeSpanToString(pi[i].KernelTime.QuadPart - pi[i].IdleTime.QuadPart).c_str(),
                TimeSpanToString(pi[i].UserTime).c_str(),
                TimeSpanToString(pi[i].KernelTime.QuadPart - pi[i].IdleTime.QuadPart + pi[i].UserTime.QuadPart).c_str(),
                TimeSpanToString(pi[i].DpcTime).c_str(),
                TimeSpanToString(pi[i].InterruptTime).c_str(),
                pi[i].InterruptCount);
        }

        NtDelayExecution(FALSE, &sleep);
        printf(upcursor.c_str());
        //printf("\033[2J");
    }
    return 0;
}
