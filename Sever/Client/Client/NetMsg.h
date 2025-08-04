#pragma once

#include "NetMsgLogin.h"

class NetMsgFunc
{
public:

	static bool Request_Login_FromUser(NetLogin::request_login_fromUser* pBase, USERSESSION* pSession);
	static bool Result_Login_FromUser(NetLogin::result_login_fromUser* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromUser(NetLogin::inform_heartbeat_fromUser* pBase, USERSESSION* pSession);
};