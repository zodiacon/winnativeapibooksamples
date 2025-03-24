// ThreadPrimes.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <memory>

#pragma comment(lib, "ntdll")

struct PrimeData {
	int From, To;
	int Count;
};

int CalculatePrimes(int from, int to, int threads);
NTSTATUS NTAPI CalcPrimesThread(PVOID p);
int CalculatePrimesModern(int from, int to);

int main(int argc, const char* argv[]) {
	int threads = 1;
	if (argc > 1)
		threads = atoi(argv[1]);
	int first = 3, last = 35000000;

	printf("Using %d threads...\n", threads);

	auto start = GetTickCount64();
	auto count = CalculatePrimes(first, last, threads);
	auto stop = GetTickCount64();
	printf("Count=%u, Elapsed=%u msec\n", count, (DWORD)(stop - start));

	return 0;
}


int CalculatePrimes(int from, int to, int threads) {
	int range = (to - from + 1) / threads;
	auto data = std::make_unique<PrimeData[]>(threads);
	auto hThread = std::make_unique<HANDLE[]>(threads);

	for (int i = 0; i < threads; i++) {
		data[i].From = i * range + from;
		data[i].To = i == threads - 1 ? to : (i + 1) * range + from - 1;
		auto status = RtlCreateUserThread(NtCurrentProcess(), nullptr, FALSE, 0, 0, 0, 
			CalcPrimesThread, &data[i], &hThread[i], nullptr);
		if (!NT_SUCCESS(status)) {
			printf("Error creaing thread (0x%X)\n", status);
			RtlExitUserProcess(status);
		}
	}

	NtWaitForMultipleObjects(threads, hThread.get(), WaitAll, FALSE, nullptr);

	int count = 0;
	for (int i = 0; i < threads; i++) {
		count += data[i].Count;
		NtClose(hThread[i]);
	}

	return count;
}

bool IsPrime(int i) {
	int limit = (int)sqrt(i);
	for (int j = 2; j <= limit; j++)
		if (i % j == 0)
			return false;
	return true;
}

NTSTATUS NTAPI CalcPrimesThread(PVOID p) {
	PrimeData* data = (PrimeData*)p;
	int* pPrimes = nullptr;
	int from = data->From, to = data->To;
	int count = 0;
	for (int i = from; i <= to; i++) {
		if (IsPrime(i)) {
			count++;
			pPrimes = (int*)realloc(pPrimes, count * sizeof(int));
			*(pPrimes + count - 1) = i;
		}
	}

	data->Count = count;

	return STATUS_SUCCESS;
}

