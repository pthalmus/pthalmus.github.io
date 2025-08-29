#pragma once
#include <UserSocket.h>
#include <Singleton.h>
#include <LogManager.h>
#include <ThreadPool.h>

#include <thread>
#include <format>
#include <unordered_map>
#include <windows.h>
#include <string>
#include <chrono>
#include <Types.h>

#include "LocalThread.h"

class MainThread : public Singleton<MainThread>
{
private:
    LPFN_CONNECTEX ConnectExPtr;
    LPFN_ACCEPTEX lpfnAcceptEx;
    bool m_bRunning = true;
    ServerType::en m_enType = ServerType::GameServer;

    //Config data;
    std::string m_strMainSIP;
    int m_nMainSPort;
    int m_nUserPort;

    HANDLE m_hIocp;
    std::unordered_map< NetLine::en, SOCKET> m_umListenSocket;
    CRITICAL_SECTION  m_cs;

    USERSESSION* m_pMainSSession;
    std::list<SOCKET> m_UserList;
    std::unordered_map<std::string, USERSESSION*> m_umUserSesseion;
public:
    void StartMainThread();
    bool Release(DWORD dwType);
    bool IsRunning() const { return m_bRunning; }
    std::string GetStrServerType() { return std::string(magic_enum::enum_name(m_enType)); }

    bool StartLogSetting();
    bool LoadConfigSetting();
    bool StartNetSetting();
    void StartHeartBeatLoop();

    bool StartConnectMainServer();

    DWORD WINAPI ThreadComplete();
    bool PostAccept(NetLine::en eLine);
    void CloseClient(USERSESSION* pSession);
    bool LoadConnectEx();
    bool LoadAcceptEx();

    USERSESSION* GetMainServer() { return this->m_pMainSSession; };
};

#define GetMainThread() MainThread::Instance()