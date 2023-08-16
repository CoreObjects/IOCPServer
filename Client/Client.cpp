#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma warning(disable:4996)
#pragma comment(lib, "Ws2_32.lib")

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 9527
#define BUFFER_SIZE 1024*4

int main() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		printf("WSAStartup失败，错误码：%d\n", WSAGetLastError());
		return 1;
	}

	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		printf("创建套接字失败，错误码：%d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	SOCKADDR_IN serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDRESS, &serverAddress.sin_addr);

	if (connect(clientSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) != 0) {
		printf("连接服务器失败，错误码：%d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}
/*	system("pause");*/
	char sendBuffer[] = "Hello, Server!";
	if (send(clientSocket, sendBuffer, strlen(sendBuffer), 0) == SOCKET_ERROR) {
		printf("发送消息失败，错误码：%d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	char recvBuffer[BUFFER_SIZE];
	int bytesReceived = recv(clientSocket, recvBuffer, BUFFER_SIZE, 0);
	if (bytesReceived == SOCKET_ERROR) {
		printf("接收消息失败，错误码：%d\n", WSAGetLastError());
	}
	else if (bytesReceived > 0) {
		printf("收到来自服务器的响应：%s\n", recvBuffer);
	}

	system("pause");
	closesocket(clientSocket);
	WSACleanup();

	return 0;
}