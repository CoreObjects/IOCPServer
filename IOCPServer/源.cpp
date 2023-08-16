#include "pch.h"
#include "IOCPServer.h"

class MyServer : public IOCPServer<MyServer> {
public:
	static MyServer& getInstance() {
		static MyServer instance; // 局部静态变量，确保只被创建一次
		return instance;
	}
	virtual BOOL dealRecvData(const ClientContext* pContext) {
		// 原来的实现
		return DoSend(pContext);
	}
private:
	MyServer() {} // 构造函数私有化，防止外部直接创建实例
	MyServer(const MyServer&) = delete; // 删除复制构造函数
	MyServer& operator=(const MyServer&) = delete; // 删除赋值操作符
};


int main() {
	MyServer::getInstance(); // 正确，通过静态方法获取实例
	system("pause");
	return 0;
}