#include <Protocol/NetMsg.h>
#include <LogManager.h>

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

bool NetMsgFunc::Result_Login_FromLogin(NetLogin::result_login_fromLogin* pBase, USERSESSION* pSession)
{
	return false;
}
