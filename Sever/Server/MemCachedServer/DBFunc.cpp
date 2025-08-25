#include "DBStruct.h"
#include <Protocol/NetMsg.h>

bool _Select_Member::Do()
{
	strSql = std::format("exec [dbo].[Select_Member] {},{}", szUserID, szPassword);
	_SQLRESULT result = GetDBManager().Excute(STRDSN_MEMBER_W, strSql);

	if (result.bSuccess == false)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Failed to execute SQL command: %s, ErrorMsg: %s", strSql.c_str(), result.strErrorMessage.c_str());
		return false;
	}

	if (result.Data.empty() || result.Data.front().empty())
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "No data returned for SQL command: %s", strSql.c_str());
		return false;
	}

	bIsValid = stoi(result.Data.front().front());
	return true;
}
void _Select_Member::Done()
{
	if (pSession != nullptr)
	{
		NetMemCached::result_login_fromMemCached* pMsg = CREATE_PACKET(NetMemCached::result_login_fromMemCached, NetLine::NetLine_Main_MemCachedS, NetMemCached::eResult_Login_FromMemCached);
		pMsg->eResult = bIsValid ? NetMemCached::eNetSuccess : NetMemCached::eMAX; // Set result based on bIsValid
		WSASend(pSession->hSocket, (LPWSABUF)pMsg, sizeof(pMsg), NULL, 0, NULL, NULL);
		delete pMsg;
	}
}