#pragma once
#include <ThreadSafeQueue.h>
#include <Types.h>
#include <HttpManager.h>

#include <LogManager.h>
#include <mutex>
#include <format>
#include <Windows.h>
#include <chrono>

#include "Mainthread.h"

class MainThread; // Forward declaration

class HttpThread
{
private:
	bool m_bRunning = false;
	void HttpRequestFunc();
	void HttpResponseFunc();
	std::mutex m_mRequet;
	std::mutex m_mResponse;
	ThreadSafeQueue<httpRequest*> requestQueue;
	ThreadSafeQueue<std::pair<httpRequest*, HttpResponse>> responseQueue;
	MainThread* m_pMainThread = nullptr;
public:
	bool IsRunning() const { return m_bRunning; }
	void StartHttpThread();
	void StopHttpThread();
	void AddRequest(httpRequest* pData);
	void SetMainThread(MainThread* pMainThread) { m_pMainThread = pMainThread; }
}