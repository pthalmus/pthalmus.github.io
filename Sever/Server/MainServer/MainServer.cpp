#pragma once

#include<iostream>
#include<Windows.h>
#include<string>
#include<chrono>
#include <thread>

#include <LogManager.h>
#include <CreatDirectorys.h>
#include <Types.h>

//Global Config Setting
std::string g_strDBID = "";
std::string g_strDBPW = "";
std::string g_strDBIP = "";
std::string g_strDBPort = "";
std::string g_strLoginPort = "";
std::string g_strUserPort = "";
std::string g_strChatPort = "";
std::string g_strMemCachedPort = "";

ServerType::en enType = ServerType::MainServer;
static std::string GetStrServerType()
{
	return std::string(magic_enum::enum_name(enType));
}

static bool LoadConfigSetting()
{
	std::string strFilePath = "./Config/MainServerConfig.ini";
	bool bResult = false;
	char strTemp[256] = { 0, };
	int nTemp = 0;

	//DB Connection Config
	bResult = GetPrivateProfileStringA("DB", "ID", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		g_strDBID = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load DB Connection Config(ID)");
		return false;
	}

	bResult = GetPrivateProfileStringA("DB", "PW", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		g_strDBPW = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load DB Connection Config(PW)");
		return false;
	}

	bResult = GetPrivateProfileStringA("DB", "IP", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		g_strDBIP = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load DB Connection Config(IP)");
		return false;
	}

	bResult = GetPrivateProfileStringA("DB", "PORT", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		g_strDBPort = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load DB Connection Config(PORT)");
		return false;
	}

	//Login Server Connection Config
	bResult = GetPrivateProfileStringA("LoginServer", "PORT", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		g_strLoginPort = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load Login Server Connection Config(PORT)");
		return false;
	}

	//User Server Connection Config
	bResult = GetPrivateProfileStringA("UserServer", "PORT", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		g_strUserPort = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load User Server Connection Config(PORT)");
		return false;
	}

	//Chat Server Connection Config
	bResult = GetPrivateProfileStringA("ChatServer", "PORT", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		g_strChatPort = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load Chat Server Connection Config(PORT)");
		return false;
	}

	//MemCached Server Connection Config
	bResult = GetPrivateProfileStringA("MemCachedServer", "PORT", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		g_strMemCachedPort = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load MemCached Server Connection Config(PORT)");
		return false;
	}

	return true;
}

std::string time_point_to_string(const std::chrono::system_clock::time_point& time_point, const std::string& format) {
	std::time_t time = std::chrono::system_clock::to_time_t(time_point);
	errno_t err_code;
	std::tm timeinfo;
	err_code = localtime_s(&timeinfo, &time);
	std::ostringstream oss;
	oss << std::put_time(&timeinfo, format.c_str());
	return oss.str();
}

bool StartLogSetting()
{
	std::string strFilePath = std::format("Log\\{0}\\", GetStrServerType());
	if (CreateNestedDirectoryA(strFilePath)) {
		std::cout << "폴더 생성 성공\n";
	}
	else {
		std::cout << "폴더 생성 실패\n";
		return false;
	}

	if (GetLogManager().init(strFilePath) == false)
	{
		return false;
	}
	return true;
}

int main()
{
	if (StartLogSetting() == false)
	{
		return 0;
	}

	if (LoadConfigSetting() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed Load Config!!");
	}

	GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Testing ErrorLog2");
	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Testing SystemLog2");

	std::thread t(LogManager::onLoop, GetLogManager());
	return 0;
}