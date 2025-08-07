#include "Mainthread.h"

DWORD WINAPI Mainthread::StartMainThread()
{
	m_bRunning = true;
	if (StartLogSetting() == false)
	{
		return 0;
	}

	if (LoadConfigSetting() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed Load Config!!");
		return 0;
	}

	if (StartNetSetting() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed Net Setting!!");
		return 0;
	}

	if (StartDBConnection() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed DB Setting!!");
		return 0;
	}

	std::cout << "Main Thread Start Complete!!" << std::endl;
	while (m_bRunning)
	{
		Sleep(1);
	}

    return 0;
}

bool WINAPI Mainthread::Release(DWORD dwType)
{
	if (dwType == CTRL_C_EVENT)
	{
		m_bRunning = false;
		GetLogManager().Release();
		::DeleteCriticalSection(&m_cs);
		
		for (auto& iter : m_vIocpThread)
		{
			iter.join();
		}
		return true;
	}
	return false;
}

bool Mainthread::StartLogSetting()
{
	std::string strFilePath = std::format("Log\\{0}\\", GetStrServerType());
	if (CreateNestedDirectoryA(strFilePath) == false)
	{
		return false;
	}

	if (GetLogManager().init(strFilePath) == false)
	{
		return false;
	}
	return true;
}

bool Mainthread::LoadConfigSetting()
{
	std::string strFilePath = "./Config/MainServerConfig.ini";
	bool bResult = false;
	char strTemp[256] = { 0, };
	int nTemp = 0;

	//DB Connection Config
	bResult = GetPrivateProfileStringA("DB", "ID", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		m_strDBID = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load DB Connection Config(ID)");
		return false;
	}

	bResult = GetPrivateProfileStringA("DB", "PW", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		m_strDBPW = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load DB Connection Config(PW)");
		return false;
	}

	bResult = GetPrivateProfileStringA("DB", "Server", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		m_strServer = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load DB Connection Config(Server)");
		return false;
	}

	//Login Server Connection Config
	m_nLoginPort = GetPrivateProfileIntA("LoginServer", "PORT", 9973, strFilePath.c_str());

	//User Server Connection Config
	m_nUserPort = GetPrivateProfileIntA("UserServer", "PORT", 9975, strFilePath.c_str());

	//Chat Server Connection Config
	m_nChatPort = GetPrivateProfileIntA("ChatServer", "PORT", 9977, strFilePath.c_str());

	//MemCached Server Connection Config
	m_nMemCachedPort = GetPrivateProfileIntA("MemCachedServer", "PORT", 9979, strFilePath.c_str());

	return true;
}

