#include <Protocol/NetMsg.h>

// 사용할 프로토콜 메시지 핸들러를 등록합니다.
// 각 핸들러는 NetMsgFunc 클래스의 정적 메서드를 사용하여 구현됩니다.

//NetLine_Main
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main, NetMain::eRequest_Connect_FromLogin, eRequest_Connect_FromLogin, NetMain::request_connect_fromLogin, NetMsgFunc::Request_Connect_FromLogin)
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main, NetMain::eResult_Connect_FromMain, eResult_Connect_FromMain, NetMain::result_connect_fromMain, NetMsgFunc::Result_Connect_FromMain)
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main, NetMain::eInform_Heartbeat_FromLogin, eInform_Heartbeat_FromLogin, NetMain::inform_heartbeat_fromLogin, NetMsgFunc::Inform_Heartbeat_FromLogin)

//NetLine_Main_LoginS
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main_LoginS, NetLogin::eResult_Login_FromMain, eResult_Login_FromMain, NetLogin::result_login_fromMain, NetMsgFunc::Result_Login_FromMain)
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main_LoginS, NetLogin::eResult_Cert_FromMain, eResult_Cert_FromMain, NetLogin::result_cert_fromMain, NetMsgFunc::Result_Cert_FromMain)

//NetLine_LoginS_User
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS_User, NetLogin::eRequest_Login_FromUser, eRequest_Login_FromUser, NetLogin::request_login_fromUser, NetMsgFunc::Request_Login_FromUser)
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS_User, NetLogin::eInform_Heartbeat_FromUser, eInform_Heartbeat_FromUser, NetLogin::inform_heartbeat_fromUser, NetMsgFunc::Inform_Heartbeat_FromUser)