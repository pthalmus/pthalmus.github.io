#include <Protocol/NetMsg.h>
#include <LogManager.h>

#include "MainThread.h"	

bool NetMsgFunc::Request_Connect_FromMemCached(NetMain::request_connect_fromMemCached* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_Main_MemCachedS)
	{
		return false;
	}

	NetMain::result_connect_fromMain* pMsg = CREATE_PACKET(NetMain::result_connect_fromMain, NetLine::NetLine_Main_MemCachedS, NetMain::eResult_Connect_FromMain);
	GetPacketDispatcher().DispatchSend(pSession, (const char*)pMsg, pMsg->GetSize());
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
	auto* pMsg = CREATE_PACKET(NetMain::result_dbinfo_fromMain, NetLine::NetLine_Main_MemCachedS, NetMain::eResult_DBInfo_FromMain);
	GetMainThread().GetDBInfo(pMsg->m_strDBID, sizeof(pMsg->m_strDBID), pMsg->m_strDBPW, sizeof(pMsg->m_strDBPW), pMsg->m_strServer, sizeof(pMsg->m_strServer));
	GetPacketDispatcher().DispatchSend(pSession, (const char*)pMsg, pMsg->GetSize());
	return true;
}

bool NetMsgFunc::Inform_Heartbeat_FromMemCached(NetMain::inform_heartbeat_fromMemCached* pBase, USERSESSION* pSession)
{
	return true;
}