#include <Protocol/NetMsg.h>
#include <LogManager.h>

#include "MainThread.h"
bool NetMsgFunc::Request_Connect_FromLogin(NetMain::request_connect_fromLogin* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_Main_LoginS)
	{
		return false;
	}

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Complete Connect LoginServer!!");
	GetMainThread().AddServerList(pSession->hSocket, pSession->eLine);
	auto* pMsg = CREATE_PACKET(NetMain::result_connect_fromMain, NetLine::NetLine_Main, NetMain::eResult_Connect_FromMain);
	GetPacketDispatcher().DispatchSend(pSession, (const char*)pMsg, pMsg->GetSize());
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

bool NetMsgFunc::Request_Login_FromLogin(NetLogin::request_login_fromLogin* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_Main_LoginS)
	{
		return false;
	}
	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Login request from LoginServer: %s", pBase->szUserID);

	return false;
}

bool NetMsgFunc::Request_Cert_FromLogin(NetLogin::request_cert_fromLogin* pBase, USERSESSION* pSession)
{
	return true;
}
