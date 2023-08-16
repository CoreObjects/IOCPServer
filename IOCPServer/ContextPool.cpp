#include "pch.h"

ContextPool::ContextPool(int nContextCount /*= INIT_IOCONTEXT_NUM*/) {
	m_lock.lock();
	for (size_t i = 0; i < INIT_IOCONTEXT_NUM; i++) {
		ClientContext* context = new ClientContext;
		m_contexts.push_back(context);
	}
	m_lock.unlock();
}

ContextPool::~ContextPool() {
	m_lock.lock();
	for (auto& context : m_contexts) {
		delete context;
		context = nullptr;
	}
	m_lock.unlock();
}

ClientContext* ContextPool::AllocateContext() {
	ClientContext* pContext = nullptr;
	m_lock.lock();
	if (!m_contexts.empty()) {
		pContext = m_contexts.back();
		m_contexts.pop_back();
	}
	else {
		pContext = new ClientContext;
	}
	m_lock.unlock();
	return pContext;
}

void ContextPool::ReleaseClientContext(ClientContext* pContext) {
	pContext->Reset();
	m_lock.lock();
	m_contexts.push_back(pContext);
	m_lock.unlock();
}
