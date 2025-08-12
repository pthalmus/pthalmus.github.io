#include "DBStruct.h"

bool _Select_Member::Do()
{
	strSql = std::format("exec [dbo].[Select_Member] {},{}", szUserID, szPassword);

	return true;
}
bool _Select_Member::Done()
{
	return false;
}