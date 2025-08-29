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
#include <ThreadPool.h>

#include "DBThread.h"

class MainThread : public Singleton<MainThread>
{
private:
	LPFN_CONNECTEX ConnectExPtr;
	LPFN_ACCEPTEX lpfnAcceptEx;
	bool m_bRunning = true;
	ServerType::en m_enType = ServerType::MemCachedServer;

	//Global Config Setting
	std::string m_strMainSIP = "";
	int m_nMainSPort = 0;
	int m_nUserSPort = 0;
	std::string m_strDBID = "";
	std::string m_strDBPW = "";
	std::string m_strServer = "";

	HANDLE m_hIocp;
	std::unordered_map< NetLine::en, SOCKET> m_umListenSocket;
	CRITICAL_SECTION  m_cs;

	USERSESSION* m_pMainSSession;
	std::list<SOCKET> m_UserSList;
	DBThread m_DBThread;
public:
	void StartMainThread();
	bool Release(DWORD dwType);
	bool IsRunning() const { return m_bRunning; }
	std::string GetStrServerType() { return std::string(magic_enum::enum_name(m_enType)); }

	bool StartLogSetting();
	bool LoadConfigSetting();
	bool StartNetSetting();
	void StartHeartBeatLoop();

	bool StartDBConnection();
	bool StartConnectMainServer();
	bool StartDBThread();

	DWORD WINAPI ThreadComplete();
	bool PostAccept(NetLine::en eLine);
	void CloseClient(USERSESSION* pSession);
	bool LoadConnectEx();
	bool LoadAcceptEx();

	USERSESSION* GetMainServer() { return m_pMainSSession; }
	void SetDBInfo(const std::string& dbID, const std::string& dbPW, const std::string& server)
	{
		m_strDBID = dbID;
		m_strDBPW = dbPW;
		m_strServer = server;
	}

	bool AddDBRequest(SQLDATA* pData);
	void CompleteConnectMainServer();
	void RequestDBConnectionData();
};

#define GetMainThread() MainThread::Instance()
