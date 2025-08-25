#pragma once

#include <UserSocket.h>
#include <Protocol/NetMsg.h>
#include <unordered_map>
#include <format>
#include <windows.h>
#include <string>
#include <chrono>


#include <Types.h>
#include <Singleton.h>
#include <LogManager.h>
#include <CreatDirectorys.h>
#include <DataBaseManager.h>
#include <ThreadSafeQueue.h>

class Mainthread : public Singleton<Mainthread>
{
	//Global Config Setting
	ServerType::en m_enType = ServerType::MainServer;
	std::string m_strDBID = "";
	std::string m_strDBPW = "";
	std::string m_strServer = "";
	int m_nLoginPort = 0;
	int m_nUserPort = 0;
	int m_nChatPort = 0;
	int m_nMemCachedPort = 0;

	bool m_bRunning = false;

	std::unordered_map< NetLine::en, SOCKET> m_umListenSocket;				//Line 별 Listen Socket을 모아둔 map
	std::list<SOCKET> m_UserSList;
	std::list<SOCKET> m_ChatSList;
	std::list<SOCKET> m_LoginSList;
	std::list<SOCKET> m_MemCachedSList;
	HANDLE	m_hIocp;																			//IOCP 핸들
	CRITICAL_SECTION  m_cs;																//스레드 동기화 객체
	std::vector<std::thread> m_vIocpThread;

	std::thread m_hDBThread;

public:

	DWORD WINAPI StartMainThread();
	bool WINAPI Release(DWORD dwType);
	bool StartLogSetting();
	bool LoadConfigSetting();
	bool StartNetSetting();
	bool StartDBConnection();
	std::string GetStrServerType();

	DWORD WINAPI LoginSAcceptLoop();
	DWORD WINAPI UserSAcceptLoop();
	DWORD WINAPI ChatSAcceptLoop();
	DWORD WINAPI MemCachedSAcceptLoop();

	DWORD WINAPI ThreadComplete();

	void CloseClient(USERSESSION* pSession);

	void GetDBInfo(char* strDBID, size_t nDBIDSize,
		char* strDBPW, size_t nDBPWSize,
		char* strServer, size_t nServerSize)
	{
		strcpy_s(strDBID, nDBIDSize, m_strDBID.c_str());
		strcpy_s(strDBPW, nDBPWSize, m_strDBPW.c_str());
		strcpy_s(strServer, nServerSize, m_strServer.c_str());
	}
};

#define GetMainThread() Mainthread::Instance()