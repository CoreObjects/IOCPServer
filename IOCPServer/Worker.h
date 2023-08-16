#pragma once
#include "Base.h"
class Worker {
public:
	Worker(bool isOne = true) :bIsOneTimes(isOne) {};

	bool IsOne() {return bIsOneTimes;}

	virtual ~Worker() {}
	// ����()��������ṩһ���ӿڹ�����ʵ��
	virtual void operator()() const = 0;

	virtual Worker* Clone() const = 0;
private:
	bool bIsOneTimes;//�ù����Ƿ�ִֻ��һ�Σ�ture:ִֻ��һ�Σ�false����һֱִ��
};

template <typename Derived>
class CloneableWorker :virtual public Worker {
public:
	CloneableWorker(bool isOne = true) : Worker(isOne) {}

	virtual Worker* Clone() const override {
		return new Derived(static_cast<const Derived&>(*this));
	}
	virtual void operator()() const = 0;
	// ������Ա
	virtual ~CloneableWorker() {}
};

using fncdeclWorker = void(__cdecl*)(void*);
class cdeclWorker : public CloneableWorker<cdeclWorker> {
public:
	cdeclWorker(fncdeclWorker fn, LPVOID arg, bool isOne = true)
		: CloneableWorker<cdeclWorker>(isOne), cdeclfunc(fn), argWorker(arg) {
	}

	virtual void operator()() const override {
		cdeclfunc(argWorker);
	}
	virtual ~cdeclWorker() {};
	LPVOID argWorker;
	fncdeclWorker cdeclfunc;
};

using fnstdcallWorker = DWORD(__stdcall*)(void*);
class stdcallWorker : public CloneableWorker<stdcallWorker> {
public:
	stdcallWorker(fnstdcallWorker fn, LPVOID arg, bool isOne = true)
		:Worker(isOne), CloneableWorker<stdcallWorker>(isOne), stdcallfunc(fn) , argWorker(arg) {}
	virtual void operator()() const override {
		stdcallfunc(argWorker);
	}
	LPVOID argWorker;
	fnstdcallWorker stdcallfunc;
};