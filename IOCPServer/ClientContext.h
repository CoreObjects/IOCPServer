#pragma once
#include "Base.h"

constexpr int BUFF_SIZE = 4096;  //��������С
enum class IO_OPERATION_TYPE {
	NULL_POSTED,		// ���ڳ�ʼ����������
	ACCEPT_POSTED,		// Ͷ��Accept����
	SEND_POSTED,		// Ͷ��Send����
	RECV_POSTED,		// Ͷ��Recv����
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


