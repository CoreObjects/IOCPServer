#include "pch.h"
//全局变量声明
HANDLE hCompletionPort;
SOCKET listenSocket;
// 函数声明
BOOL PostAccepts();
BOOL PostRecv(SOCKET acceptSocket);
BOOL PostSend(SOCKET acceptSocket, const std::string& strData);
BOOL DoAccpet(const ClientContext* pContext);
BOOL DoRecv(const ClientContext* pContext);
BOOL DoSend(const ClientContext* pContext);
BOOL DoClose(ClientContext* pContext);

class ServercdeclWorker : public CloneableWorker<ServercdeclWorker> {
public:
	ServercdeclWorker(fncdeclWorker fn, ClientContext* pContext, bool isOne = true)
		: CloneableWorker<ServercdeclWorker>(isOne), cdeclfunc(fn) {
		Context = *(ClientContext*)pContext;
	}
	virtual void operator()() const override {
		cdeclfunc((void*)&Context);
	}
	ClientContext Context;
	fncdeclWorker cdeclfunc;
};

BOOL DoAccpet(const ClientContext* pContext) {
	OutputDebugStringA("收到连接!!\r\n");
	//1. 将新socket和完成端口绑定
	CreateIoCompletionPort((HANDLE)pContext->acceptSocket, hCompletionPort, 0, 0); // 关联新连接的套接字和完成端口
	//2. 投递异步Recv
	BOOL bRet = PostRecv(pContext->acceptSocket);
	//3. 继续异步接受
	bRet &= PostAccepts();
	return bRet;
}

BOOL DoRecv(const ClientContext* pContext) {
	//1.重新投递recv
	return PostRecv(pContext->acceptSocket);
}

BOOL DoSend(const ClientContext* pContext) {
	return PostSend(pContext->acceptSocket, pContext->wsaBuf.buf);
}

BOOL DoClose(ClientContext* pContext) {
	if (pContext->acceptSocket != INVALID_SOCKET) {
		closesocket(pContext->acceptSocket);
		pContext->acceptSocket = INVALID_SOCKET;
	}
	return TRUE;
}

class AcceptWorker : public CloneableWorker<AcceptWorker> {
public:
	AcceptWorker(ClientContext* pContext) {
		accpetContext = *pContext;
	}
	virtual void operator()() const override {
		DoAccpet(&accpetContext);
	}
	ClientContext accpetContext;
};

class RecvWorker : public CloneableWorker<RecvWorker> {
public:
	RecvWorker(ClientContext* pContext) {
		recvContext = *pContext;
	}
	virtual void operator()() const override {
		DoRecv(&recvContext);
		//		DoSend(&recvContext);
		//		//2.投递send
		//		PostSend(recvContext.acceptSocket, recvContext.wsaBuf.buf);
	}
	ClientContext recvContext;
};

class SendWorker : public CloneableWorker<SendWorker> {
public:
	SendWorker(ClientContext* pContext) {
		sendContext = *pContext;
	}
	virtual void operator()() const override {
		DoSend(&sendContext);
	}
	ClientContext sendContext;
};


