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

class MainThread : public Singleton<MainThread>
{
private:
	LPFN_CONNECTEX ConnectExPtr;
	LPFN_ACCEPTEX lpfnAcceptEx;
	bool m_bRunning = true;

	//Global Config Setting
	ServerType::en m_enType = ServerType::MainServer;
	std::string m_strDBID = "";
	std::string m_strDBPW = "";
	std::string m_strServer = "";
	int m_nLoginPort = 0;
	int m_nUserPort = 0;
	int m_nChatPort = 0;
	int m_nMemCachedPort = 0;

	HANDLE m_hIocp;
	std::unordered_map< NetLine::en, SOCKET> m_umListenSocket;
	CRITICAL_SECTION  m_cs;

	std::list<USERSESSION*> m_UserSList;
	std::list<USERSESSION*> m_ChatSList;
	std::list<USERSESSION*> m_LoginSList;
	std::list<USERSESSION*> m_MemCachedSList;
	std::list<USERSESSION*> m_GameSSList;

public:
	void StartMainThread();
	bool Release(DWORD dwType);
	bool IsRunning() const { return m_bRunning; }
	std::string GetStrServerType() { return std::string(magic_enum::enum_name(m_enType)); }

	bool StartLogSetting();
	bool LoadConfigSetting();
	bool StartNetSetting();
	void StartHeartBeatLoop();

	DWORD WINAPI ThreadComplete();
	bool PostAccept(NetLine::en eLine);
	void CloseClient(USERSESSION* pSession);
	bool LoadConnectEx();
	bool LoadAcceptEx();

	bool StartDBConnection();
	void GetDBInfo(char* strDBID, size_t nDBIDSize,
		char* strDBPW, size_t nDBPWSize,
		char* strServer, size_t nServerSize)
	{
		strcpy_s(strDBID, nDBIDSize, m_strDBID.c_str());
		strcpy_s(strDBPW, nDBPWSize, m_strDBPW.c_str());
		strcpy_s(strServer, nServerSize, m_strServer.c_str());
	}

	void AddServerList(USERSESSION* pSession, NetLine::en eLine);
	USERSESSION* GetMemCachedServer()
	{
		if (m_MemCachedSList.empty())
		{
			return nullptr;
		}
		return m_MemCachedSList.front();
	}
	USERSESSION* GetLoginServer()
	{
		if (m_LoginSList.empty())
		{
			return nullptr;
		}
		return m_LoginSList.front();
	}
};

#define GetMainThread() MainThread::Instance()