
#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>
#include <memory>
#include <cassert>

#pragma comment(lib, "ntdll")

#define ALPC_FLG_MSG_SEC_ATTR               0x80000000
#define ALPC_FLG_MSG_DATAVIEW_ATTR          0x40000000
#define ALPC_FLG_MSG_CONTEXT_ATTR           0x20000000
#define ALPC_FLG_MSG_HANDLE_ATTR            0x10000000
#define ALPC_FLG_MSG_TOKEN_ATTR             0x08000000
#define ALPC_FLG_MSG_DIRECT_ATTR            0x04000000
#define ALPC_FLG_MSG_WORK_ON_BEHALF_ATTR    0x02000000

typedef enum _ALPC_DUP_OBJECT_TYPE {
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
} DUP_OBJECT_TYPE;

#define OB_FILE_OBJECT_TYPE         0x00000001
#define OB_THREAD_OBJECT_TYPE       0x00000004
#define OB_SEMAPHORE_OBJECT_TYPE    0x00000008
#define OB_EVENT_OBJECT_TYPE        0x00000010
#define OB_PROCESS_OBJECT_TYPE      0x00000020
#define OB_MUTANT_OBJECT_TYPE       0x00000040
#define OB_SECTION_OBJECT_TYPE      0x00000080
#define OB_REG_KEY_OBJECT_TYPE      0x00000100
#define OB_TOKEN_OBJECT_TYPE        0x00000200
#define OB_COMPOSITION_OBJECT_TYPE  0x00000400
#define OB_JOB_OBJECT_TYPE          0x00000800

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
		PALPC_HANDLE_ATTR32 HandleAttrArray;
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

void Delay(int seconds) {
	LARGE_INTEGER time;
	time.QuadPart = -10000000LL * seconds;
	NtDelayExecution(FALSE, &time);
}

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

void DestroyMessageAttributes(PALPC_MESSAGE_ATTRIBUTES msgAttr) {
	RtlFreeHeap(NtCurrentPeb()->ProcessHeap, 0, msgAttr);
}

int main() {
	HANDLE hPort;
	UNICODE_STRING portName;
	RtlInitUnicodeString(&portName, L"\\SimpleServerPort2");
	NTSTATUS status;
	Message connMsg{};
	strcpy_s(connMsg.Text, "Abracadabra");
	connMsg.u1.s1.DataLength = sizeof(connMsg.Text);
	connMsg.u1.s1.TotalLength = sizeof(connMsg);
	Message replyMsg{};

	auto msgAttr = CreateMessageAttributes(ALPC_FLG_MSG_HANDLE_ATTR | ALPC_FLG_MSG_DATAVIEW_ATTR | ALPC_FLG_MSG_SEC_ATTR);
	assert(msgAttr);
	msgAttr->ValidAttributes = ALPC_FLG_MSG_HANDLE_ATTR | ALPC_FLG_MSG_DATAVIEW_ATTR;

	HANDLE hEvent;
	status = NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, nullptr, SynchronizationEvent, FALSE);
	assert(NT_SUCCESS(status));

	auto handleAttr = (PALPC_HANDLE_ATTR)AlpcGetMessageAttribute(msgAttr, ALPC_FLG_MSG_HANDLE_ATTR);
	handleAttr->Flags = 0;
	handleAttr->Handle = hEvent;
	handleAttr->ObjectType = OB_EVENT_OBJECT_TYPE;
	handleAttr->DesiredAccess = EVENT_MODIFY_STATE | SYNCHRONIZE;

	for (int i = 0; i < 10; i++) {
		status = NtAlpcConnectPort(&hPort, &portName, nullptr, nullptr,
			ALPC_MSGFLG_SYNC_REQUEST, nullptr, &connMsg, nullptr,
			nullptr, nullptr, nullptr);
		if (NT_SUCCESS(status))
			break;
		printf("NtAlpcConnectPort failed: 0x%X\n", status);
		Delay(1);
	}
	if (!NT_SUCCESS(status)) {
		printf("Failed to connect (0x%X)\n", status);
		return status;
	}

	printf("Client port connected: 0x%p\n", hPort);
	LARGE_INTEGER time;
	TIME_FIELDS tf;

	ALPC_HANDLE hPortSection;
	SIZE_T actualSize;
	status = NtAlpcCreatePortSection(hPort, 0, nullptr, 1 << 12, &hPortSection, &actualSize);
	assert(NT_SUCCESS(status));
	auto dataView = (PALPC_DATA_VIEW_ATTR)AlpcGetMessageAttribute(msgAttr, ALPC_FLG_MSG_DATAVIEW_ATTR);
	assert(dataView);
	dataView->Flags = 0;
	dataView->SectionHandle = hPortSection;
	dataView->ViewSize = actualSize;

	status = NtAlpcCreateSectionView(hPort, 0, dataView);
	assert(NT_SUCCESS(status));


	ALPC_PORT_ASSOCIATE_COMPLETION_PORT iocp{};
	NtCreateIoCompletion(&iocp.CompletionPort, IO_COMPLETION_ALL_ACCESS, nullptr, 4);
	//iocp.CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 4);
	status = NtAlpcSetInformation(hPort, AlpcAssociateCompletionPortInformation, &iocp, sizeof(iocp));
	assert(NT_SUCCESS(status));

	for (;;) {
		Message msg{};
		NtQuerySystemTime(&time);
		RtlSystemTimeToLocalTime(&time, &time);
		RtlTimeToTimeFields(&time, &tf);
		sprintf_s(msg.Text, "The Time is %02d:%02d:%02d.%03d",
			tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
		msg.u1.s1.DataLength = sizeof(msg.Text);
		msg.u1.s1.TotalLength = sizeof(msg);

		Message reply{};
		LARGE_INTEGER li{};
		SIZE_T msgLen = sizeof(reply);
		status = NtAlpcSendWaitReceivePort(hPort, 0, &msg,
			msgAttr, nullptr, nullptr, nullptr, nullptr);
		if (!NT_SUCCESS(status)) {
			printf("NtAlpcSendWaitReceivePort failed: 0x%X\n", status);
			break;
		}

		printf("Sent message %s.\n", msg.Text);
		printf("Waiting for reply...\n");
		struct Context {
			HANDLE hPort;
			SIZE_T* MsgLen;
			Message* Msg;
			HANDLE hIocp;
		};

		Context ctx{ hPort, &msgLen, &reply, iocp.CompletionPort };

		TrySubmitThreadpoolCallback([](auto, auto p) {
			auto ctx = (Context*)p;
			DWORD bytes;
			ULONG_PTR key;
			OVERLAPPED* ov;
			GetQueuedCompletionStatus(ctx->hIocp, &bytes, &key, &ov, INFINITE);
			auto status = NtAlpcSendWaitReceivePort(ctx->hPort, 0, nullptr,
				nullptr, ctx->Msg, ctx->MsgLen, nullptr, nullptr);
			printf("Received reply from PID: %u TID: %u: %s\n",
				HandleToULong(ctx->Msg->ClientId.UniqueProcess),
				HandleToULong(ctx->Msg->ClientId.UniqueThread),
				ctx->Msg->Text);
			}, &ctx, nullptr);

		auto tick = GetTickCount();
		while (GetTickCount() - tick < 6000) {
			printf(".");
			Sleep(300);
		}
		printf("\n");

		if (GetAsyncKeyState(VK_ESCAPE) < 0)
			break;

		Delay(5);
	}
	NtClose(hPort);

	return 0;
}

