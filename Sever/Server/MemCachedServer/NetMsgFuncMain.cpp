#include <Protocol/NetMsg.h>
#include "MainThread.h"

bool NetMsgFunc::Request_Connect_FromMemCached(NetMain::request_connect_fromMemCached* pBase, USERSESSION* pSession)
{
	return false;
}

bool NetMsgFunc::Result_Connect_FromMain(NetMain::result_connect_fromMain* pBase, USERSESSION* pSession)
{
	GetMainThread().CompleteConnectMainServer();
	return true;
}

bool NetMsgFunc::Inform_Heartbeat_FromMemCached(NetMain::inform_heartbeat_fromMemCached* pBase, USERSESSION* pSession)
{
	WSASend(pSession->hSocket, (LPWSABUF)&pBase, sizeof(pBase), NULL, 0, NULL, NULL);
	return true;
}

bool NetMsgFunc::Request_DBInfo_FromMemCached(NetMain::request_dbinfo_fromMemCached* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_Main_MemCachedS)
	{
		return false;
	}
	// Send the DB information request to the MainServer
	WSASend(pSession->hSocket, (LPWSABUF)&pBase, sizeof(pBase), NULL, 0, NULL, NULL);
	return true;
}

bool NetMsgFunc::Result_DBInfo_FromMain(NetMain::result_dbinfo_fromMain* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_Main_MemCachedS)
	{
		return false;
	}
	
	// Store the DB information in the MainThread
	GetMainThread().SetDBInfo(pBase->m_strDBID, pBase->m_strDBPW, pBase->m_strServer);

	if (GetMainThread().StartDBConnection() == false)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Failed to start DB connection in MemCachedServer");
	}
	if(GetMainThread().StartDBThread() == false)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Failed to start DB thread in MemCachedServer");
	}
	return true;
}