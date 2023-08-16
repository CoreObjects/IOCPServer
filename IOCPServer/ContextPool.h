#pragma once
#include "ClientContext.h"

#define INIT_IOCONTEXT_NUM (100)

class ContextPool {
public:
	// ��ȡ��������Ľӿ�
	static ContextPool& getInstance(int nContextCount = INIT_IOCONTEXT_NUM) {
		static ContextPool instance(nContextCount);
		return instance;
	}

private:
	// ˽�л����캯��
	ContextPool(int nContextCount = INIT_IOCONTEXT_NUM);

	// ˽�л����ƹ��캯��
	ContextPool(const ContextPool&) = delete;

	// ˽�л���ֵ�����
	ContextPool& operator=(const ContextPool&) = delete;

	// ��������
	~ContextPool();

public:
	// ����ClientContext
	ClientContext* AllocateContext();

	// �ͷ�ClientContext
	void ReleaseClientContext(ClientContext* pContext);

private:
	std::list<ClientContext*> m_contexts; // ClientContext�б�
	std::mutex m_lock;                    // ������
};