bool Mainthread::StartNetSetting()
{
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	::InitializeCriticalSection(&m_cs);

	m_hIocp = ::CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,	//연결된 파일 없음.
		NULL,			//기존 핸들 없음.
		0,				//식별자(Key) 해당되지 않음.
		0);				//스레드 개수는 OS에 맡김.
	if (m_hIocp == NULL)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Can not Create IOCP Handle");
		return false;
	}

	//IOCP 스레드들 생성
	for (int i = 0; i < MAX_THREAD_CNT; ++i)
	{
		m_vIocpThread.emplace_back(&Mainthread::ThreadComplete, this);
	}

	SOCKET hListenUserS = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKET hListenChatS = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKET hListenLoginS = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKET hListenMemCachedS = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//bind()/listen() UserS
	SOCKADDR_IN addrUserS;
	addrUserS.sin_family = AF_INET;
	addrUserS.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	addrUserS.sin_port = ::htons(m_nUserPort);

	if (::bind(hListenUserS,(SOCKADDR*)&addrUserS, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		return false;
	}

	if (::listen(hListenUserS, SOMAXCONN) == SOCKET_ERROR)
	{
		return false;
	}

	m_umListenSocket.insert(std::make_pair(NetLine::NetLine_Main_UserS, hListenUserS));

	//bind()/listen() ChatS
	SOCKADDR_IN addrChatS;
	addrChatS.sin_family = AF_INET;
	addrChatS.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	addrChatS.sin_port = ::htons(m_nChatPort);

	if (::bind(hListenChatS, (SOCKADDR*)&addrChatS, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		return false;
	}

	if (::listen(hListenChatS, SOMAXCONN) == SOCKET_ERROR)
	{
		return false;
	}

	m_umListenSocket.insert(std::make_pair(NetLine::NetLine_Main_ChatS, hListenChatS));

	//bind()/listen() LoginS
	SOCKADDR_IN addrLoginS;
	addrLoginS.sin_family = AF_INET;
	addrLoginS.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	addrLoginS.sin_port = ::htons(m_nLoginPort);

	if (::bind(hListenLoginS, (SOCKADDR*)&addrLoginS, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		return false;
	}

	if (::listen(hListenLoginS, SOMAXCONN) == SOCKET_ERROR)
	{
		return false;
	}

	m_umListenSocket.insert(std::make_pair(NetLine::NetLine_Main_LoginS, hListenLoginS));

	//bind()/listen() MemCachedS
	SOCKADDR_IN addrMemCachedS;
	addrMemCachedS.sin_family = AF_INET;
	addrMemCachedS.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	addrMemCachedS.sin_port = ::htons(m_nMemCachedPort);

	if (::bind(hListenMemCachedS, (SOCKADDR*)&addrMemCachedS, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		return false;
	}

	if (::listen(hListenMemCachedS, SOMAXCONN) == SOCKET_ERROR)
	{
		return false;
	}

	m_umListenSocket.insert(std::make_pair(NetLine::NetLine_MemCachedS, hListenMemCachedS));

	std::thread tAcceptLoginS(&Mainthread::LoginSAcceptLoop, this);
	std::thread tAcceptUserS(&Mainthread::UserSAcceptLoop, this);
	std::thread tAcceptChatS(&Mainthread::ChatSAcceptLoop, this);
	std::thread tAcceptMemCachedS(&Mainthread::MemCachedSAcceptLoop, this);

	tAcceptLoginS.detach();
	tAcceptUserS.detach();
	tAcceptChatS.detach();
	tAcceptMemCachedS.detach();

	return true;
}

bool Mainthread::StartDBConnection()
{
	if (GetDBManager().init(m_strDBID, m_strDBPW, m_strServer) == false)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Failed initalize DB Connection");
		return false;
	}

	if (GetDBManager().Connect(STRDSN_MEMBER_W) == false)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Failed initalize MemberDB Connection");
		return false;
	}

	if (GetDBManager().Connect(STRDSN_USER_W) == false)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Failed initalize UserDB Connection");
		return false;
	}
	return true;
}

std::string Mainthread::GetStrServerType()
{
	return std::string(magic_enum::enum_name(m_enType));
}

DWORD WINAPI Mainthread::LoginSAcceptLoop()
{
	LPWSAOVERLAPPED	pWol = NULL;
	DWORD			dwReceiveSize, dwFlag;
	USERSESSION* pNewUser;
	int				nAddrSize = sizeof(SOCKADDR);
	WSABUF			wsaBuf;
	SOCKADDR		ClientAddr;
	SOCKET			hClient;
	int				nRecvResult = 0;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_Main_LoginS];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket,&ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("새 클라이언트가 연결됐습니다.");

		//새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_Main_LoginS;
		pNewUser->hAddr = ClientAddr;

		//비동기 수신 처리를 위한 OVERLAPPED 구조체 생성.
		pWol = new WSAOVERLAPPED;
		::ZeroMemory(pWol, sizeof(WSAOVERLAPPED));

		//(연결된) 클라이언트 소켓 핸들을 IOCP에 연결.
		::CreateIoCompletionPort((HANDLE)hClient, m_hIocp,
			(ULONG_PTR)pNewUser,		//KEY!!!
			0);

		dwReceiveSize = 0;
		dwFlag = 0;
		wsaBuf.buf = pNewUser->strRecvBuffer;
		wsaBuf.len = sizeof(pNewUser->strRecvBuffer);

		//클라이언트가 보낸 정보를 비동기 수신한다.
		nRecvResult = ::WSARecv(hClient, &wsaBuf, 1, &dwReceiveSize,
			&dwFlag, pWol, NULL);
		if (::WSAGetLastError() != WSA_IO_PENDING)
			puts("ERROR: WSARecv() != WSA_IO_PENDING");
	}

	return 0;
}

