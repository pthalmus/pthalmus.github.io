#include <Protocol/NetMsg.h>
#include "Mainthread.h"

bool NetMsgFunc::Request_Login_FromUser(NetLogin::request_login_fromUser* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_LoginS_User)
	{
		return false;
	}
	if (pBase->szUserID[0] == '\0' || pBase->szPassword[0] == '\0' || pBase->szClientVersion[0] == '\0')
	{
		return false; // Invalid login request
	}
	// Log the login request
	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Login request from User: %s",pBase->szUserID);
	
	NetLogin::request_login_fromLogin* pMsg = CREATE_PACKET(NetLogin::request_login_fromLogin, NetLine::NetLine_Main_LoginS, NetLogin::eRequest_Login_FromLogin);
	strcpy_s(pMsg->szUserID, sizeof(pMsg->szUserID), pBase->szUserID);
	strcpy_s(pMsg->szPassword, sizeof(pMsg->szPassword), pBase->szPassword);
	WSASend(GetMainThread().GetMainServer()->hSocket, (LPWSABUF)&pMsg, sizeof(pMsg), NULL, 0, NULL, NULL);
	return true;
}

bool NetMsgFunc::Result_Login_FromLogin(NetLogin::result_login_fromLogin* pBase, USERSESSION* pSession)
{
	return true;
}

bool NetMsgFunc::Inform_Heartbeat_FromUser(NetLogin::inform_heartbeat_fromUser* pBase, USERSESSION* pSession)
{
	return true;
}