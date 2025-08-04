#pragma once
#include<magic_enum/magic_enum.hpp>

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
}

namespace LogType
{
	enum en
	{
		SystemLog,
		ErrorLog,
		eMAX
	};
}

namespace NetLine {
	enum en
	{
		NetLine_Main_LoginS,
		NetLine_Main_UserS,
		NetLine_Main_ChatS,
		NetLine_Main_MemCachedS,
		NetLine_UserS_User,
		NetLine_UserS_MemCachedS,
		NetLine_ChatS_User,
		NetLine_LoginS_User,
		NetLine_MemCachedS,
		eMAX
	};
}

#define MAX_THREAD_CNT 6