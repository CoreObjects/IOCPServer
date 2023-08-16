#pragma once
#include "Thread.h"

class ThreadPool {
public:
	ThreadPool() {};

	ThreadPool(int threadCount);

	~ThreadPool();

	bool resize(int threadCount);

	bool DispatchWorker(const Worker& worker);

	void StartAllThread();
	void StopAllThread();
	bool CheckThreadValid(size_t index);
private:
	std::vector<Thread*> m_threads;
	std::mutex m_lock;
};
