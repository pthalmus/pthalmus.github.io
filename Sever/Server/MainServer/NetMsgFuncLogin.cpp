#include "NetMsg.h"

AUTO_REGISTER_PACKET_HANDLER(NetLine::NetLine_LoginS, NetLogin::eRequest_Cert_User_fromLogin, NetLogin::request_cert_user_fromLogin, NetMsgFunc::Request_Cert_User_FromLogin)

bool NetMsgFunc::Request_Cert_User_FromLogin(NetLogin::request_cert_user_fromLogin* pBase)
{


	return true;
}