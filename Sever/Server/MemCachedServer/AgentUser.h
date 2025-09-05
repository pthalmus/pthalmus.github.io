#pragma once
#include <Singleton.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>

typedef struct _UserInfo
{
	std::string userID;
	std::string sessionToken;
	
	std::chrono::system_clock::time_point lastActiveTime;

	//TableData

} UserInfo;

class AgentUser : public Singleton<AgentUser>
{
private:
	std::unordered_map<std::string, UserInfo> userMap; // Key: userID
	std::mutex mapMutex;

public:
}

#define GetAgentUser() AgentUser::Instance()