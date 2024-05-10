// SimpleServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <cassert>

#pragma comment(lib, "ntdll")

struct Message : PORT_MESSAGE {
	char Text[100];
};

int main() {
	HANDLE hServerPort;
	UNICODE_STRING portName;
	RtlInitUnicodeString(&portName, L"\\RPC Control\\SimpleServerPort");
	OBJECT_ATTRIBUTES portAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(&portName, 0);
	auto status = NtAlpcCreatePort(&hServerPort, &portAttr, nullptr);
	if (!NT_SUCCESS(status)) {
		printf("NtAlpcCreatePort failed: 0x%X\n", status);
		return 1;
	}
	printf("Server port created: 0x%p\n", hServerPort);

	Message msg;
	SIZE_T size = sizeof(msg);
	PPORT_MESSAGE sendMessage = nullptr;
	PPORT_MESSAGE receiveMessage = &msg;

	for (;;) {
		status = NtAlpcSendWaitReceivePort(hServerPort, ALPC_MSGFLG_RELEASE_MESSAGE, 
			sendMessage, nullptr, receiveMessage, &size, nullptr, nullptr);
		if (!NT_SUCCESS(status))
			break;

		printf("Received msg type: 0x%X (ID: 0x%X)\n", 
			receiveMessage->u2.s2.Type, receiveMessage->MessageId);
		switch (receiveMessage->u2.s2.Type & 0xff) {
			case LPC_CONNECTION_REQUEST:
				printf("Connection request received from PID: %u TID: %u\n", 
					HandleToULong(receiveMessage->ClientId.UniqueProcess), 
					HandleToULong(receiveMessage->ClientId.UniqueThread));
				HANDLE hCommPort;
				status = NtAlpcAcceptConnectPort(&hCommPort, hServerPort, 0, nullptr, nullptr, nullptr, 
					receiveMessage, nullptr, TRUE);
				if (!NT_SUCCESS(status)) {
					printf("NtAlpcAcceptConnectPort failed: 0x%X\n", status);
				}
				else {
					printf("Client port connected: 0x%p\n", hCommPort);
				}
				sendMessage = nullptr;
				break;

			case LPC_REQUEST:
				printf("\t%s\n", msg.Text);
				sendMessage = receiveMessage;
				sendMessage->u1.s1.DataLength = 0;
				sendMessage->u1.s1.TotalLength = sizeof(PORT_MESSAGE);
				break;

			default:
				printf("Other message type: 0x%X\n", receiveMessage->u2.s2.Type);
				break;
		}
	}

	NtClose(hServerPort);

	return 0;
}
