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

	enum class ucType2 : uint8_t {
		request_connect_fromLogin = 1,
		result_connect_fromMain = 2,
		inform_heartbeat_fromLogin = 3,
	};

	////////////////////////////////////////////////////
	enum { eRequest_Connect_FromLogin = 1 };
	struct request_connect_fromLogin : public PACKET
	{
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eResult_Connect_FromMain = 2 };
	struct result_connect_fromMain : public PACKET
	{
	};
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	enum { eInform_Heartbeat_FromLogin = 3 };
	struct inform_heartbeat_fromLogin : public PACKET
	{
	};
	////////////////////////////////////////////////////
}