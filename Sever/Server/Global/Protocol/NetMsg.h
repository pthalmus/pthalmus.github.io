#pragma once
#include <stdint.h>
#include <Types.h>

#include "NetMsgLogin.h"
#include "NetMsgMain.h"
#include "NetMsgUser.h"
#include "NetMsgMemCached.h"

// 프로토콜 메시지 처리 함수들을 정의합니다.
// 각 메시지 타입에 따라 처리 함수를 구현합니다.
// 각 처리 함수의 구현은 라이브러리를 포함하는 곳에서 정의되어야 합니다.

class NetMsgFunc
{
	// NetMsgMain.h
public:
	//Request
	static bool Request_Connect_FromLogin(NetMain::request_connect_fromLogin* pBase, USERSESSION* pSession);
	static bool Request_Connect_FromMemCached(NetMain::request_connect_fromMemCached* pBase, USERSESSION* pSession);
	static bool Request_Connect_FromUserS(NetMain::request_connect_fromUserS* pBase, USERSESSION* pSession);
	static bool Request_Connect_FromChat(NetMain::request_connect_fromChat* pBase, USERSESSION* pSession);
	static bool Request_DBInfo_FromMemCached(NetMain::request_dbinfo_fromMemCached* pBase, USERSESSION* pSession);

	//Result
	static bool Result_Connect_FromMain(NetMain::result_connect_fromMain* pBase, USERSESSION* pSession);
	static bool Result_DBInfo_FromMain(NetMain::result_dbinfo_fromMain* pBase, USERSESSION* pSession);

	//Inform
	static bool Inform_Heartbeat_FromLogin(NetMain::inform_heartbeat_fromLogin* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromMemCached(NetMain::inform_heartbeat_fromMemCached* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromUserS(NetMain::inform_heartbeat_fromUserS* pBase, USERSESSION* pSession);
	static bool Inform_Heartbeat_FromChat(NetMain::inform_heartbeat_fromChat* pBase, USERSESSION* pSession);

	// NetMsgLogin.h
public:
	//Request
	static bool Request_Login_FromUser(NetLogin::request_login_fromUser* pBase, USERSESSION* pSession);
	static bool Request_Login_FromLogin(NetLogin::request_login_fromLogin* pBase, USERSESSION* pSession);
	static bool Request_Cert_FromLogin(NetLogin::request_cert_fromLogin* pBase, USERSESSION* pSession);

	//Result
	static bool Result_Login_FromLogin(NetLogin::result_login_fromLogin* pBase, USERSESSION* pSession);
	static bool Result_Login_FromMain(NetLogin::result_login_fromMain* pBase, USERSESSION* pSession);
	static bool Result_Cert_FromMain(NetLogin::result_cert_fromMain* pBase, USERSESSION* pSession);

	//Inform
	static bool Inform_Heartbeat_FromUser(NetLogin::inform_heartbeat_fromUser* pBase, USERSESSION* pSession);

	// NetMsgMemCached.h
public:
	//Request
	static bool Request_Connect_FromUserS(NetMemCached::request_connect_fromUserS* pBase, USERSESSION* pSession);
	static bool Request_Login_FromMain(NetMemCached::request_login_fromMain* pBase, USERSESSION* pSession);
	static bool Request_Cert_FromMain(NetMemCached::request_cert_fromMain* pBase, USERSESSION* pSession);

	//Result
	static bool Result_Connect_FromMemCached(NetMemCached::result_connect_fromMemCached* pBase, USERSESSION* pSession);
	static bool Result_Login_FromMemCached(NetMemCached::result_login_fromMemCached* pBase, USERSESSION* pSession);
	static bool Result_Cert_FromMemCached(NetMemCached::result_cert_fromMemCached* pBase, USERSESSION* pSession);

	//Inform
	static bool Inform_Heartbeat_FromMemCached(NetMemCached::inform_heartbeat_fromMemCached* pBase, USERSESSION* pSession);

};