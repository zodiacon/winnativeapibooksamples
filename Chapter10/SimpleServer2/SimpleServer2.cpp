
#include <phnt_windows.h>
#include <phnt.h>
#include <cassert>
#include <stdio.h>

#pragma comment(lib, "ntdll")

#define ALPC_FLG_MSG_SEC_ATTR               0x80000000
#define ALPC_FLG_MSG_DATAVIEW_ATTR          0x40000000
#define ALPC_FLG_MSG_CONTEXT_ATTR           0x20000000
#define ALPC_FLG_MSG_HANDLE_ATTR            0x10000000
#define ALPC_FLG_MSG_TOKEN_ATTR             0x08000000
#define ALPC_FLG_MSG_DIRECT_ATTR            0x04000000
#define ALPC_FLG_MSG_WORK_ON_BEHALF_ATTR    0x02000000
#define ALPC_FLG_MSG_ALL_ATTR				0xff000000

#define LPC_CONNECTION_REPLY				11
#define LPC_CANCELED						12
#define LPC_UNREGISTER_PROCESS				13

typedef struct _ALPC_TOKEN_ATTR {
	LUID TokenId;
	LUID AuthenticationId;
	LUID ModifiedId;
} ALPC_TOKEN_ATTR, * PALPC_TOKEN_ATTR;

typedef enum _ALPC_PORT_FLAGS {
	ALPC_PORT_FLAG_ALLOW_IMPERSONATION = 0x00010000,
	ALPC_PORT_FLAG_ACCEPT_REQUESTS = 0x00020000,
	ALPC_PORT_FLAG_WAITABLE_PORT = 0x00040000,
	ALPC_PORT_FLAG_ACCEPT_DUP_HANDLES = 0x00080000,
	ALPC_PORT_FLAG_SYSTEM_PROCESS = 0x00100000,
	ALPC_PORT_FLAG_SUPPRESS_WAKE = 0x00200000,
	ALPC_PORT_FLAG_ALWAYS_WAKE = 0x00400000,
	ALPC_PORT_FLAG_DO_NOT_DISTURB = 0x00800000,
	ALPC_PORT_FLAG_NO_SHARED_SECTION = 0x01000000,
	ALPC_PORT_FLAG_ACCEPT_INDIRECT_HANDLES = 0x02000000
} ALPC_PORT_FLAGS;

typedef enum _ALPC_OBJECT_TYPE {
	ALPC_FILE_OBJECT_TYPE = 0x00000001,
	ALPC_THREAD_OBJECT_TYPE = 0x00000004,
	ALPC_SEMAPHORE_OBJECT_TYPE = 0x00000008,
	ALPC_EVENT_OBJECT_TYPE = 0x00000010,
	ALPC_PROCESS_OBJECT_TYPE = 0x00000020,
	ALPC_MUTANT_OBJECT_TYPE = 0x00000040,
	ALPC_SECTION_OBJECT_TYPE = 0x00000080,
	ALPC_REG_KEY_OBJECT_TYPE = 0x00000100,
	ALPC_TOKEN_OBJECT_TYPE = 0x00000200,
	ALPC_COMPOSITION_OBJECT_TYPE = 0x00000400,
	ALPC_JOB_OBJECT_TYPE = 0x00000800,
	ALPC_ALL_OBJECT_TYPES = 0x00000FFD,
} ALPC_OBJECT_TYPE;

typedef struct _ALPC_HANDLE_ATTR32 {
	union {
		ULONG Flags;
		struct {
			ULONG Reserved0 : 16;
			ULONG SameAccess : 1;
			ULONG SameAttributes : 1;
			ULONG Indirect : 1;
			ULONG Inherit : 1;
			ULONG Reserved1 : 12;
		};
	};

	ULONG Handle;
	ULONG ObjectType;
	union {
		ULONG DesiredAccess;
		ULONG GrantedAccess;
	};
} ALPC_HANDLE_ATTR32, * PALPC_HANDLE_ATTR32;

// the structure
typedef struct _ALPC_HANDLE_ATTR {
	union {
		ULONG Flags;
		struct {
			ULONG Reserved0 : 16;
			ULONG SameAccess : 1;
			ULONG SameAttributes : 1;
			ULONG Indirect : 1;
			ULONG Inherit : 1;
			ULONG Reserved1 : 12;
		};
	};
	union {
		HANDLE Handle;
		ALPC_HANDLE_ATTR32* HandleAttrArray;
	};
	union {
		ULONG ObjectType;
		ULONG HandleCount;
	};
	union {
		ACCESS_MASK DesiredAccess;
		ACCESS_MASK GrantedAccess;
	};
} ALPC_HANDLE_ATTR, * PALPC_HANDLE_ATTR;

struct Message : PORT_MESSAGE {
	char Text[64];
};

