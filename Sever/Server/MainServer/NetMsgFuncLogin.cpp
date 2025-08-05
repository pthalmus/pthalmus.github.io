#include "NetMsg.h"

bool NetMsgFunc::Request_Connect_FromLogin(NetMain::request_connect_fromLogin* pBase, USERSESSION* pSession)
{
	if (pSession->eLine != NetLine::NetLine_Main_LoginS)
	{
		return false;
	}
	//GetMainThread().AddLoginServer(pSession->hSocket);

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Complete Connect LoginServer!!");
	return true;
}

bool NetMsgFunc::Inform_Heartbeat_FromLogin(NetMain::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession)
{
	if (pSession->eLine != NetLine::NetLine_Main_LoginS)
	{
		return false;
	}


	return true;
}