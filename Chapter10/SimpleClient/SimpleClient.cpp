// SimpleClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#pragma comment(lib, "ntdll")

struct Message : PORT_MESSAGE {
	char Text[100];
};

void Delay(int seconds) {
	LARGE_INTEGER time;
	time.QuadPart = -10000000LL * seconds;
	NtDelayExecution(FALSE, &time);
}

int main() {
	HANDLE hPort;
	UNICODE_STRING portName;
	RtlInitUnicodeString(&portName, L"\\RPC Control\\SimpleServerPort");
	NTSTATUS status;
	for (int i = 0; i < 10; i++) {
		status = NtAlpcConnectPort(&hPort, &portName, nullptr, nullptr, 
			ALPC_MSGFLG_SYNC_REQUEST, nullptr, nullptr, nullptr, 
			nullptr, nullptr, nullptr);
		if (NT_SUCCESS(status))
			break;
		printf("NtAlpcConnectPort failed: 0x%X\n", status);
		Delay(1);
	}
	if(!NT_SUCCESS(status))
		return 1;

	printf("Client port connected: 0x%p\n", hPort);
	LARGE_INTEGER time;
	TIME_FIELDS tf;

	for (;;) {
		Message msg{};
		NtQuerySystemTime(&time);
		RtlSystemTimeToLocalTime(&time, &time);
		RtlTimeToTimeFields(&time, &tf);
		sprintf_s(msg.Text, "The Time is %02d:%02d:%02d.%03d",
			tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
		msg.u1.s1.DataLength = sizeof(msg.Text);
		msg.u1.s1.TotalLength = msg.u1.s1.DataLength + sizeof(PORT_MESSAGE);

		Message reply;
		SIZE_T msgLen = sizeof(reply);
		status = NtAlpcSendWaitReceivePort(hPort, ALPC_MSGFLG_SYNC_REQUEST, &msg, 
			nullptr, &reply, &msgLen, nullptr, nullptr);
		if (!NT_SUCCESS(status)) {
			printf("NtAlpcSendWaitReceivePort failed: 0x%X\n", status);
			break;
		}
		printf("Sent message %s.\n", msg.Text);
		printf("Received reply from PID: %u TID: %u\n", 
			HandleToULong(reply.ClientId.UniqueProcess), 
			HandleToULong(reply.ClientId.UniqueThread));
		Delay(1);
	}
	NtClose(hPort);

	return 0;
}

