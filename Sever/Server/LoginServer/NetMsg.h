#pragma once
#include <stdint.h>
#include <Types.h>

#include "NetMsgMain.h"
#include "NetMsgUser.h"

class NetMsgFunc
{
public:
	//Login <-> Main
	static bool Request_Connect_FromLogin(NetMain::request_connect_fromLogin* pBase, USERSESSION* pSession);
	static bool Result_Connect_FromMain(NetMain::result_connect_fromMain* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromLogin(NetMain::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession);

	//Login <-> User
	static bool Request_Login_FromUser(NetLogin::request_login_fromUser* pBase, USERSESSION* pSession);
	static bool Result_Login_FromUser(NetLogin::result_login_fromUser* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromUser(NetLogin::inform_heartbeat_fromUser* pBase, USERSESSION* pSession);
};

