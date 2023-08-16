#pragma once
#include "ClientContext.h"
#include "ThreadPool.h"


// ����һЩ����
constexpr int DEFAULT_PORT = 9527; //�˿�
constexpr int IOCP_THREAD = 2;
constexpr int WORKER_THREADS_PER_PROCESSOR = 2;

class IOCPBase {
protected:
	IOCPBase();
	IOCPBase(const IOCPBase&) = delete;
	IOCPBase& operator=(const IOCPBase&) = delete;
	virtual ~IOCPBase() = 0;
	bool StartServer(int nPort = DEFAULT_PORT, int iocpThreadCnt = IOCP_THREAD);
	bool DispatchWorker(const Worker& worker);
	BOOL DoSend(const ClientContext* pContext);
private:
	BOOL PostAccepts();
	BOOL PostRecv(SOCKET acceptSocket);
	BOOL PostSend(SOCKET acceptSocket, const std::string& strData);
	BOOL DoClose(ClientContext* pContext);
	BOOL DoRecv(const ClientContext* pContext);
	BOOL DoAccpet(const ClientContext* pContext);
protected:
	// �¼�֪ͨ����(���������ش��庯��)
	// ������
	virtual void OnConnectionEstablished(const ClientContext* pContext);
	// ���ӹر�
	virtual void OnConnectionClosed(const ClientContext* pContext);
	// �����Ϸ�������
	virtual void OnConnectionError(const ClientContext* pContext);
	// ���������
	virtual void OnRecvCompleted(const ClientContext* pContext);
	// д�������
	virtual void OnSendCompleted(const ClientContext* pContext);

private:
	// ��ȡ����CPU������
	int GetNumOfProcessors();
	static DWORD WINAPI IOCPThreadEntry(LPVOID arg);
	void IOCPThread();
private:
	HANDLE hCompletionPort;
	SOCKET listenSocket;
	ThreadPool m_pool;
	ContextHashTable m_contexts;
};


