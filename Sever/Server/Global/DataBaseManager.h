#pragma once

#include <UserSocket.h>

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <odbcinst.h>
#include <map>

#include <Singleton.h>
#include <LogManager.h>
#include <Types.h>


typedef struct _SQLDATA
{
	SQLTYPE::en		eType;
	USERSESSION* pSession;
	std::string		strSql;
	virtual bool Do() { return false; } // Execute the SQL command
	virtual bool Done() { return false; } // Optional cleanup after execution
} SQLDATA;

class DataBaseManager : public Singleton<DataBaseManager>
{
private:
	std::string		m_strID;
	std::string		m_strPW;
	std::string		m_strDataSource;

	SQLHENV		m_hEnv;
	std::map<std::string, SQLHDBC> connections;
public:
	DataBaseManager();
	bool init(const std::string strID, const std::string strPW, const std::string strSoucre);
	void Release();
	bool Connect(const std::string& strDsn);
	void Disconnect(const std::string& strDsn);
	void DIsconnectALL();
	bool Excute(const std::string& strDsn, const std::string& strSql);

	bool addDynamicDsn(const std::string& server);
	void printInstallerError();
	void PrintODBCDrivers();
};

#define GetDBManager() DataBaseManager::Instance()

constexpr auto STRDSN_MEMBER =					L"MemberDB";
constexpr auto STRDSN_USER =						L"UserDB";
constexpr auto STRDSN_VERSION =					L"ODBC Driver 17 for SQL Server";
constexpr auto STRDSN_DBNAME_MEMBER =		L"MemberDatabase";
constexpr auto STRDSN_DBNAME_USER =			L"UserDatabase";

constexpr auto STRDSN_MEMBER_W = "MemberDB";
constexpr auto STRDSN_USER_W = "UserDB";
constexpr auto STRDSN_VERSION_W = "ODBC Driver 17 for SQL Server";
constexpr auto STRDSN_DBNAME_MEMBER_W = "MemberDatabase";
constexpr auto STRDSN_DBNAME_USER_W = "UserDatabase";