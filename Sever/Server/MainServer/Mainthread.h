#pragma once


#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <unordered_map>
#include <format>
#include <windows.h>
#include <string>

#include <Types.h>
#include <Singleton.h>
#include <LogManager.h>
#include <CreatDirectorys.h>
#include <UserSocket.h>

class Mainthread : public Singleton<Mainthread>
{
	//Global Config Setting
	ServerType::en m_enType = ServerType::MainServer;
	std::string m_strDBID = "";
	std::string m_strDBPW = "";
	std::string m_strDBIP = "";
	int m_nDBPort = 0;
	int m_nLoginPort = 0;
	int m_nUserPort = 0;
	int m_nChatPort = 0;
	int m_nMemCachedPort = 0;

	std::unordered_map< NetLine::en, SOCKET> m_umListenSocket;				//Line 별 Listen Socket을 모아둔 map
	std::list<SOCKET> m_UserSList;
	std::list<SOCKET> m_ChatSList;
	std::list<SOCKET> m_LoginSList;
	std::list<SOCKET> m_MemCachedSList;
	HANDLE	m_hIocp;																			//IOCP 핸들
	CRITICAL_SECTION  m_cs;																//스레드 동기화 객체

public:
	DWORD WINAPI StartMainThread();
	bool Release(DWORD dwType);
	bool StartLogSetting();
	bool LoadConfigSetting();
	bool StartNetSetting();
	std::string GetStrServerType();

	DWORD WINAPI LoginSAcceptLoop();
	DWORD WINAPI UserSAcceptLoop();
	DWORD WINAPI ChatSAcceptLoop();
	DWORD WINAPI MemCachedSAcceptLoop();

	DWORD WINAPI ThreadComplete();

	void CloseClient(USERSESSION* pSession);
};

#define GetMainThread() Mainthread::Instance()