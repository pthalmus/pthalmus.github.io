#include <Protocol/NetMsg.h>
#include <LogManager.h>

#include "MainThread.h"

bool NetMsgFunc::Request_Connect_FromChat(NetMain::request_connect_fromChat* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_Main_ChatS)
	{
		return false;
	}

	GetMainThread().AddServerList(pSession->hSocket, NetLine::NetLine_Main_ChatS);
	NetMain::result_connect_fromMain* pMsg = CREATE_PACKET(NetMain::result_connect_fromMain, NetLine::NetLine_Main, NetMain::eResult_Connect_FromMain);
	GetPacketDispatcher().DispatchSend(pSession, (const char*)pMsg, pMsg->GetSize());
	return true;
}