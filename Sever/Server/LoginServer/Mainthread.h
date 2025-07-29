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
	ServerType::en m_enType = ServerType::LoginServer;
	std::string m_strMainSIP = "";
	int m_nMainSPort = 0;
	int m_nUserPort = 0;
	bool m_bRunning = false;

	std::unordered_map< NetLine::en, SOCKET> m_umListenSocket;				//Line 별 Listen Socket을 모아둔 map
	SOCKET m_SockMainS;
	std::list<SOCKET> m_UserList;
	HANDLE	m_hIocp;																			//IOCP 핸들
	CRITICAL_SECTION  m_cs;																//스레드 동기화 객체
	std::vector<std::thread> m_vIocpThread;

public:
	DWORD WINAPI StartMainThread();
	bool WINAPI Release(DWORD dwType);
	bool StartLogSetting();
	bool LoadConfigSetting();
	bool StartNetSetting();
	std::string GetStrServerType();

	DWORD WINAPI LoginSAcceptLoop();

	DWORD WINAPI ThreadComplete();

	void CloseClient(USERSESSION* pSession);
};

#define GetMainThread() Mainthread::Instance()