#pragma once

#include "Worker.h"
class Thread {
public:
	Thread();
	~Thread();
	bool Start();
	bool Stop();
	bool IsValid();
	bool SetWorker(const Worker& worker);
	bool IsIdle();
private:
	static DWORD WINAPI ThreadEntry(void* arg);
	void ThreadMain();
private:
	HANDLE m_hThread;
	DWORD m_ThreadId;
	bool m_bStatus;   //停止标志false停止，true运行。
	Worker* m_worker;
};

