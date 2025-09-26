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
		GameServer,
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
		NetLine_Main							= 0, // NetMsgMain 용 라인
		NetLine_Main_LoginS					= 1, // NetMsgLogin 용 라인
		NetLine_Main_UserS					= 2, // NetMsgUser 용 라인
		NetLine_Main_ChatS					= 3, // NetMsgChat 용 라인
		NetLine_Main_MemCachedS		= 4, // NetMsgMemCached 용 라인
		NetLine_Main_GameS					= 5, // NetMsgGame 용 라인
		NetLine_LoginS_User					= 11, // LoginServer - User 용 라인
		NetLine_UserS_User					= 12, // UserServer - User  용 라인
		NetLine_ChatS_User					= 13, // ChatServer - User  용 라인
		NetLine_GameS_User					= 14, // GameServer - User  용 라인
		NetLine_MemCachedS_UserS		= 21, // MemCachedServer - UserServer 용 라인
		eMAX										=255
	};
}

namespace SQLTYPE {
	enum en
	{
		SQL_MEMBER							=1,
		SQL_USER								=2,
		eMAX										=255
	};
}

namespace opType {
	enum en
	{
		IO_RECV,
		IO_SEND,
		IO_ACCEPT,
		IO_CONNECT,
		eMAX
	};
}

namespace HttpMethod {
	enum en
	{
		GET,
		POST,
	};
}



#define MAX_THREAD_CNT 12
#define IOCP_THREAD_CNT 4