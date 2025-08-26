#include <Protocol/NetMsg.h>
#include "Mainthread.h"

bool NetMsgFunc::Request_Connect_FromLogin(NetMain::request_connect_fromLogin* pBase, USERSESSION* pSession)
{
	if (pBase == nullptr || pSession == nullptr)
	{
		return false;
	}
	if (pSession->eLine != NetLine::NetLine_Main)
	{
		return false;
	}

	GetPacketDispatcher().DispatchSend(pSession, (const char*)pBase, pBase->GetSize());
	return true;
}

bool NetMsgFunc::Result_Connect_FromMain(NetMain::result_connect_fromMain* pBase, USERSESSION* pSession)
{
	std::cout << "Connected to MainServer successfully!" << std::endl;
	GetMainThread().StartHeartBeatLoop();
	return true;
}

bool NetMsgFunc::Inform_Heartbeat_FromLogin(NetMain::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession)
{
	GetPacketDispatcher().DispatchSend(pSession, (const char*)pBase, pBase->GetSize());
	return true;
}
