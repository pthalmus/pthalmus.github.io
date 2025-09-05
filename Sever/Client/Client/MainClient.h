#pragma once
#include <string>
#include <thread>
#include <vector>
#include <UserSocket.h>
#include <LogManager.h>
#include <ThreadPool.h>

class MainClient
{
public:
	bool						m_bRun;
	std::string				m_strLoginSIP;
	std::string				m_strVersion;
	int							m_nLoginSPort;
	USERSESSION*			m_pSession;
	HANDLE					hCompletionPort;
	LPFN_CONNECTEX	ConnectExPtr;

	MainClient() : m_bRun(true), m_nLoginSPort(0), m_pSession(nullptr), hCompletionPort(NULL) {}

	~MainClient()
	{
		m_bRun = false;
		if (m_pSession)
		{
			delete m_pSession;
			m_pSession = nullptr;
		}
		if (hCompletionPort)
		{
			::CloseHandle(hCompletionPort);
			hCompletionPort = NULL;
		}
	}

	bool ConnectToLoginServer();
	bool StartNetwork();
	DWORD WINAPI ThreadComplete();
	bool LoadConnectEx();
};