#pragma once
#include <stdint.h>
#include <Types.h>
#include <UserSocket.h>
#include <LogManager.h>
#include <DataBaseManager.h>

#include "NetMsgLogin.h"

class NetMsgFunc
{
public:
	static bool Request_Connect_FromLogin(NetMain::request_connect_fromLogin* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromLogin(NetMain::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession);
	static bool Request_Login_FromLogin(NetMain::request_login_fromLogin* pBase, USERSESSION* pSession);
	static bool Result_Login_FromLogin(NetMain::result_login_fromLogin* pBase, USERSESSION* pSession);
};

