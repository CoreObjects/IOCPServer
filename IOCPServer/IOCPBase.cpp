#include "pch.h"

int nCount = 0;

bool IOCPBase::StartServer(int nPort, int iocpThreadCnt) {
	// ���������׽���
	listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "���������׽���ʧ��\n";
		WSACleanup();
		return false;
	}

	// �󶨵�ַ�Ͷ˿�
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(nPort);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(listenSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		std::cerr << "��ʧ��\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	// ��ʼ����
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "����ʧ��\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	// ����I/O��ɶ˿�
	hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (!hCompletionPort) {
		std::cerr << "����I/O��ɶ˿�ʧ��\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	// ���������׽��ֺ���ɶ˿�
	if (!CreateIoCompletionPort((HANDLE)listenSocket, hCompletionPort, 0, 0)) {
		std::cerr << "���������׽��ֺ���ɶ˿�ʧ��\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	// Ͷ���첽����
	// ClientContext* ioContext = new ClientContext;
	if (!PostAccepts()) {
		std::cerr << "�����첽����ʧ��\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	//���������߳�
	m_pool.StartAllThread();

	//��ǲ2����ȡ��ɶ����߳�
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
		std::cerr << "WSAStartupʧ��\n";
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
		std::cerr << "��ȡAcceptExʧ��\n";
		return FALSE;
	}
	if (!lpfnAcceptEx(listenSocket, ioContext->acceptSocket, ioContext->wsaBuf.buf, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, nullptr, &ioContext->overlapped)) {
		if (WSAGetLastError() != ERROR_IO_PENDING) {
			std::cerr << "PostAcceptsʧ��\n";
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
	//1. ����socket����ɶ˿ڰ�
	CreateIoCompletionPort((HANDLE)pContext->acceptSocket, hCompletionPort, 0, 0); // ���������ӵ��׽��ֺ���ɶ˿�
	//2. Ͷ���첽Recv
	BOOL bRet = PostRecv(pContext->acceptSocket);
	//3. �����첽����
	bRet &= PostAccepts();
	//֪ͨ���ӽ���
	OnConnectionEstablished(pContext);
	return bRet;
}

BOOL IOCPBase::DoRecv(const ClientContext* pContext) {
	/*DoSend(pContext);*/
	//1.����Ͷ��recv
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
	Tool::OutputDebugPrintf("�յ�����, Socket = 0X%X\r\n", pContext->acceptSocket);
}

void IOCPBase::OnConnectionClosed(const ClientContext* pContext) {
	Tool::OutputDebugPrintf("���ӹر�, Socket = 0X%X\r\n", pContext->acceptSocket);
}

void IOCPBase::OnConnectionError(const ClientContext* pContext) {
	OutputDebugStringA("��������!!\r\n");
}

void IOCPBase::OnRecvCompleted(const ClientContext* pContext) {
	Tool::OutputDebugPrintf("�յ�Socket = 0X%X�����ݣ�%s\r\n", pContext->acceptSocket, pContext->wsaBuf.buf);
}

void IOCPBase::OnSendCompleted(const ClientContext* pContext) {
	Tool::OutputDebugPrintf("���͸�Socket = 0X%X�����ݣ�%s\r\n", pContext->acceptSocket, pContext->wsaBuf.buf);
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
			// �ж��Ƿ��пͻ��˶Ͽ�
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
