#include <iostream>
#include <UserSocket.h>

class MainClient
{
public:
    SOCKET hSock;
};

void ErrorHandler(const char* pszMessage)
{
	printf("ERROR: %s\n", pszMessage);
	::WSACleanup();
	exit(1);
}

int main()
{
	MainClient client;
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	client.hSock = ::socket(AF_INET, SOCK_STREAM, 0);
	if (client.hSock == INVALID_SOCKET)
		ErrorHandler("소켓을 생성할 수 없습니다.");

	//포트 바인딩 및 연결
	SOCKADDR_IN	svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	if (inet_pton(AF_INET, "127.0.0.1", &svraddr.sin_addr.S_un.S_addr) != 1)
	{
		ErrorHandler("소켓을 생성할 수 없습니다.");
	}
	if (::connect(client.hSock,
		(SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
		ErrorHandler("서버에 연결할 수 없습니다.");

	::closesocket(client.hSock);
	::WSACleanup();
	return 0;
}