#include "pch.h"

ClientContext::ClientContext() {
	ZeroMemory(&overlapped, sizeof(overlapped));
	acceptSocket = INVALID_SOCKET;
	wsaBuf.buf = new CHAR[BUFF_SIZE]{ 0 };
	wsaBuf.len = BUFF_SIZE;
	operation = IO_OPERATION_TYPE::NULL_POSTED;
}

ClientContext::ClientContext(const ClientContext& ioContext) {
	RtlCopyMemory(&overlapped, &ioContext.overlapped, sizeof(OVERLAPPED));
	acceptSocket = ioContext.acceptSocket;
	wsaBuf.buf = new char[BUFF_SIZE] {0};
	wsaBuf.len = BUFF_SIZE;
	RtlCopyMemory(wsaBuf.buf, ioContext.wsaBuf.buf, BUFF_SIZE);
	operation = ioContext.operation;
}

ClientContext::~ClientContext() {
	if (wsaBuf.buf != nullptr) {
		delete wsaBuf.buf;
		wsaBuf.buf = nullptr;
		wsaBuf.len = 0;
	}
}

void ClientContext::Reset() {
	if (wsaBuf.buf != NULL)
		ZeroMemory(wsaBuf.buf, BUFF_SIZE);
	else
		wsaBuf.buf = new char[BUFF_SIZE] {0};
	ZeroMemory(&overlapped, sizeof(overlapped));
	operation = IO_OPERATION_TYPE::NULL_POSTED;
	acceptSocket = INVALID_SOCKET;
}

ClientContext& ClientContext::operator=(const ClientContext& ioContext) {
	if (&ioContext == this) {
		return *this;
	}
	RtlCopyMemory(&overlapped, &ioContext.overlapped, sizeof(OVERLAPPED));
	acceptSocket = ioContext.acceptSocket;
	RtlCopyMemory(wsaBuf.buf, ioContext.wsaBuf.buf, BUFF_SIZE);
	wsaBuf.len = BUFF_SIZE;
	operation = ioContext.operation;
	return *this;
}

void ContextHashTable::removeContext(ClientContext* pContext) {
	m_contexts.erase(pContext);
	//delete pContext;
	ContextPool::getInstance().ReleaseClientContext(pContext);
}