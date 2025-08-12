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

    // ODBC 환경 핸들 할당
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv) != SQL_SUCCESS)
    {
        GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to allocate environment handle.");
        return false;
    }

    // ODBC 환경 버전 설정
    if (SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0) != SQL_SUCCESS)
    {
        GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to set ODBC version.");
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    //if (addDynamicDsn(this->m_strDataSource) == false)
    //{
    //    GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to Add Dynamic Dsn.");
    //    SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
    //    m_hEnv = SQL_NULL_HENV;
    //    return false;
    //}

	return true;
}

void DataBaseManager::Release()
{
    this->DIsconnectALL();
}

bool DataBaseManager::Connect(const std::string& strDsn)
{
    // 이미 연결되어 있으면 false 반환
    if (this->connections.count(strDsn))
    {
        return false;
    }

    // DSN이 시스템에 존재하는지 확인 (간단히 시도, 실패하면 DSN 추가)
    SQLHDBC hDbc;
    if (SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &hDbc) != SQL_SUCCESS)
    {
        GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to allocate connection handle.");
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    SQLRETURN ret = SQLConnectA(
        hDbc,
        (SQLCHAR*)strDsn.c_str(), SQL_NTS,
        (SQLCHAR*)m_strID.c_str(), SQL_NTS,
        (SQLCHAR*)m_strPW.c_str(), SQL_NTS
    );

    // DSN이 없어서 실패한 경우, DSN을 동적으로 추가 시도
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        // DSN 동적 추가 시도
        if (!addDynamicDsn(m_strDataSource))
        {
            GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to add DSN dynamically.");
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            return false;
        }

        // DSN 추가 후 재시도
        ret = SQLConnectA(
            hDbc,
            (SQLCHAR*)strDsn.c_str(), SQL_NTS,
            (SQLCHAR*)m_strID.c_str(), SQL_NTS,
            (SQLCHAR*)m_strPW.c_str(), SQL_NTS
        );

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed to connect to database after adding DSN.");
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            return false;
        }
    }

    connections[strDsn] = hDbc;
    GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Successfully connected to the database.");
    std::cout << "Successfully connected to the database DSN:" << strDsn << std::endl;
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

// DSN을 동적으로 추가하는 함수
bool DataBaseManager::addDynamicDsn(const std::string& server)
{
    // 문자열 변환
    std::wstring server_w(server.begin(), server.end());
    std::wstring id_w(m_strID.begin(), m_strID.end());
    std::wstring pw_w(m_strPW.begin(), m_strPW.end());

    if (server_w.empty())
        server_w = L"(local)";

    // 연결 문자열 생성 함수
    auto makeConnStr = [](const std::wstring& dsn, const std::wstring& db, const std::wstring& server,
                          const std::wstring& id, const std::wstring& pw) -> std::wstring {
        std::wstring conn;
        conn += L"DSN=" + dsn + L"\0";
        conn += L"DRIVER={" + std::wstring(STRDSN_VERSION) + L"}\0";
        conn += L"SERVER=" + server + L"\0";
        conn += L"DATABASE=" + db + L"\0";
        conn += L"Trusted_Connection=No\0";
        conn += L"UID=" + id + L"\0";
        conn += L"PWD=" + pw + L"\0";
        conn += L"\0";
        return conn;
    };

    std::wstring connStrMember = makeConnStr(STRDSN_MEMBER, STRDSN_DBNAME_MEMBER, server_w, id_w, pw_w);
    std::wstring connStrUser   = makeConnStr(STRDSN_USER,   STRDSN_DBNAME_USER,   server_w, id_w, pw_w);

    // 디버깅용 출력
    std::wcout << L"connStrMember: ";
    for (wchar_t ch : connStrMember) std::wcout << (ch ? ch : L'|');
    std::wcout << std::endl;
    std::wcout << L"connStrUser: ";
    for (wchar_t ch : connStrUser) std::wcout << (ch ? ch : L'|');
    std::wcout << std::endl;
    std::wcout << L"STRDSN_VERSION: " << STRDSN_VERSION << std::endl;

    // Member DSN 추가
    BOOL result = SQLConfigDataSourceW(
        NULL, ODBC_ADD_SYS_DSN, STRDSN_VERSION, connStrMember.c_str());
    if (result != TRUE) {
        printInstallerError();
        GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Can not Add MemberDsn");
        return false;
    }

    // User DSN 추가
    result = SQLConfigDataSourceW(
        NULL, ODBC_ADD_SYS_DSN, STRDSN_VERSION, connStrUser.c_str());
    if (result != TRUE) {
        printInstallerError();
        GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Can not Add UserDsn");
        return false;
    }

    return true;
}

inline void DataBaseManager::printInstallerError() {
    DWORD error;
    WCHAR errorMsg[1024];
    int i = 1;
    while (SQLInstallerErrorW(i++, &error, errorMsg, 1024, NULL) == SQL_SUCCESS) {
        std::wcout << L"ODBC Error: " << error << L" - " << errorMsg << std::endl;
    }
}

inline void DataBaseManager::PrintODBCDrivers()
{
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
