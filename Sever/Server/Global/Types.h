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
		NetLine_Main_LoginS					=0,
		NetLine_Main_UserS					=1,
		NetLine_Main_ChatS					=2,
		NetLine_Main_MemCachedS		=3,
		NetLine_UserS_User					=4,
		NetLine_UserS_MemCachedS		=5,
		NetLine_ChatS_User					=6,
		NetLine_LoginS_User					=7,
		NetLine_MemCachedS				=8,
		eMAX										=255
	};
}

#define MAX_THREAD_CNT 6