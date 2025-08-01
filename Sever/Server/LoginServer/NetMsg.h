#pragma once
#include <stdint.h>
#include <Types.h>

#include "NetMsgMain.h"
#include "NetMsgUser.h"

class NetMsgFunc
{
public:
	static bool Request_Connect_FromLogin(NetLogin::request_connect_fromLogin* pBase, USERSESSION* pSession);
	static bool Result_Connect_FromMain(NetLogin::result_connect_fromMain* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromLogin(NetLogin::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession);
};

