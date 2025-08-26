#include <Protocol/NetMsg.h>
#include "MainThread.h"

bool NetMsgFunc::Request_Connect_FromUserS(NetMain::request_connect_fromUserS* pBase, USERSESSION* pSession)
{
	GetPacketDispatcher().DispatchSend(pSession, (const char*)pBase, pBase->GetSize());
	return true;
}
bool NetMsgFunc::Result_Connect_FromMain(NetMain::result_connect_fromMain* pBase, USERSESSION* pSession)
{
	GetMainThread().CompleteConnectMainServer();
	return true;
}
bool NetMsgFunc::Inform_Heartbeat_FromUserS(NetMain::inform_heartbeat_fromUserS* pBase, USERSESSION* pSession)
{
	GetPacketDispatcher().DispatchSend(pSession, (const char*)pBase, pBase->GetSize());
	return true;
}