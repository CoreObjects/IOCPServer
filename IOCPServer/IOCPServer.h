#pragma once
#include "IOCPBase.h"

template<typename ClassName>
class IOCPServer : public IOCPBase {
protected:
	virtual void OnRecvCompleted(const ClientContext* pContext) {
		IOCPBase::OnRecvCompleted(pContext);
		serverThiscallWorker<ClassName> dealWorker((ClassName*)this, &ClassName::dealRecvData, pContext);
		while (DispatchWorker(dealWorker) == false);
	}
	IOCPServer() {
		StartServer();
	}
	virtual BOOL dealRecvData(const ClientContext* pContext) = 0;
	IOCPServer(const IOCPServer&) = delete;
	IOCPServer& operator=(const IOCPServer&) = delete;
	virtual ~IOCPServer() = 0 {};
};
