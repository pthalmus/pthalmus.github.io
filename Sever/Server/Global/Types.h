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
		NetLine_MainS,
		NetLine_UserS,
		NetLine_ChatS,
		NetLine_LoginS,
		NetLine_MemCachedS,
		NetLine_User,
		eMAX
	};
}

#define MAX_THREAD_CNT 6;