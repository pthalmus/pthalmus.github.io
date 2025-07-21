#pragma once

#include"Singleton.h"
#include<string>
#include<unordered_map>
#include <fstream>
#include <queue>

struct sLogData
{
	std::string strLogPath;
	std::string strLogData;
};

class LogManager : public Singleton<LogManager>
{

public:
	LogManager();
	void ErrorLog(const char*, int, const char*);
	void SystemLog(const char*, int , const char*);

private:
	std::queue<sLogData*> qLog;

	void onLoop();
};

#define GetLogManager() LogManager::Instance()