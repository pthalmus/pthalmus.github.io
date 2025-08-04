#include "NetMsg.h"
#include "Mainthread.h"



bool NetMsgFunc::Request_Connect_FromLogin(NetMain::request_connect_fromLogin* pBase, USERSESSION* pSession)
{
	::send(pSession->hSocket, (const char*)&pBase, sizeof(pBase), 0);
	return true;
}

bool NetMsgFunc::Result_Connect_FromMain(NetMain::result_connect_fromMain* pBase, USERSESSION* pSession)
{
	GetMainThread().HeartBeatLoop();
	return true;
}

bool NetMsgFunc::Inform_Heartbeat_FromLogin(NetMain::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession)
{
	::send(pSession->hSocket, (const char*)&pBase, sizeof(pBase), 0);
	return true;
}
