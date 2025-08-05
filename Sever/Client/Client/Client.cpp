#include <iostream>
#include <UserSocket.h>
#include <LogManager.h>

class MainClient
{
public:
    SOCKET		hSock;
	std::string	m_strLoginSIP;
	int				m_nLoginSPort;
};

bool StartLogSetting()
{
	std::string strFilePath = std::format("Log\\{0}\\", "Client");
	if (CreateNestedDirectoryA(strFilePath) == false)
	{
		return false;
	}

	if (GetLogManager().init(strFilePath) == false)
	{
		return false;
	}
	return true;
}

void ErrorHandler(const char* pszMessage)
{
	GetLogManager().ErrorLog(__FUNCTION__, __LINE__, pszMessage);
	::WSACleanup();
	GetLogManager().Release();
}

bool LoadConfigSetting(MainClient* pClient)
{
	std::string strFilePath = "./Config/ClientConfig.ini";
	bool bResult = false;
	char strTemp[256] = { 0, };
	int nTemp = 0;

	//LoginServer Connection Config
	bResult = GetPrivateProfileStringA("LoginServer", "IP", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		pClient->m_strLoginSIP = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load MainServer Connection Config(IP)");
		return false;
	}

	pClient->m_nLoginSPort = GetPrivateProfileIntA("LoginServer", "PORT", 9973, strFilePath.c_str());

	return true;
}

DWORD WINAPI ThreadFunc(MainClient client)
{
	std::string strStream;
	while (true)
	{
		std::cin >> strStream;

		if (strcmp(strStream.c_str(), "Quit"))
		{
			break;
		}
	}

	return 0;
}

int main()
{
	MainClient client;
	if (StartLogSetting() == false)
	{
		return 0;
	}
	if (LoadConfigSetting(&client) == false)
	{
		ErrorHandler("Can not LoadConfig Settings");
		return 0;
	}

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
	svraddr.sin_port = htons(client.m_nLoginSPort);
	if (inet_pton(AF_INET, client.m_strLoginSIP.c_str(), &svraddr.sin_addr.S_un.S_addr) != 1)
	{
		ErrorHandler("소켓을 생성할 수 없습니다.");
	}
	if (::connect(client.hSock,
		(SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
		ErrorHandler("서버에 연결할 수 없습니다.");

	std::thread t(ThreadFunc, client);
	t.join();

	::closesocket(client.hSock);
	::WSACleanup();
	return 0;
}