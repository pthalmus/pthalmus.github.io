#pragma once

#include <UserSocket.h>

#pragma pack(push, 1)

namespace NetLogin
{
	enum eNetResult
	{
		eNetSuccess	= 0,
		eMAX				= 255
	};

	enum class ucType2 : uint8_t {
		request_login_fromUser			= 1,
		request_logout_fromUser		= 3
	};

	enum { eRequest_Cert_User_fromLogin = 1 };
	struct request_cert_user_fromLogin : public PACKET
	{
		char strUserID[30];
	};
}

#pragma pack(pop)