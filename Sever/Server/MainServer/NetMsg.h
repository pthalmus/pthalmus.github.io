#pragma once
#include <stdint.h>
#include <Types.h>
#include <UserSocket.h>
#include <LogManager.h>

#include "NetMsgLogin.h"

class NetMsgFunc
{
public:
	static bool Request_Connect_FromLogin(NetMain::request_connect_fromLogin* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromLogin(NetMain::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession);
};

