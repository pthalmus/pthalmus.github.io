#include "NetMsg.h"

bool NetMsgFunc::Request_Connect_FromLogin(NetLogin::request_connect_fromLogin* pBase, USERSESSION* pSession)
{
	if (pSession->eLine != NetLine::NetLine_LoginS)
	{
		return false;
	}
	//GetMainThread().AddLoginServer(pSession->hSocket);

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Complete Connect LoginServer!!");
	return true;
}

bool NetMsgFunc::Inform_Heartbeat_FromLogin(NetLogin::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession)
{
	if (pSession->eLine != NetLine::NetLine_LoginS)
	{
		return false;
	}


	return true;
}