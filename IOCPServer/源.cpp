#include "pch.h"
#include "IOCPServer.h"

class MyServer : public IOCPServer<MyServer> {
public:
	static MyServer& getInstance() {
		static MyServer instance; // �ֲ���̬������ȷ��ֻ������һ��
		return instance;
	}
	virtual BOOL dealRecvData(const ClientContext* pContext) {
		// ԭ����ʵ��
		return DoSend(pContext);
	}
private:
	MyServer() {} // ���캯��˽�л�����ֹ�ⲿֱ�Ӵ���ʵ��
	MyServer(const MyServer&) = delete; // ɾ�����ƹ��캯��
	MyServer& operator=(const MyServer&) = delete; // ɾ����ֵ������
};


int main() {
	MyServer::getInstance(); // ��ȷ��ͨ����̬������ȡʵ��
	system("pause");
	return 0;
}