DWORD WINAPI Mainthread::UserSAcceptLoop()
{
	LPWSAOVERLAPPED	pWol = NULL;
	DWORD			dwReceiveSize, dwFlag;
	USERSESSION* pNewUser;
	int				nAddrSize = sizeof(SOCKADDR);
	WSABUF			wsaBuf;
	SOCKADDR		ClientAddr;
	SOCKET			hClient;
	int				nRecvResult = 0;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_Main_UserS];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket, &ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("새 클라이언트가 연결됐습니다.");
		::EnterCriticalSection(&m_cs);
		m_UserSList.push_back(hClient);
		::LeaveCriticalSection(&m_cs);

		//새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_Main_UserS;
		pNewUser->hAddr = ClientAddr;

		//비동기 수신 처리를 위한 OVERLAPPED 구조체 생성.
		pWol = new WSAOVERLAPPED;
		::ZeroMemory(pWol, sizeof(WSAOVERLAPPED));

		//(연결된) 클라이언트 소켓 핸들을 IOCP에 연결.
		::CreateIoCompletionPort((HANDLE)hClient, m_hIocp,
			(ULONG_PTR)pNewUser,		//KEY!!!
			0);

		dwReceiveSize = 0;
		dwFlag = 0;
		wsaBuf.buf = pNewUser->strRecvBuffer;
		wsaBuf.len = sizeof(pNewUser->strRecvBuffer);

		//클라이언트가 보낸 정보를 비동기 수신한다.
		nRecvResult = ::WSARecv(hClient, &wsaBuf, 1, &dwReceiveSize,
			&dwFlag, pWol, NULL);
		if (::WSAGetLastError() != WSA_IO_PENDING)
			puts("ERROR: WSARecv() != WSA_IO_PENDING");
	}

	return 0;
}

DWORD WINAPI Mainthread::ChatSAcceptLoop()
{
	LPWSAOVERLAPPED	pWol = NULL;
	DWORD			dwReceiveSize, dwFlag;
	USERSESSION* pNewUser;
	int				nAddrSize = sizeof(SOCKADDR);
	WSABUF			wsaBuf;
	SOCKADDR		ClientAddr;
	SOCKET			hClient;
	int				nRecvResult = 0;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_Main_ChatS];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket, &ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("새 클라이언트가 연결됐습니다.");
		::EnterCriticalSection(&m_cs);
		m_ChatSList.push_back(hClient);
		::LeaveCriticalSection(&m_cs);

		//새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_Main_ChatS;
		pNewUser->hAddr = ClientAddr;

		//비동기 수신 처리를 위한 OVERLAPPED 구조체 생성.
		pWol = new WSAOVERLAPPED;
		::ZeroMemory(pWol, sizeof(WSAOVERLAPPED));

		//(연결된) 클라이언트 소켓 핸들을 IOCP에 연결.
		::CreateIoCompletionPort((HANDLE)hClient, m_hIocp,
			(ULONG_PTR)pNewUser,		//KEY!!!
			0);

		dwReceiveSize = 0;
		dwFlag = 0;
		wsaBuf.buf = pNewUser->strRecvBuffer;
		wsaBuf.len = sizeof(pNewUser->strRecvBuffer);

		//클라이언트가 보낸 정보를 비동기 수신한다.
		nRecvResult = ::WSARecv(hClient, &wsaBuf, 1, &dwReceiveSize,
			&dwFlag, pWol, NULL);
		if (::WSAGetLastError() != WSA_IO_PENDING)
			puts("ERROR: WSARecv() != WSA_IO_PENDING");
	}

	return 0;
}

DWORD WINAPI Mainthread::MemCachedSAcceptLoop()
{
	LPWSAOVERLAPPED	pWol = NULL;
	DWORD			dwReceiveSize, dwFlag;
	USERSESSION* pNewUser;
	int				nAddrSize = sizeof(SOCKADDR);
	WSABUF			wsaBuf;
	SOCKADDR		ClientAddr;
	SOCKET			hClient;
	int				nRecvResult = 0;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_MemCachedS];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket, &ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("새 클라이언트가 연결됐습니다.");
		::EnterCriticalSection(&m_cs);
		m_MemCachedSList.push_back(hClient);
		::LeaveCriticalSection(&m_cs);

		//새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_MemCachedS;
		pNewUser->hAddr = ClientAddr;

		//비동기 수신 처리를 위한 OVERLAPPED 구조체 생성.
		pWol = new WSAOVERLAPPED;
		::ZeroMemory(pWol, sizeof(WSAOVERLAPPED));

		//(연결된) 클라이언트 소켓 핸들을 IOCP에 연결.
		::CreateIoCompletionPort((HANDLE)hClient, m_hIocp,
			(ULONG_PTR)pNewUser,		//KEY!!!
			0);

		dwReceiveSize = 0;
		dwFlag = 0;
		wsaBuf.buf = pNewUser->strRecvBuffer;
		wsaBuf.len = sizeof(pNewUser->strRecvBuffer);

		//클라이언트가 보낸 정보를 비동기 수신한다.
		nRecvResult = ::WSARecv(hClient, &wsaBuf, 1, &dwReceiveSize,
			&dwFlag, pWol, NULL);
		if (::WSAGetLastError() != WSA_IO_PENDING)
			puts("ERROR: WSARecv() != WSA_IO_PENDING");
	}

	return 0;
}

