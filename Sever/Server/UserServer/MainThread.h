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
#include <ThreadPool.h>

class MainThread : public Singleton<MainThread>
{
private:
    LPFN_CONNECTEX ConnectExPtr;
    LPFN_ACCEPTEX lpfnAcceptEx;
    bool m_bRunning = true;
    ServerType::en m_enType = ServerType::UserServer;

    //Global Config Setting
    std::string m_strMainSIP = "";
    int m_nMainSPort = 0;
    std::string m_strMemCachedSIP = "";
    int m_nMemCachedSPort = 0;
    int m_nUserPort = 0;

    HANDLE m_hIocp;
    std::unordered_map< NetLine::en, SOCKET> m_umListenSocket;
    CRITICAL_SECTION  m_cs;

    USERSESSION* m_pMainSSession;
    USERSESSION* m_pMemCachedSSession;
    std::list<SOCKET> m_UserList;
    std::unordered_map< std::string, USERSESSION*> m_umUserSesseion;

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
    bool StartConnectMemCachedServer();
    void CompleteConnectMainServer();

    DWORD WINAPI ThreadComplete();
    bool PostAccept(NetLine::en eLine);
    void CloseClient(USERSESSION* pSession);
    bool LoadConnectEx();
    bool LoadAcceptEx();

    USERSESSION* GetMainServer() { return this->m_pMainSSession; }
};

#define GetMainThread() MainThread::Instance()