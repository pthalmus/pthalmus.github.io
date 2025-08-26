#include <Protocol/NetMsg.h>

//NetLine_Main
AUTO_REGISTER_PACKET_HANDLER(NetLine_Main, NetMain::eResult_Connect_FromMain, eResult_Connect_FromMain, NetMain::result_connect_fromMain, NetMsgFunc::Result_Connect_FromMain)

//NetLine_MemCachedS_UserS
AUTO_REGISTER_PACKET_HANDLER(NetLine_MemCachedS_UserS, NetMemCached::eResult_Connect_FromMemCached, eResult_Connect_FromMemCached, NetMemCached::result_connect_fromMemCached, NetMsgFunc::Result_Connect_FromMemCached)