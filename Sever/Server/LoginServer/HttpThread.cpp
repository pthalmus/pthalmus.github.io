#include "HttpThread.h"

void HttpThread::StartHttpThread()
{
	m_bRunning = true;
	SetMainThread(&GetMainThread());
}
void HttpThread::StopHttpThread()
{
	if (m_bRunning)
	{
		m_bRunning = false;
	}
}
void HttpThread::AddRequest(httpRequest* pData)
{
	if (pData == nullptr)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(m_mRequet);
	requestQueue.push(pData);
}
void HttpThread::HttpRequestFunc()
{
	while (m_bRunning)
	{
		httpRequest* pData = nullptr;
		m_mRequet.lock();
		if (requestQueue.empty() == false)
		{
			pData = requestQueue.pop();
			HttpResponse response = GetHttpManager().Run(pData);
			m_mResponse.lock();
			responseQueue.push(std::make_pair(pData, response));
			m_mResponse.unlock();
		}
		m_mRequet.unlock();
		if (pData == nullptr)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Prevent busy waiting
		}
	}
}
void HttpThread::HttpResponseFunc()
{
	while (m_bRunning)
	{
		std::pair<httpRequest*, HttpResponse> responsePair;
		m_mResponse.lock();
		if (responseQueue.empty() == false)
		{
			responsePair = responseQueue.pop();
			if (m_pMainThread)
			{
				// Here you would typically notify the main thread about the response
				// For example, you might have a method in Mainthread to handle responses
				// m_pMainThread->HandleHttpResponse(responsePair.first, responsePair.second);
			}
			delete responsePair.first; // Clean up the request after processing
		}
		m_mResponse.unlock();
		if (responsePair.first == nullptr)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Prevent busy waiting
		}
	}
}