#include <Protocol/NetMsg.h>

// ����� �������� �޽��� �ڵ鷯�� ����մϴ�.
// �� �ڵ鷯�� NetMsgFunc Ŭ������ ���� �޼��带 ����Ͽ� �����˴ϴ�.

//NetLine_LoginS
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main_LoginS, NetMain::eRequest_Connect_FromLogin, eRequest_Connect_FromLogin, NetMain::request_connect_fromLogin, NetMsgFunc::Request_Connect_FromLogin)
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main_LoginS, NetMain::eResult_Connect_FromMain, eResult_Connect_FromMain, NetMain::result_connect_fromMain, NetMsgFunc::Result_Connect_FromMain)
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main_LoginS, NetMain::eInform_Heartbeat_FromLogin, eInform_Heartbeat_FromLogin, NetMain::inform_heartbeat_fromLogin, NetMsgFunc::Inform_Heartbeat_FromLogin)


//NetLine_User
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS_User, NetLogin::eRequest_Login_FromUser, eRequest_Login_FromUser, NetLogin::request_login_fromUser, NetMsgFunc::Request_Login_FromUser)
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS_User, NetLogin::eResult_Login_FromLogin, eResult_Login_FromLogin, NetLogin::result_login_fromLogin, NetMsgFunc::Result_Login_FromLogin)
AUTO_REGISTER_PACKET_HANDLER(NetLine_LoginS_User, NetLogin::eInform_Heartbeat_FromUser, eInform_Heartbeat_FromUser, NetLogin::inform_heartbeat_fromUser, NetMsgFunc::Inform_Heartbeat_FromUser)