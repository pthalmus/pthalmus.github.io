#include "pch.h"

LogManager::LogManager()
{
	hSystemLog = INVALID_HANDLE_VALUE;
	hErrorLog = INVALID_HANDLE_VALUE;
	m_bRunning = false;
}

bool LogManager::init(std::string pstrLogPath)
{
	this->strBaseLogPath = pstrLogPath;

	if (CreateNestedDirectoryA(strBaseLogPath + "SystemLog\\") == false)
	{
		return false;
	}

	if (CreateNestedDirectoryA(strBaseLogPath + "ErrorLog\\") == false)
	{
		return false;
	}
	std::string strSystemLogPath = strBaseLogPath + "SystemLog\\SystemLog.txt";
	hSystemLog = CreateFileA(strSystemLogPath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	std::string strErrorLogPath = strBaseLogPath + "ErrorLog\\ErrorLog.txt";
	hErrorLog = CreateFileA(strErrorLogPath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hSystemLog == INVALID_HANDLE_VALUE || hErrorLog == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	SetFilePointer(hSystemLog, 0, nullptr, FILE_END);
	SetFilePointer(hErrorLog, 0, nullptr, FILE_END);

	m_bRunning = true;

	std::thread tLog(&LogManager::onLoop, this);
	tLog.detach();

	return true;
}

void LogManager::Release()
{
	std::lock_guard<std::mutex> lg(m_mutex);
	CloseHandle(hSystemLog);
	CloseHandle(hErrorLog);

	m_bRunning = false;
}

void LogManager::ErrorLog(const char* pstrfunc, int nRow, const char* pstrData, ...)
{
	char msgBuf[1024];

	va_list args;
	va_start(args, pstrData);
	vsnprintf(msgBuf, sizeof(msgBuf), pstrData, args);
	va_end(args);

	sLogData* LogData = new sLogData;
	LogData->strLogPath = strBaseLogPath + "\\ErrorLog\\";
	LogData->eLogType = LogType::en::ErrorLog;

	// msgBuf를 로그 메시지로 사용
	LogData->strLogData = std::format("[{}:{}] {}\n", pstrfunc, nRow, msgBuf);

	std::lock_guard<std::mutex> lg(m_mutex);
	qLog.push(LogData);
}

void LogManager::SystemLog(const char* pstrfunc, int nRow, const char* pstrData, ...)
{
	char msgBuf[1024];

	va_list args;
	va_start(args, pstrData);
	vsnprintf(msgBuf, sizeof(msgBuf), pstrData, args);
	va_end(args);

	sLogData* LogData = new sLogData;
	LogData->strLogPath = strBaseLogPath + "\\SystemLog\\";
	LogData->eLogType = LogType::en::SystemLog;

	// msgBuf를 로그 메시지로 사용
	LogData->strLogData = std::format("[{}:{}] {}\n", pstrfunc, nRow, msgBuf);

	std::lock_guard<std::mutex> lg(m_mutex);
	qLog.push(LogData);
}

DWORD WINAPI LogManager::onLoop()
{
	while (m_bRunning == true)
	{
		m_mutex.lock();
		if (qLog.empty() == false)
		{
			sLogData* pLogData = qLog.front();
			qLog.pop();

			printLog(pLogData);

			delete pLogData;
		}
		m_mutex.unlock();
	}

	return 0;
}

void LogManager::printLog(const sLogData* pData)
{
	DWORD dwRead;
	switch (pData->eLogType)
	{
	case LogType::SystemLog:
		WriteFile(hSystemLog, pData->strLogData.c_str(), strlen(pData->strLogData.c_str()) * sizeof(char), &dwRead, nullptr);
		break;
	case LogType::ErrorLog:
		WriteFile(hErrorLog, pData->strLogData.c_str(), strlen(pData->strLogData.c_str()) * sizeof(char), &dwRead, nullptr);
		break;
	default:
		break;
	}
}
