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
#include <ThreadSafeQueue.h>

class MainThread : public Singleton<MainThread>
{
	//Global Config Setting
	ServerType::en m_enType = ServerType::UserServer;
	std::string m_strMainSIP = "";
	int m_nMainSPort = 0;
	std::string m_strMemCachedSIP = "";
	int m_nMemCachedSPort = 0;
	int m_nUserPort = 0;
	bool m_bRunning = false;
	CRITICAL_SECTION  m_cs;																//스레드 동기화 객체

	USERSESSION* m_pMainSSession;
	USERSESSION* m_pMemCachedSSession;
	HANDLE	m_hIocp;																			//IOCP 핸들
	std::vector<std::thread> m_vIocpThread;
	std::unordered_map< NetLine::en, SOCKET> m_umListenSocket;				//Line 별 Listen Socket을 모아둔 map
	std::list<SOCKET> m_UserList;


public:
	DWORD WINAPI StartMainThread();
	bool WINAPI Release(DWORD dwType);
	bool StartLogSetting();
	bool LoadConfigSetting();
	bool StartNetSetting();
	bool StartConnectMainServer();
	bool StartConnectMemCachedServer();
	std::string GetStrServerType();

	void CompleteConnectMainServer();
	DWORD WINAPI HeartBeatLoop();
	USERSESSION* GetMainServer();
	bool IsRunning() const { return m_bRunning; }

	DWORD WINAPI ThreadComplete();
	void CloseClient(USERSESSION* pSession);

	DWORD WINAPI UserAcceptLoop();
};

#define GetMainThread() MainThread::Instance()