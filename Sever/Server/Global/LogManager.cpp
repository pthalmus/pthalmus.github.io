#pragma once

#include "LogManager.h"

LogManager::LogManager()
{
	
}

void LogManager::ErrorLog(const char*, int, const char*)
{
}

void LogManager::SystemLog(const char*, int , const char*)
{
}

void LogManager::onLoop()
{
	while (!qLog.empty())
	{
		sLogData* pLogData = qLog.front();
		qLog.pop();


		delete pLogData;
	}
}
