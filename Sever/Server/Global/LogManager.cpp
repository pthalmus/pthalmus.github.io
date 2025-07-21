#pragma once

#include "LogManager.h"

LogManager::LogManager()
{
	hSystemLog = INVALID_HANDLE_VALUE;
	hErrorLog = INVALID_HANDLE_VALUE;
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

	return true;
}

void LogManager::ErrorLog(const char* pstrfunc, int nRow, const char* pstrData)
{
	sLogData* LogData = new sLogData;
	LogData->strLogPath = strBaseLogPath + "\\ErrorLog\\";
	
	LogData->eLogType = LogType::en::ErrorLog;
	LogData->strLogData = std::format("[{0}:{1}] {2}\n", pstrfunc, nRow, pstrData);
	qLog.push(LogData);
}

void LogManager::SystemLog(const char* pstrfunc, int nRow, const char* pstrData)
{
	sLogData* LogData = new sLogData;
	LogData->strLogPath = strBaseLogPath + "\\SystemLog\\";

	LogData->eLogType = LogType::en::SystemLog;
	LogData->strLogData = std::format("[{0}:{1}] {2}\n", pstrfunc, nRow, pstrData);
	qLog.push(LogData);
}

DWORD WINAPI LogManager::onLoop()
{
	while (!qLog.empty())
	{
		sLogData* pLogData = qLog.front();
		qLog.pop();

		printLog(pLogData);

		delete pLogData;
	}

	return 0;
}

void LogManager::printLog(const sLogData* pData)
{
	DWORD dwRead;
	switch (pData->eLogType)
	{
	case LogType::SystemLog:
		WriteFile(hSystemLog, pData->strLogData.c_str(), strlen(pData->strLogData.c_str())*sizeof(char), &dwRead, nullptr);
		break;
	case LogType::ErrorLog:
		WriteFile(hErrorLog, pData->strLogData.c_str(), strlen(pData->strLogData.c_str()) * sizeof(char), &dwRead, nullptr);
		break;
	default:
		break;
	}
}
