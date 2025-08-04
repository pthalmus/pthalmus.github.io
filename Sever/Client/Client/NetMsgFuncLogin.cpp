#include "NetMsg.h"

bool NetMsgFunc::Request_Login_FromUser(NetLogin::request_login_fromUser* pBase, USERSESSION* pSession)
{
	return false;
}

bool NetMsgFunc::Result_Login_FromUser(NetLogin::result_login_fromUser* pBase, USERSESSION* pSession)
{
	return false;
}

bool NetMsgFunc::Inform_Heartbeat_FromUser(NetLogin::inform_heartbeat_fromUser* pBase, USERSESSION* pSession)
{
	return false;
}