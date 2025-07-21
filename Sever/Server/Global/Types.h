#pragma once
#include<string>
#include<unordered_map>

namespace ServerType
{
	enum en
	{
		MainServer,
		UserServer,
		ChatServer,
		LoginServer,
		MemCachedServer,
		eMAX
	};

	std::unordered_map<en, std::string> map;
	void insertMap(en eNum, std::string str)
	{
		map.insert({ eNum, str });
	}

	struct sInitalizer
	{
		sInitalizer();
	};

	sInitalizer::sInitalizer()
	{
		insertMap(MainServer, "MainServer");
		insertMap(UserServer, "UserServer");
		insertMap(ChatServer, "ChatServer");
		insertMap(LoginServer, "LoginServer");
		insertMap(MemCachedServer, "MemCachedServer");
		insertMap(eMAX, "Error");
	}

	static sInitalizer initializer;

	const char* GetStrByEnum(en eNum = eMAX)
	{
		return map[eNum].c_str();
	}
};