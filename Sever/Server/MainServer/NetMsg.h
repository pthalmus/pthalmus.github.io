#pragma once
#include <stdint.h>
#include <Types.h>

#include "NetMsgLogin.h"

class NetMsgFunc
{
public:
	static bool Request_Cert_User_FromLogin(PACKET* pBase);
};

