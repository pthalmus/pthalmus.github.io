#include "NetMsg.h"

AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS, NetLogin::eRequest_Connect_FromLogin, eRequest_Connect_FromLogin, NetLogin::request_connect_fromLogin, NetMsgFunc::Request_Connect_FromLogin)
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS, NetLogin::eInform_Heartbeat_FromLogin, eInform_Heartbeat_FromLogin, NetLogin::inform_heartbeat_fromLogin, NetMsgFunc::Inform_Heartbeat_FromLogin)