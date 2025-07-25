#pragma once

#include"Singleton.h"
#include<string>
#include<unordered_map>
#include <fstream>
#include <queue>
#include <format>
#include <mutex>

#include <CreatDirectorys.h>
#include <Types.h>

struct sLogData
{
	LogType::en eLogType = LogType::eMAX;
	std::string strLogPath;
	std::string strLogData;
};

class LogManager : public Singleton<LogManager>
{
	std::mutex m_mutex;
	std::string strBaseLogPath;

	HANDLE hSystemLog;
	HANDLE hErrorLog;

	bool		m_bRunning;
public:
	LogManager();
	bool init(std::string pstrLogPath);
	void Release();
	void ErrorLog(const char* pstrfunc, int nRow, const char* pstrData);
	void SystemLog(const char* pstrfunc, int nRow, const char* pstrData);
	DWORD WINAPI onLoop();
private:
	std::queue<sLogData*> qLog;
	void printLog(const sLogData* pData);
};

#define GetLogManager() LogManager::Instance()