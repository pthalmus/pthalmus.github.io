#include <Protocol/NetMsg.h>

#include "MainThread.h"
#include "DBThread.h"
#include "DBStruct.h"

bool NetMsgFunc::Request_Login_FromMain(NetMemCached::request_login_fromMain* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_Main_MemCachedS)
	{
		return false;
	}

	// Add the request to the DB thread
	if (GetMainThread().AddDBRequest(new Select_Member(pBase->szUserID, pBase->szPassword, pSession)) == false)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Failed to add login request to DB thread: %s", pBase->szUserID);
		return false;
	}

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Login request from MainServer: %s", pBase->szUserID);
	return true;
}