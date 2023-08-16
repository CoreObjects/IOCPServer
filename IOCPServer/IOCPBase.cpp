#include "pch.h"

int nCount = 0;

bool IOCPBase::StartServer(int nPort, int iocpThreadCnt) {
	// 创建监听套接字
	listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "创建监听套接字失败\n";
		WSACleanup();
		return false;
	}

	// 绑定地址和端口
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(nPort);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(listenSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		std::cerr << "绑定失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	// 开始监听
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "监听失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	// 创建I/O完成端口
	hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (!hCompletionPort) {
		std::cerr << "创建I/O完成端口失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	// 关联监听套接字和完成端口
	if (!CreateIoCompletionPort((HANDLE)listenSocket, hCompletionPort, 0, 0)) {
		std::cerr << "关联监听套接字和完成端口失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	// 投递异步接受
	// ClientContext* ioContext = new ClientContext;
	if (!PostAccepts()) {
		std::cerr << "启动异步接受失败\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	//启动所有线程
	m_pool.StartAllThread();

	//派遣2个获取完成队列线程
	/*stdcallWorker IOCPWorker */
	stdcallWorker IOCPWorker(IOCPBase::IOCPThreadEntry, this, false);
	for (int i = 0; i < iocpThreadCnt; i++) {
		DispatchWorker(IOCPWorker);
	}
	return true;
}

bool IOCPBase::DispatchWorker(const Worker& worker) {
	return m_pool.DispatchWorker(worker);
}

IOCPBase::IOCPBase() :hCompletionPort(INVALID_HANDLE_VALUE), listenSocket(INVALID_SOCKET) {
	WSADATA wsData;
	if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
		std::cerr << "WSAStartup失败\n";
		return;
	}
	m_pool.resize(GetNumOfProcessors() * WORKER_THREADS_PER_PROCESSOR);
}

IOCPBase::~IOCPBase() {
	closesocket(listenSocket);
	CloseHandle(hCompletionPort);
	WSACleanup();
}

BOOL IOCPBase::PostAccepts() {
	ClientContext* ioContext = ContextPool::getInstance().AllocateContext();
	nCount++;
	Tool::OutputDebugPrintf("new accpetContext Count:%d\r\n",nCount);
	m_contexts.addContext(ioContext);
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

BOOL IOCPBase::PostRecv(SOCKET acceptSocket) {
	DWORD dwFlags = 0, dwBytes = 0;
	ClientContext* ioContext = ContextPool::getInstance().AllocateContext();
	nCount++;
	Tool::OutputDebugPrintf("new recvContext Count:%d\r\n", nCount);
	m_contexts.addContext(ioContext);
	ioContext->operation = IO_OPERATION_TYPE::RECV_POSTED;
	ioContext->acceptSocket = acceptSocket;
	int nBytesRecv = WSARecv(ioContext->acceptSocket, &ioContext->wsaBuf, 1, &dwBytes, &dwFlags, &ioContext->overlapped, NULL);
	return TRUE;
}

BOOL IOCPBase::PostSend(SOCKET acceptSocket, const std::string& strData) {
	DWORD dwBytes = 0, dwFlags = 0;
	ClientContext* ioContext = ContextPool::getInstance().AllocateContext();
	nCount++;
	Tool::OutputDebugPrintf("new sendContext Count:%d\r\n", nCount);
	m_contexts.addContext(ioContext);
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

BOOL IOCPBase::DoAccpet(const ClientContext* pContext) {
	//1. 将新socket和完成端口绑定
	CreateIoCompletionPort((HANDLE)pContext->acceptSocket, hCompletionPort, 0, 0); // 关联新连接的套接字和完成端口
	//2. 投递异步Recv
	BOOL bRet = PostRecv(pContext->acceptSocket);
	//3. 继续异步接受
	bRet &= PostAccepts();
	//通知连接建立
	OnConnectionEstablished(pContext);
	return bRet;
}

BOOL IOCPBase::DoRecv(const ClientContext* pContext) {
	/*DoSend(pContext);*/
	//1.重新投递recv
	BOOL bRet = PostRecv(pContext->acceptSocket);
	OnRecvCompleted(pContext);
	return bRet;
}

BOOL IOCPBase::DoSend(const ClientContext* pContext) {
	BOOL bRet = PostSend(pContext->acceptSocket, pContext->wsaBuf.buf);
	OnSendCompleted(pContext);
	return bRet;
}

BOOL IOCPBase::DoClose(ClientContext* pContext) {
	if (pContext->acceptSocket != INVALID_SOCKET) {
		closesocket(pContext->acceptSocket);
		pContext->acceptSocket = INVALID_SOCKET;
	}
	OnConnectionClosed(pContext);
	return TRUE;
}

void IOCPBase::OnConnectionEstablished(const ClientContext* pContext) {
	Tool::OutputDebugPrintf("收到连接, Socket = 0X%X\r\n", pContext->acceptSocket);
}

void IOCPBase::OnConnectionClosed(const ClientContext* pContext) {
	Tool::OutputDebugPrintf("连接关闭, Socket = 0X%X\r\n", pContext->acceptSocket);
}

void IOCPBase::OnConnectionError(const ClientContext* pContext) {
	OutputDebugStringA("发生错误!!\r\n");
}

void IOCPBase::OnRecvCompleted(const ClientContext* pContext) {
	Tool::OutputDebugPrintf("收到Socket = 0X%X的数据：%s\r\n", pContext->acceptSocket, pContext->wsaBuf.buf);
}

void IOCPBase::OnSendCompleted(const ClientContext* pContext) {
	Tool::OutputDebugPrintf("发送给Socket = 0X%X的数据：%s\r\n", pContext->acceptSocket, pContext->wsaBuf.buf);
}

int IOCPBase::GetNumOfProcessors() {
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

DWORD WINAPI IOCPBase::IOCPThreadEntry(LPVOID arg) {
	IOCPBase* thiz = (IOCPBase*)arg;
	thiz->IOCPThread();
	return 0;
}

void IOCPBase::IOCPThread() {
//	while (true) {
		DWORD transferred = 0;
		ULONG_PTR completionKey = 0;
		OVERLAPPED* pOverlapped = nullptr;
		BOOL bRet = GetQueuedCompletionStatus(hCompletionPort, &transferred, &completionKey, &pOverlapped, INFINITE);
		ClientContext* pContext = CONTAINING_RECORD(pOverlapped, ClientContext, overlapped);
		if (bRet != 0) {
			// 判断是否有客户端断开
			if (transferred == 0 && (pContext->operation == IO_OPERATION_TYPE::SEND_POSTED || pContext->operation == IO_OPERATION_TYPE::RECV_POSTED)) {
				DoClose(pContext);
			}
			else {
				switch (pContext->operation) {

				case IO_OPERATION_TYPE::ACCEPT_POSTED:
				{
					serverThiscallWorker<IOCPBase> acceptWorker(this, &IOCPBase::DoAccpet, pContext);
					while (DispatchWorker(acceptWorker) == false);
				}
				break;
				case IO_OPERATION_TYPE::RECV_POSTED:
				{
					serverThiscallWorker<IOCPBase> recvWorker(this, &IOCPBase::DoRecv, pContext);
					while (DispatchWorker(recvWorker) == false);
				}break;
				case IO_OPERATION_TYPE::SEND_POSTED:
				{

				}
				break;
				default:
					break;
				}


			}
			nCount--;
			Tool::OutputDebugPrintf("delete %d Context Count:%d\r\n",pContext->operation, nCount);
			m_contexts.removeContext(pContext);
		}
//	}
}
