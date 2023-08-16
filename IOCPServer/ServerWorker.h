#pragma once
#include "Worker.h"

template<typename ClassName>
class serverThiscallWorker : public CloneableWorker<serverThiscallWorker<ClassName>> {
	using fnthiscallWorker = BOOL(ClassName::*)(const ClientContext*);
public:
	serverThiscallWorker(ClassName* This, fnthiscallWorker fn, LPCVOID pContext, bool isOne = true)
		:Worker(isOne), CloneableWorker<serverThiscallWorker<ClassName>>(isOne),\
		Context(*(ClientContext*)pContext), thiscallfunc(fn), thiz(This) {}
	virtual void operator()() const override {
		(thiz->*thiscallfunc)(&Context);
	}
	ClientContext Context;
	fnthiscallWorker thiscallfunc;
	ClassName* thiz;
};