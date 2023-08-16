#include "pch.h"
#define PORT 9527
#define WORKER_THREADS_PER_PROCESSOR 2

struct TB_ClientContext {
	OVERLAPPED overlapped;
	SOCKET clientSocket;
	char buffer[1024];
	WSABUF wsaBuf;
	// 定义操作类型，用于区分接收和发送
	enum { OP_READ, OP_WRITE } operation;
};

DWORD WINAPI WorkerThread(LPVOID CompletionPortID) {
	HANDLE CompletionPort = (HANDLE)CompletionPortID;
	DWORD bytesTransferred;
	TB_ClientContext* context = nullptr;
	LPOVERLAPPED overlapped;

	while (GetQueuedCompletionStatus(CompletionPort, &bytesTransferred, (PULONG_PTR)&context, &overlapped, INFINITE)) {
		if (bytesTransferred == 0 || context == nullptr) {
			if (context) {
				closesocket(context->clientSocket);
				delete context;
			}
			continue;
		}

		// 处理客户端的请求（例如，回显）
		context->wsaBuf.len = bytesTransferred;
		WSASend(context->clientSocket, &(context->wsaBuf), 1, NULL, 0, overlapped, NULL);
	}

	return 0;
}


int t_main() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET listenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(listenSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
	listen(listenSocket, SOMAXCONN);

	HANDLE completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	for (DWORD i = 0; i < systemInfo.dwNumberOfProcessors * WORKER_THREADS_PER_PROCESSOR; ++i) {
		HANDLE threadHandle = CreateThread(NULL, 0, WorkerThread, completionPort, 0, NULL);
		CloseHandle(threadHandle);
	}
	while (true) {
		SOCKET clientSocket = accept(listenSocket, NULL, NULL);
		TB_ClientContext* context = new TB_ClientContext();
		ZeroMemory(&(context->overlapped), sizeof(OVERLAPPED)); // 清零 OVERLAPPED 结构
		context->clientSocket = clientSocket;
		context->wsaBuf.buf = context->buffer;
		context->wsaBuf.len = sizeof(context->buffer);
		context->operation = TB_ClientContext::OP_READ;
		CreateIoCompletionPort((HANDLE)clientSocket, completionPort, (ULONG_PTR)context, 0);
		DWORD flags = 0;
		WSARecv(clientSocket, &(context->wsaBuf), 1, NULL, &flags, &(context->overlapped), NULL);
	}

	closesocket(listenSocket);
	WSACleanup();

	return 0;
}