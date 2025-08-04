#include "NetMsg.h"

//NetLine_Login
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS_User, NetLogin::eRequest_Login_FromUser, eRequest_Login_FromUser, NetLogin::request_login_fromUser, NetMsgFunc::Request_Login_FromUser)
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS_User, NetLogin::eResult_Login_FromUser, eResult_Login_FromUser, NetLogin::result_login_fromUser, NetMsgFunc::Result_Login_FromUser)
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS_User, NetLogin::eInform_Heartbeat_FromUser, eInform_Heartbeat_FromUser, NetLogin::inform_heartbeat_fromUser, NetMsgFunc::Inform_Heartbeat_FromUser)