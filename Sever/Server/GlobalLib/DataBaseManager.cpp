#include "pch.h"

DataBaseManager::DataBaseManager()
{
    m_hEnv = SQL_NULL_HENV;
}

bool DataBaseManager::init(const std::string strID, const std::string strPW, const std::string strSoucre)
{
    this->m_strID = strID;
    this->m_strPW = strPW;
    this->m_strDataSource = strSoucre;

    PrintODBCDrivers();

    // ODBC ȯ�� �ڵ� �Ҵ�
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv) != SQL_SUCCESS)
    {
        GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to allocate environment handle.");
        return false;
    }

    // ODBC ȯ�� ���� ����
    if (SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0) != SQL_SUCCESS)
    {
        GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to set ODBC version.");
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    if (addDynamicDsn(this->m_strDataSource) == false)
    {
        GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to Add Dynamic Dsn.");
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

	return true;
}

void DataBaseManager::Release()
{
    this->DIsconnectALL();
}

bool DataBaseManager::Connect(const std::string& strDsn)
{
    if (this->connections.count(strDsn))
    {
        return false;
    }

    SQLHDBC hDbc;

    // ODBC ���� �ڵ� �Ҵ�
    if (SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &hDbc) != SQL_SUCCESS)
    {
        GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to allocate connection handle.");
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    // �����ͺ��̽��� ����
    SQLRETURN ret = SQLConnectA(
        hDbc,
        (SQLCHAR*)strDsn.c_str(), SQL_NTS,
        (SQLCHAR*)m_strID.c_str(), SQL_NTS,
        (SQLCHAR*)m_strPW.c_str(), SQL_NTS
    );

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        int n = WSAGetLastError();
        GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to connect to database.");
        return false;
    }

    connections[strDsn] = hDbc; // ���� ���� �� �ʿ� �߰�
    GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Successfully connected to the database.");
    std::cout << "Successfully connected to the database DSN:"<< strDsn << std::endl;
    return true;
}

void DataBaseManager::Disconnect(const std::string& strDsn)
{
    if (this->connections[strDsn] != SQL_NULL_HDBC)
    {
        SQLDisconnect(this->connections[strDsn]);
        SQLFreeHandle(SQL_HANDLE_DBC, this->connections[strDsn]);
        this->connections[strDsn] = SQL_NULL_HDBC;
        GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Disconnected from the database.");
        std::cout << "Disconnected from the database." << std::endl;
    }
}

void DataBaseManager::DIsconnectALL()
{
    SQLDisconnect(this->connections[STRDSN_MEMBER_W]);
    SQLFreeHandle(SQL_HANDLE_DBC, this->connections[STRDSN_MEMBER_W]);
    this->connections[STRDSN_MEMBER_W] = SQL_NULL_HDBC;
    
    SQLDisconnect(this->connections[STRDSN_USER_W]);
    SQLFreeHandle(SQL_HANDLE_DBC, this->connections[STRDSN_USER_W]);
    this->connections[STRDSN_USER_W] = SQL_NULL_HDBC;
}

bool DataBaseManager::Excute(const std::string& strDsn, const std::string& strSql)
{
    SQLHDBC* phDbc = &this->connections[strDsn];
    if (phDbc  == SQL_NULL_HDBC)
    {
        GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Not connected to the database.");
        return false;
    }

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    if (SQLAllocHandle(SQL_HANDLE_STMT, phDbc, &hStmt) != SQL_SUCCESS)
    {
        GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Failed to allocate statement handle.");
        return false;
    }

    SQLRETURN ret = SQLExecDirectA(hStmt, (SQLCHAR*)strSql.c_str(), SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        std::string strTemp = "Failed to allocate statement handle." + strSql;
        GetLogManager().SystemLog(__FUNCTION__, __LINE__, strTemp.c_str());
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return true;
}

#include <sstream>

// DSN�� �������� �߰��ϴ� �Լ�
bool DataBaseManager::addDynamicDsn(const std::string& server)
{
    std::wstring server_w;
    server_w.assign(server.begin(), server.end());
    // ODBC ���� ���ڿ� ����
    std::wstring connStrMember;
    connStrMember += L"DSN=" + std::wstring(STRDSN_MEMBER) + L"\0";
    connStrMember += L"DRIVER={" + std::wstring(STRDSN_VERSION) + L"}\0";
    connStrMember += L"SERVER=" + server_w + L"\0";
    connStrMember += L"DATABASE=" + std::wstring(STRDSN_DBNAME_MEMBER) + L"\0";
    connStrMember += L"Trusted_Connection=Yes\0";
    connStrMember += L"\0";


    // ODBC ���� ���ڿ� ����
    std::wstring connStrUser;
    connStrUser += L"DSN=" + std::wstring(STRDSN_USER) + L"\0";
    connStrUser += L"DRIVER={" + std::wstring(STRDSN_VERSION) + L"}\0";
    connStrUser += L"SERVER=" + server_w + L"\0";
    connStrUser += L"DATABASE=" + std::wstring(STRDSN_DBNAME_USER) + L"\0";
    connStrUser += L"Trusted_Connection=Yes\0";
    connStrUser += L"\0";

    // DSN �߰�
    // ������ ����, ȯ�濡 ���� 32��Ʈ/64��Ʈ ����̹��� ����ؾ� �� ���� �ֽ��ϴ�.
    BOOL result = SQLConfigDataSource(NULL, ODBC_ADD_SYS_DSN, L"ODBC Driver 17 for SQL Server", connStrMember.c_str());

    if (result == TRUE)
    {
        return true;
    }
    else
    {
        printInstallerError();
        GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Can not Add MemberDsn");
        return false;
    }

    // DSN �߰�
    // ������ ����, ȯ�濡 ���� 32��Ʈ/64��Ʈ ����̹��� ����ؾ� �� ���� �ֽ��ϴ�.
    result = SQLConfigDataSource(NULL, ODBC_ADD_SYS_DSN, L"ODBC Driver 17 for SQL Server", connStrUser.c_str());

    if (result == TRUE)
    {
        return true;
    }
    else
    {
        printInstallerError();
        GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Can not Add UserDsn");
        return false;
    }
}

inline void DataBaseManager::printInstallerError() {
    DWORD error;
    WCHAR errorMsg[1024];
    SQLInstallerErrorW(1, &error, errorMsg, 1024, NULL);
    std::wcout << L"ODBC Error: " << error << std::endl;
}

inline void DataBaseManager::PrintODBCDrivers()
{
    WCHAR name[256];
    WCHAR attr[1024];
    WORD size = 0;

    std::wcout << L"Installed ODBC Drivers:\n";
    BOOL success = SQLGetInstalledDriversW(attr, 1024, &size);
    if (success)
    {
        const wchar_t* p = attr;
        while (*p)
        {
            std::wcout << L"- " << p << std::endl;
            p += wcslen(p) + 1;
        }
    }
}
