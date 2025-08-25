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

#include "DBThread.h"

class Mainthread : public Singleton<Mainthread>
{
	//Global Config Setting
	ServerType::en m_enType = ServerType::MemCachedServer;
	std::string m_strMainSIP = "";
	int m_nMainSPort = 0;
	int m_nUserSPort = 0;
	std::string m_strDBID = "";													//MainServer 로부터 전달받음
	std::string m_strDBPW = "";													//MainServer 로부터 전달받음
	std::string m_strServer = "";													//MainServer 로부터 전달받음
	USERSESSION* m_pMainSSession;

	bool m_bRunning = false;
	SOCKET m_hListenSocket;													// Listen Socket
	HANDLE	m_hIocp;																//IOCP 핸들
	CRITICAL_SECTION  m_cs;													//스레드 동기화 객체
	std::vector<std::thread> m_vIocpThread;
	std::list<SOCKET> m_UserSList;												// UserS List

	DBThread m_DBThread;														// DB Thread 관리
	std::thread m_hDBThread;													// DB Thread
public:
	DWORD WINAPI StartMainThread();
	bool WINAPI Release(DWORD dwType);
	bool StartLogSetting();
	bool LoadConfigSetting();
	bool StartNetSetting();
	bool StartDBConnection();
	bool StartConnectMainServer();
	bool StartDBThread();
	std::string GetStrServerType();

	void CompleteConnectMainServer();
	DWORD WINAPI HeartBeatLoop();
	USERSESSION* GetMainServer();
	bool IsRunning() const { return m_bRunning; }

	DWORD WINAPI UserSAcceptLoop();
	DWORD WINAPI ThreadComplete();
	void CloseClient(USERSESSION* pSession);

	void SetDBInfo(const std::string& dbID, const std::string& dbPW, const std::string& server)
	{
		m_strDBID = dbID;
		m_strDBPW = dbPW;
		m_strServer = server;
	}

	bool AddDBRequest(SQLDATA* pData);
};

#define GetMainThread() Mainthread::Instance()