PALPC_MESSAGE_ATTRIBUTES CreateMessageAttributes(ULONG attributes) {
	SIZE_T size = AlpcGetHeaderSize(attributes);
	auto msgAttr = (PALPC_MESSAGE_ATTRIBUTES)RtlAllocateHeap(NtCurrentPeb()->ProcessHeap, HEAP_ZERO_MEMORY, size);
	assert(msgAttr);
	auto status = AlpcInitializeMessageAttribute(attributes, msgAttr, size, &size);
	if (NT_SUCCESS(status))
		return msgAttr;
	RtlFreeHeap(NtCurrentPeb()->ProcessHeap, 0, msgAttr);
	return nullptr;
}

int main() {
	HANDLE hServerPort;
	UNICODE_STRING portName;
	RtlInitUnicodeString(&portName, L"\\SimpleServerPort2");
	OBJECT_ATTRIBUTES portAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(&portName, OBJ_CASE_INSENSITIVE);
	ALPC_PORT_ATTRIBUTES alpcPortAttr{};
	
	SECURITY_QUALITY_OF_SERVICE qos = { sizeof(qos) };
	qos.ImpersonationLevel = SecurityIdentification;
	qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
	qos.EffectiveOnly = TRUE;
	alpcPortAttr.SecurityQos = qos;
	alpcPortAttr.Flags = ALPC_PORT_FLAG_ACCEPT_DUP_HANDLES | ALPC_PORT_FLAG_ALLOW_IMPERSONATION;
	alpcPortAttr.DupObjectTypes = ALPC_ALL_OBJECT_TYPES;
	alpcPortAttr.MaxMessageLength = sizeof(Message);
	alpcPortAttr.MaxPoolUsage = 0x40000;
	alpcPortAttr.MaxSectionSize = 0x400000;
	alpcPortAttr.MaxTotalSectionSize = 0x400000 * 8;

	auto status = NtAlpcCreatePort(&hServerPort, &portAttr, &alpcPortAttr);
	if (!NT_SUCCESS(status)) {
		printf("NtAlpcCreatePort failed: 0x%X\n", status);
		return 1;
	}
	printf("Server port created: 0x%p\n", hServerPort);

	Message sendMsg, recvMsg;
	SIZE_T size = sizeof(sendMsg);
	Message* sendMessage = nullptr;
	Message* receiveMessage = &recvMsg;
	auto recvMsgAttr = CreateMessageAttributes(ALPC_FLG_MSG_ALL_ATTR);
	assert(recvMsgAttr);
	HANDLE hCommPort = nullptr;

	for (;;) {
		status = NtAlpcSendWaitReceivePort(hServerPort, 0,
			sendMessage, nullptr, receiveMessage, &size, recvMsgAttr, nullptr);
		if (!NT_SUCCESS(status))
			break;

		printf("Received msg type: 0x%X (ID: 0x%X)\n",
			receiveMessage->u2.s2.Type, receiveMessage->MessageId);
		if (recvMsgAttr->ValidAttributes & ALPC_FLG_MSG_TOKEN_ATTR) {
			auto tokenAttr = (PALPC_TOKEN_ATTR)AlpcGetMessageAttribute(recvMsgAttr, ALPC_FLG_MSG_TOKEN_ATTR);
			printf("Token Attr: Logon session: 0x%X:%08X", 
				tokenAttr->AuthenticationId.HighPart, tokenAttr->AuthenticationId.LowPart);
		}
		if (recvMsgAttr->ValidAttributes & ALPC_FLG_MSG_HANDLE_ATTR) {
			auto handleAttr = (PALPC_HANDLE_ATTR)AlpcGetMessageAttribute(recvMsgAttr, ALPC_FLG_MSG_HANDLE_ATTR);
			printf("Handle available: 0x%p\n", handleAttr->Handle);
		}

		if (recvMsgAttr->ValidAttributes & ALPC_FLG_MSG_DATAVIEW_ATTR) {
			auto dataView = (PALPC_DATA_VIEW_ATTR)AlpcGetMessageAttribute(recvMsgAttr, ALPC_FLG_MSG_DATAVIEW_ATTR);
			printf("Section map base address: 0x%p\n", dataView->ViewBase);
		}

		switch (receiveMessage->u2.s2.Type & 0xff) {
			case LPC_CONNECTION_REQUEST:
			{
				printf("Connection request received from PID: %u TID: %u Text: %s\n",
					HandleToULong(receiveMessage->ClientId.UniqueProcess),
					HandleToULong(receiveMessage->ClientId.UniqueThread),
					receiveMessage->Text);
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
			}

			case LPC_REQUEST:
				printf("\t%s\n", receiveMessage->Text);
				sendMessage = receiveMessage;
				strcpy_s(sendMessage->Text, "OK.");
				sendMessage->u1.s1.DataLength = sizeof(sendMsg.Text);
				sendMessage->u1.s1.TotalLength = sizeof(sendMsg);
				Sleep(3000);
				break;

			case LPC_PORT_CLOSED:
			case LPC_CLIENT_DIED:
				NtClose(hCommPort);
				break;

			default:
				printf("Other message type: 0x%X\n", receiveMessage->u2.s2.Type);
				break;
		}
	}

	NtClose(hServerPort);

	return 0;
}