void Mainthread::AddLoginServer(SOCKET pSession)
{
	::EnterCriticalSection(&m_cs);
	m_LoginSList.push_back(pSession);
	::LeaveCriticalSection(&m_cs);
}

DWORD WINAPI Mainthread::ThreadComplete()
{
	DWORD			dwTransferredSize = 0;
	DWORD			dwFlag = 0;
	USERSESSION* pSession = NULL;
	LPWSAOVERLAPPED	pWol = NULL;
	BOOL			bResult;

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "IOCP WorkerThread Start!!");
	while (this->m_bRunning)
	{
		bResult = ::GetQueuedCompletionStatus(
			m_hIocp,							//Dequeue할 IOCP 핸들.
			&dwTransferredSize,			//수신한 데이터 크기.
			(PULONG_PTR)&pSession,	//수신된 데이터가 저장된 메모리
			&pWol,							//OVERLAPPED 구조체.
			INFINITE);						//이벤트를 무한정 대기.

		if (bResult == TRUE)
		{
			//정상적인 경우.

			/////////////////////////////////////////////////////////////
			//1. 클라이언트가 소켓을 정상적으로 닫고 연결을 끊은 경우.
			if (dwTransferredSize == 0)
			{
				CloseClient(pSession);
				delete pWol;
				delete pSession;
				GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Close Client Nomally.");
			}

			/////////////////////////////////////////////////////////////
			//2. 클라이언트가 보낸 데이터를 수신한 경우.
			else
			{
				GetPacketDispatcher().Dispatch(pSession->strRecvBuffer, dwTransferredSize, pSession);
				memset(pSession->strRecvBuffer, 0, sizeof(pSession->strRecvBuffer));

				//다시 IOCP에 등록.
				DWORD dwReceiveSize = 0;
				DWORD dwFlag = 0;
				WSABUF wsaBuf = { 0 };
				wsaBuf.buf = pSession->strRecvBuffer;
				wsaBuf.len = sizeof(pSession->strRecvBuffer);

				::WSARecv(
					pSession->hSocket,	//클라이언트 소켓 핸들
					&wsaBuf,			//WSABUF 구조체 배열의 주소
					1,					//배열 요소의 개수
					&dwReceiveSize,
					&dwFlag,
					pWol,
					NULL);
				if (::WSAGetLastError() != WSA_IO_PENDING)
					puts("\tGQCS: ERROR: WSARecv()");
			}
		}
		else
		{
			//비정상적인 경우.

			/////////////////////////////////////////////////////////////
			//3. 완료 큐에서 완료 패킷을 꺼내지 못하고 반환한 경우.
			if (pWol == NULL)
			{
				//IOCP 핸들이 닫힌 경우(서버를 종료하는 경우)도 해당된다.
				puts("\tGQCS: IOCP 핸들이 닫혔습니다.");
				break;
			}

			/////////////////////////////////////////////////////////////
			//4. 클라이언트가 비정상적으로 종료됐거나
			//   서버가 먼저 연결을 종료한 경우.
			else
			{
				if (pSession != NULL)
				{
					CloseClient(pSession);
					delete pWol;
					delete pSession;
				}

				puts("\tGQCS: 서버 종료 혹은 비정상적 연결 종료");
			}
		}
	}

	puts("[IOCP 작업자 스레드 종료]");
	return 0;
}

void Mainthread::CloseClient(USERSESSION* pSession)
{
	::shutdown(pSession->hSocket, SD_BOTH);
	::closesocket(pSession->hSocket);

	::EnterCriticalSection(&m_cs);
	switch (pSession->eLine)
	{
	case NetLine::NetLine_Main_UserS:
		m_UserSList.remove(pSession->hSocket);
		break;
	case NetLine::NetLine_Main_ChatS:
		m_ChatSList.remove(pSession->hSocket);
		break;
	case NetLine::NetLine_Main_LoginS:
		m_LoginSList.remove(pSession->hSocket);
		break;
	case NetLine::NetLine_Main_MemCachedS:
		m_MemCachedSList.remove(pSession->hSocket);
		break;
	default:
		m_UserSList.remove(pSession->hSocket);
		m_ChatSList.remove(pSession->hSocket);
		m_LoginSList.remove(pSession->hSocket);
		m_MemCachedSList.remove(pSession->hSocket);
		break;
	}
	::LeaveCriticalSection(&m_cs);
}
