#pragma once
#include <UserSocket.h>

#pragma pack(push, 1)

//MemCachedServer와 통신하는 부분을 정의합니다.
//통신	상대는 MainServer와 UserServer입니다.
//MainServer와의 연결정보는 NetMsgMain.h에 정의되어 있습니다.
//UserServer와의 연결정보를 포함합니다.

namespace NetMemCached
{
	enum eNetResult
	{
		eNetSuccess = 0,
		eMAX = 255
	};
	////////////////////////////////////////////////////
	enum { eRequest_Connect_FromUserS = 1 };
	struct request_connect_fromUserS : public PACKET
	{
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Connect_FromMemCached = 2 };
	struct result_connect_fromMemCached : public PACKET
	{
		eNetResult m_nResult; // eNetResult
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eInform_Heartbeat_FromMemCached = 3 };
	struct inform_heartbeat_fromMemCached : public PACKET
	{
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eRequest_Login_FromMain = 11 };
	struct request_login_fromMain : public PACKET
	{
		char szUserID[32]; // User ID
		char szPassword[32]; // Password
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Login_FromMemCached = 12 };
	struct result_login_fromMemCached : public PACKET
	{
		eNetResult eResult; // Result of the login attempt
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eRequest_Cert_FromMain = 13 };
	struct request_cert_fromMain : public PACKET
	{
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Cert_FromMemCached = 14 };
	struct result_cert_fromMemCached : public PACKET
	{
		eNetResult eResult; // Result of the certification attempt
	};
	////////////////////////////////////////////////////

}

#pragma pack(pop)