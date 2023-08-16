#pragma once
#include "Base.h"

constexpr int BUFF_SIZE = 4096;  //缓冲区大小
enum class IO_OPERATION_TYPE {
	NULL_POSTED,		// 用于初始化，无意义
	ACCEPT_POSTED,		// 投递Accept操作
	SEND_POSTED,		// 投递Send操作
	RECV_POSTED,		// 投递Recv操作
};

class ClientContext {
public:
	ClientContext();
	ClientContext(const ClientContext& ioContext);
	ClientContext& operator=(const ClientContext& ioContext);
	~ClientContext();
	void Reset();
public:
	OVERLAPPED overlapped;
	SOCKET acceptSocket;
	WSABUF wsaBuf;
	IO_OPERATION_TYPE operation;
};

class ContextHashTable {
public:
	~ContextHashTable() {
		for (auto& context : m_contexts) {
			if(context!=nullptr)
			delete context;
		}
		m_contexts.clear();
	}
	void addContext(ClientContext* pContext) {
		m_contexts.insert(pContext);
	}
	void removeContext(ClientContext* pContext);
private:
	std::unordered_set<ClientContext*> m_contexts;
};


