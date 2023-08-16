#pragma once
#include "ClientContext.h"

#define INIT_IOCONTEXT_NUM (100)

class ContextPool {
public:
	// 获取单例对象的接口
	static ContextPool& getInstance(int nContextCount = INIT_IOCONTEXT_NUM) {
		static ContextPool instance(nContextCount);
		return instance;
	}

private:
	// 私有化构造函数
	ContextPool(int nContextCount = INIT_IOCONTEXT_NUM);

	// 私有化复制构造函数
	ContextPool(const ContextPool&) = delete;

	// 私有化赋值运算符
	ContextPool& operator=(const ContextPool&) = delete;

	// 析构函数
	~ContextPool();

public:
	// 分配ClientContext
	ClientContext* AllocateContext();

	// 释放ClientContext
	void ReleaseClientContext(ClientContext* pContext);

private:
	std::list<ClientContext*> m_contexts; // ClientContext列表
	std::mutex m_lock;                    // 互斥锁
};
