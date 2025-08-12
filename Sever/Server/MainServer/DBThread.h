#pragma once
#include <ThreadSafeQueue.h>
#include <Types.h>
#include <DataBaseManager.h>

#include <thread>
#include <LogManager.h>
#include <mutex>
#include <format>
#include <Windows.h>

#include "Mainthread.h"

class DBThread
{
public:
    bool IsRunning() const { return m_bRunning; }
    DWORD WINAPI StartDBThread();
    void StopDBThread();
    void AddRequest(SQLDATA* pData);

private:
    bool m_bRunning = false;
    std::thread m_hRequestThread;
    std::thread m_hResponseThread;
    DWORD WINAPI DBRequestFunc();
    DWORD WINAPI DBResponseFunc();

    std::mutex m_mRequet;
    std::mutex m_mResponse;

    ThreadSafeQueue<SQLDATA*> requestQueue;
    ThreadSafeQueue<SQLDATA*> responseQueue;
};