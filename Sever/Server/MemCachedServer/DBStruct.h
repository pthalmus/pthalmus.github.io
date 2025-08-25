#pragma once
#include <NetWork.h>
#include <DataBaseManager.h>
#include <LogManager.h>
#include <Types.h>
#include <format>
#include <string>

typedef struct _Select_Member : public SQLDATA
{
	//IN
	char szUserID[32]; // User ID
	char szPassword[32]; // Password

	//OUT
	bool bIsValid; // Is the user certificate valid?

	_Select_Member(const char* userID, const char* password, USERSESSION* in_pSession)
	{
		eType = SQLTYPE::SQL_MEMBER;
		pSession = in_pSession;
		strncpy_s(szUserID, userID, sizeof(szUserID) - 1);
		szUserID[sizeof(szUserID) - 1] = '\0'; // Ensure null termination
		strncpy_s(szPassword, password, sizeof(szPassword) - 1);
		szPassword[sizeof(szPassword) - 1] = '\0'; // Ensure null termination
		bIsValid = false;
	}

	// 명시적으로 Do()를 override하여 상속 오류 해결
	bool Do() override;
	void Done() override;
	std::string GetSql() const override
	{
		return std::format("exec [dbo].[Select_Member] '{}','{}'", szUserID, szPassword);
	}
} Select_Member;