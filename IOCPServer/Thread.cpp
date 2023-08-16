#include "pch.h"

Thread::Thread() {
	m_hThread = nullptr;
	m_bStatus = false;
	m_ThreadId = 0;
	m_worker = nullptr;
}

Thread::~Thread() {
	Stop();
	if (m_worker != nullptr) {
		delete m_worker;
		m_worker = nullptr;
	}
}

bool Thread::Start() {
	if (m_bStatus == false) {
		m_bStatus = true;
		m_hThread = CreateThread(NULL, 0, Thread::ThreadEntry, this, 0, &m_ThreadId);
	}
	if (!IsValid()) {
		m_bStatus = false;
	}
	return m_bStatus;
}

bool Thread::Stop() {
	if (!IsValid()) {
		m_bStatus = false;
		return true;
	}
	else {
		m_bStatus = false;
		DWORD dwRet = WaitForSingleObject(m_hThread, 100);
		if (dwRet == WAIT_TIMEOUT) {
			bool ret = TerminateThread(m_hThread, 0);
			if (ret == true) {
				CloseHandle(m_hThread);
				m_hThread = INVALID_HANDLE_VALUE;
				m_ThreadId = 0;
			}
			return ret;
		}
		else if (dwRet == WAIT_OBJECT_0) {
			CloseHandle(m_hThread);
			return true;
		}
		else {
			return false;
		}
	}
}

bool Thread::IsValid() {
	if (m_hThread == NULL || m_hThread == INVALID_HANDLE_VALUE) {
		return false;
	}
	else {
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}
}

bool Thread::SetWorker(const Worker& worker) {
	if (m_worker != nullptr) {
		delete m_worker;
		m_worker = nullptr;
	}
	m_worker = static_cast<Worker*>(worker.Clone()); // 使用克隆方法
	return true;
}

bool Thread::IsIdle() {//是否空闲
	if (m_worker == nullptr) {
		return true;
	}
	return false;
}

DWORD WINAPI Thread::ThreadEntry(void* arg) {
	Thread* thiz = (Thread*)arg;
	if (thiz) {
		thiz->ThreadMain();
	}
	return 0;
}

void Thread::ThreadMain() {
	while (m_bStatus) {
		if (m_worker != nullptr) {
			(*m_worker)();
			if (m_worker->IsOne()) {
				delete m_worker;
				m_worker = nullptr;
			}
		}
		else {
			Sleep(1);
		}
	}
}
