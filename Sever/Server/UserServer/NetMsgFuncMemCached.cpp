#include <Protocol/NetMsg.h>
#include "MainThread.h"

bool NetMsgFunc::Request_Connect_FromUserS(NetMemCached::request_connect_fromUserS* pBase, USERSESSION* pSession)
{
	GetPacketDispatcher().DispatchSend(pSession, (const char*)pBase, pBase->GetSize());
	return true;
}

bool NetMsgFunc::Result_Connect_FromMemCached(NetMemCached::result_connect_fromMemCached* pBase, USERSESSION* pSession)
{
	return true;
}