int _main() {
	// 初始化Winsock
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
		std::cerr << "WSAStartup失败\n";
		return 1;
	}

	// 创建监听套接字
	listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "创建监听套接字失败\n";
		WSACleanup();
		return 1;
	}

	// 绑定地址和端口
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(DEFAULT_PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(listenSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		std::cerr << "绑定失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// 开始监听
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "监听失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// 创建I/O完成端口
	hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (!hCompletionPort) {
		std::cerr << "创建I/O完成端口失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// 关联监听套接字和完成端口
	if (!CreateIoCompletionPort((HANDLE)listenSocket, hCompletionPort, 0, 0)) {
		std::cerr << "关联监听套接字和完成端口失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// 开始异步接受
	// ClientContext* ioContext = new ClientContext;
	if (!PostAccepts()) {
		std::cerr << "启动异步接受失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	ThreadPool pool(10);
	pool.StartAllThread();
	// 主循环等待完成
	while (true) {
		DWORD transferred = 0;
		ULONG_PTR completionKey = 0;
		OVERLAPPED* pOverlapped = nullptr;
		BOOL bRet = GetQueuedCompletionStatus(hCompletionPort, &transferred, &completionKey, &pOverlapped, INFINITE);
		ClientContext* pContext = CONTAINING_RECORD(pOverlapped, ClientContext, overlapped);
		if (bRet != 0) {
			// 判断是否有客户端断开
			if (transferred == 0 && (pContext->operation == IO_OPERATION_TYPE::SEND_POSTED || pContext->operation == IO_OPERATION_TYPE::RECV_POSTED)) {
				DoClose(pContext);
				delete pContext;
				continue;
			}
			else {
				switch (pContext->operation) {

				case IO_OPERATION_TYPE::ACCEPT_POSTED:
				{
//					DoAccpet(pContext);
 					/*AcceptWorker acceptWorker(pContext);*/
					ServercdeclWorker acceptWorker((fncdeclWorker)DoAccpet, pContext);
  					while(pool.DispatchWorker(acceptWorker) == false);
				}
				break;
				case IO_OPERATION_TYPE::RECV_POSTED:
				{
//					DoRecv(pContext);
					ServercdeclWorker recvWorker((fncdeclWorker)DoRecv, pContext);
 					while (pool.DispatchWorker(recvWorker) == false);
				}break;
				case IO_OPERATION_TYPE::SEND_POSTED:
				{

				}
				break;
				default:
					break;
				}
			}
			delete pContext;
			pContext = nullptr;
		}
	}
	// 清理
	closesocket(listenSocket);
	CloseHandle(hCompletionPort);
	WSACleanup();

	return 0;
}

BOOL PostAccepts() {

	ClientContext* ioContext = new ClientContext;
	ZeroMemory(&ioContext->overlapped, sizeof(ioContext->overlapped));
	ioContext->acceptSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	ioContext->operation = IO_OPERATION_TYPE::ACCEPT_POSTED;
	LPFN_ACCEPTEX lpfnAcceptEx = nullptr;
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx), &lpfnAcceptEx, sizeof(lpfnAcceptEx), &dwBytes, nullptr, nullptr)) {
		std::cerr << "获取AcceptEx失败\n";
		return FALSE;
	}
	if (!lpfnAcceptEx(listenSocket, ioContext->acceptSocket, ioContext->wsaBuf.buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, nullptr, &ioContext->overlapped)) {
		if (WSAGetLastError() != ERROR_IO_PENDING) {
			std::cerr << "PostAccepts失败\n";
			return FALSE;
		}
	}

	return TRUE;
}

BOOL PostRecv(SOCKET acceptSocket) {
	DWORD dwFlags = 0, dwBytes = 0;
	ClientContext* ioContext = new ClientContext;
	ioContext->operation = IO_OPERATION_TYPE::RECV_POSTED;
	ioContext->acceptSocket = acceptSocket;
	int nBytesRecv = WSARecv(ioContext->acceptSocket, &ioContext->wsaBuf, 1, &dwBytes, &dwFlags, &ioContext->overlapped, NULL);
	return TRUE;
}

BOOL PostSend(SOCKET acceptSocket, const std::string& strData) {
	DWORD dwBytes = 0, dwFlags = 0;
	ClientContext* ioContext = new ClientContext;
	ioContext->operation = IO_OPERATION_TYPE::SEND_POSTED;
	ioContext->acceptSocket = acceptSocket;
	memcpy(ioContext->wsaBuf.buf, strData.c_str(), strData.length());
	if (::WSASend(ioContext->acceptSocket, &ioContext->wsaBuf, 1, &dwBytes, dwFlags, &ioContext->overlapped, NULL) != NO_ERROR) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			return false;
		}
	}
	return true;
}