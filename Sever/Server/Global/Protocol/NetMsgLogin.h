#pragma once
#include <UserSocket.h>

#pragma pack(push, 1)

//LoginServer와 통신하는 부분을 정의합니다.
//통신 상대는 User와 MainServer입니다.
//MainServer와의 연결정보는 NetMsgMain.h에 정의되어 있습니다.
//해당 Header 파일은 User의 로그인 요청, 로그인 결과, Heartbeat 통신 등을 포함합니다.
//User 인증관련 Mainserver와의 통신도 포함되어 있습니다.
//DEV용 User 인증은 Login입니다.
//Stage 및 Live용 User 인증은 Cert입니다.

namespace NetLogin
{
	enum eNetResult
	{
		eNetSuccess = 0,
		eMAX = 255
	};
	////////////////////////////////////////////////////
	enum { eRequest_Login_FromUser = 1 };
	struct request_login_fromUser : public PACKET
	{
		char szUserID[32]; // User ID
		char szPassword[32]; // Password
		char szClientVersion[16]; // Client Version
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Login_FromLogin = 2 };
	struct result_login_fromLogin : public PACKET
	{
		eNetResult eResult; // Result of the login attempt
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eInform_Heartbeat_FromUser = 3 };
	struct inform_heartbeat_fromUser : public PACKET
	{
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eRequest_Login_FromLogin = 11 };
	struct request_login_fromLogin : public PACKET
	{
		char szUserID[32]; // User ID
		char szPassword[32]; // Password
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Login_FromMain = 12 };
	struct result_login_fromMain : public PACKET
	{
		eNetResult eResult; // Result of the login attempt
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eRequest_Cert_FromLogin = 13 };
	struct request_cert_fromLogin : public PACKET
	{
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Cert_FromMain = 12 };
	struct result_cert_fromMain : public PACKET
	{
		eNetResult eResult; // Result of the login attempt
	};
	////////////////////////////////////////////////////
}

#pragma pack(pop)