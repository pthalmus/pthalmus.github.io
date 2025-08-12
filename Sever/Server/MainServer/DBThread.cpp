#include "DBThread.h"

DWORD WINAPI DBThread::StartDBThread()
{
	m_bRunning = true;
	m_hRequestThread = std::thread(&DBThread::DBRequestFunc, this);
	m_hResponseThread = std::thread(&DBThread::DBResponseFunc, this);

	m_hRequestThread.detach();
	m_hResponseThread.detach();

	return 0;
}

void DBThread::StopDBThread()
{
	if (m_bRunning)
	{
		m_bRunning = false;
		m_hRequestThread.join();
		m_hResponseThread.join();
	}
}

void DBThread::AddRequest(SQLDATA* pData)
{
	if (pData == nullptr)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(m_mRequet);
	requestQueue.push(pData);
}

DWORD WINAPI DBThread::DBRequestFunc()
{
	while (m_bRunning)
	{
		SQLDATA* pData = nullptr;
		m_mRequet.lock();
		if (requestQueue.empty() == false)
		{
			pData = requestQueue.pop();

			// Process the SQLDATA request
			switch (pData->eType)
			{
			case SQLTYPE::SQL_MEMBER:
				GetDBManager().Excute(STRDSN_MEMBER_W, pData->strSql); // Example SQL command
				break;
			case SQLTYPE::SQL_USER:
				break;
			default:
				GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Unknown SQLTYPE: %d", pData->eType);
				break;
			}
		}
		m_mRequet.unlock();
	}
	return 0;
}

DWORD WINAPI DBThread::DBResponseFunc()
{
	while (m_bRunning)
	{
		SQLDATA* pData = nullptr;
		m_mResponse.lock();
		if (responseQueue.empty() == false)
		{
			pData = responseQueue.pop();
			pData->Done(); // Call Done method if implemented
			delete pData; // Clean up after processing
		}
		m_mResponse.unlock();
		Sleep(1); // Prevent busy waiting
	}
	return 0;
}
