#include "pch.h"

ThreadPool::ThreadPool(int threadCount) {
	for (int i = 0; i < threadCount; i++) {
		m_threads.push_back(new Thread());
	}
}

ThreadPool::~ThreadPool() {
	for (auto& thread : m_threads) {
		thread->Stop();
		delete thread;
	}
}

bool ThreadPool::resize(int threadCount) {
	StopAllThread();
	m_threads.clear();
	for (int i = 0; i < threadCount; i++) {
		m_threads.push_back(new Thread());
	}
	return true;
}

bool ThreadPool::DispatchWorker(const Worker& worker) {
	m_lock.lock();
	int index = -1;
	for (size_t i = 0; i < m_threads.size(); i++) {
		if (m_threads[i]->IsIdle()) {
			m_threads[i]->SetWorker(worker);
			index = i;
			break;
		}
	}
	m_lock.unlock();
	if (index == -1) {
		return false;
	}
	else {
		return true;
	}
}

void ThreadPool::StartAllThread() {
	for (auto& thread : m_threads) {
		thread->Start();
	}
}

void ThreadPool::StopAllThread() {
	for (auto& thread : m_threads) {
		thread->Stop();
	}
}

bool ThreadPool::CheckThreadValid(size_t index) {
	if (index < m_threads.size()) {
		return m_threads[index]->IsValid();
	}
	else {
		return false;
	}
}
