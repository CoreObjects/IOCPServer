#pragma once
#include "ClientContext.h"
#include "ThreadPool.h"


// 定义一些常量
constexpr int DEFAULT_PORT = 9527; //端口
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
	// 事件通知函数(派生类重载此族函数)
	// 新连接
	virtual void OnConnectionEstablished(const ClientContext* pContext);
	// 连接关闭
	virtual void OnConnectionClosed(const ClientContext* pContext);
	// 连接上发生错误
	virtual void OnConnectionError(const ClientContext* pContext);
	// 读操作完成
	virtual void OnRecvCompleted(const ClientContext* pContext);
	// 写操作完成
	virtual void OnSendCompleted(const ClientContext* pContext);

private:
	// 获取本机CPU核心数
	int GetNumOfProcessors();
	static DWORD WINAPI IOCPThreadEntry(LPVOID arg);
	void IOCPThread();
private:
	HANDLE hCompletionPort;
	SOCKET listenSocket;
	ThreadPool m_pool;
	ContextHashTable m_contexts;
};


