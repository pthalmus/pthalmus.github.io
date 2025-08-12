#pragma once
#include <UserSocket.h>

#pragma pack(push, 1)

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
	enum { eResult_Login_FromUser = 2 };
	struct result_login_fromUser : public PACKET
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

}

#pragma pack(pop)