#include <iostream>
#include <UserSocket.h>
#include <LogManager.h>
#include <Protocol/NetMsg.h>
#include "MainClient.h"


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

	bResult = GetPrivateProfileStringA("Client", "VERSION", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		pClient->m_strVersion = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load ClientVersion(IP)");
		return false;
	}

	pClient->m_nLoginSPort = GetPrivateProfileIntA("LoginServer", "PORT", 10445, strFilePath.c_str());

	return true;
}

DWORD WINAPI ThreadFunc(MainClient client)
{
	std::string strStream;
	auto* pMsg = CREATE_PACKET(NetLogin::request_login_fromUser, NetLine::NetLine_LoginS_User, NetLogin::eRequest_Login_FromUser);
	std::cout << "Enter User ID: ";
	std::cin >> pMsg->szUserID;
	std::cout << "Enter Password: ";
	std::cin >> pMsg->szPassword;

	strcpy_s(pMsg->szClientVersion, client.m_strVersion.c_str());

	GetPacketDispatcher().DispatchSend(client.m_pSession, (char*)pMsg, pMsg->GetSize());
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
	client.LoadConnectEx();
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
	if(client.StartNetwork() == false)
	{
		ErrorHandler("Can not Start Network");
		return 0;
	}
	if(client.ConnectToLoginServer() == false)
	{
		ErrorHandler("Can not Connect To LoginServer");
		return 0;
	}

	std::thread t(ThreadFunc, client);
	t.join();

	::closesocket(client.m_pSession->hSocket);
	::WSACleanup();
	return 0;
}