#include "MainThread.h"
#include "MainThread.h"
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

	m_umListenSocket.insert(std::make_pair(NetLine::NetLine_Main_MemCachedS, hListenMemCachedS));

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
	USERSESSION* pNewUser;
	int					nAddrSize = sizeof(SOCKADDR);
	SOCKADDR		ClientAddr;
	SOCKET			hClient;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_Main_LoginS];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket,&ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("새 클라이언트가 연결됐습니다.");

		//새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		if(pNewUser == nullptr)
		{
			::closesocket(hClient);
			continue;
		}

		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_Main_LoginS;
		pNewUser->hAddr = ClientAddr;

		HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)hClient, m_hIocp,(ULONG_PTR)pNewUser,0);

		if(hIOCPResult == NULL)
		{
			::closesocket(hClient);
			delete pNewUser;
			continue;
		}

		pNewUser->recv_io.opType = opType::IO_RECV;
		pNewUser->recv_io.wsaBuf.buf = pNewUser->recv_io.buffer;
		pNewUser->recv_io.wsaBuf.len = sizeof(pNewUser->recv_io.buffer);

		::EnterCriticalSection(&m_cs);
		m_LoginSList.push_back(hClient);
		::LeaveCriticalSection(&m_cs);

		//클라이언트가 보낸 정보를 비동기 수신한다.
		DWORD dwRecvBytes = 0;
		DWORD dwFlag = 0;
		if (::WSARecv(hClient, &pNewUser->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlag, &pNewUser->recv_io, NULL) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() != WSA_IO_PENDING)
			{
				puts("ERROR: WSARecv() failed on new client connection.");
				// Handle WSARecv failure
				::closesocket(hClient);
				delete pNewUser;
				continue;
			}
		}
	}

	return 0;
}

DWORD WINAPI Mainthread::UserSAcceptLoop()
{
	USERSESSION* pNewUser;
	int					nAddrSize = sizeof(SOCKADDR);
	SOCKADDR		ClientAddr;
	SOCKET			hClient;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_Main_UserS];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket, &ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("새 클라이언트가 연결됐습니다.");

		//새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		if (pNewUser == nullptr)
		{
			::closesocket(hClient);
			continue;
		}

		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_Main_UserS;
		pNewUser->hAddr = ClientAddr;

		HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)hClient, m_hIocp, (ULONG_PTR)pNewUser, 0);

		if (hIOCPResult == NULL)
		{
			::closesocket(hClient);
			delete pNewUser;
			continue;
		}

		pNewUser->recv_io.opType = opType::IO_RECV;
		pNewUser->recv_io.wsaBuf.buf = pNewUser->recv_io.buffer;
		pNewUser->recv_io.wsaBuf.len = sizeof(pNewUser->recv_io.buffer);

		::EnterCriticalSection(&m_cs);
		m_UserSList.push_back(hClient);
		::LeaveCriticalSection(&m_cs);

		//클라이언트가 보낸 정보를 비동기 수신한다.
		DWORD dwRecvBytes = 0;
		DWORD dwFlag = 0;
		if (::WSARecv(hClient, &pNewUser->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlag, &pNewUser->recv_io, NULL) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() != WSA_IO_PENDING)
			{
				puts("ERROR: WSARecv() failed on new client connection.");
				// Handle WSARecv failure
				::closesocket(hClient);
				delete pNewUser;
				continue;
			}
		}
	}

	return 0;
}

DWORD WINAPI Mainthread::ChatSAcceptLoop()
{
	USERSESSION* pNewUser;
	int				nAddrSize = sizeof(SOCKADDR);
	SOCKADDR		ClientAddr;
	SOCKET			hClient;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_Main_ChatS];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket, &ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("새 클라이언트가 연결됐습니다.");

		//새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		if (pNewUser == nullptr)
		{
			::closesocket(hClient);
			continue;
		}

		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_Main_ChatS;
		pNewUser->hAddr = ClientAddr;

		HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)hClient, m_hIocp, (ULONG_PTR)pNewUser, 0);

		if (hIOCPResult == NULL)
		{
			::closesocket(hClient);
			delete pNewUser;
			continue;
		}

		pNewUser->recv_io.opType = opType::IO_RECV;
		pNewUser->recv_io.wsaBuf.buf = pNewUser->recv_io.buffer;
		pNewUser->recv_io.wsaBuf.len = sizeof(pNewUser->recv_io.buffer);

		::EnterCriticalSection(&m_cs);
		m_ChatSList.push_back(hClient);
		::LeaveCriticalSection(&m_cs);

		//클라이언트가 보낸 정보를 비동기 수신한다.
		DWORD dwRecvBytes = 0;
		DWORD dwFlag = 0;
		if (::WSARecv(hClient, &pNewUser->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlag, &pNewUser->recv_io, NULL) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() != WSA_IO_PENDING)
			{
				puts("ERROR: WSARecv() failed on new client connection.");
				::closesocket(hClient);
				delete pNewUser;
				continue;
			}
		}
	}

	return 0;
}

DWORD WINAPI Mainthread::MemCachedSAcceptLoop()
{
	USERSESSION* pNewUser;
	int					nAddrSize = sizeof(SOCKADDR);
	SOCKADDR		ClientAddr;
	SOCKET			hClient;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_Main_MemCachedS];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket, &ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("새 클라이언트가 연결됐습니다.");

		//새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		if (pNewUser == nullptr)
		{
			::closesocket(hClient);
			continue;
		}

		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_Main_MemCachedS;
		pNewUser->hAddr = ClientAddr;

		HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)hClient, m_hIocp, (ULONG_PTR)pNewUser, 0);

		if (hIOCPResult == NULL)
		{
			::closesocket(hClient);
			delete pNewUser;
			continue;
		}

		pNewUser->recv_io.opType = opType::IO_RECV;
		pNewUser->recv_io.wsaBuf.buf = pNewUser->recv_io.buffer;
		pNewUser->recv_io.wsaBuf.len = sizeof(pNewUser->recv_io.buffer);

		::EnterCriticalSection(&m_cs);
		m_MemCachedSList.push_back(hClient);
		::LeaveCriticalSection(&m_cs);

		//클라이언트가 보낸 정보를 비동기 수신한다.
		DWORD dwRecvBytes = 0;
		DWORD dwFlag = 0;
		if (::WSARecv(hClient, &pNewUser->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlag, &pNewUser->recv_io, NULL) == SOCKET_ERROR)
		{
			if (::WSAGetLastError() != WSA_IO_PENDING)
			{
				puts("ERROR: WSARecv() failed on new client connection.");
				// Handle WSARecv failure
				::closesocket(hClient);
				delete pNewUser;
				continue;
			}
		}
	}

	return 0;
}

DWORD WINAPI Mainthread::ThreadComplete()
{
	DWORD			dwTransferredSize = 0;
	USERSESSION* pSession = NULL;
	IO_DATA*		pIOData = NULL;
	BOOL				bResult;

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "IOCP WorkerThread Start!!");
	while (this->m_bRunning)
	{
		bResult = ::GetQueuedCompletionStatus(
			m_hIocp,								//Dequeue할 IOCP 핸들.
			&dwTransferredSize,				//수신한 데이터 크기.
			(PULONG_PTR)&pSession,		//수신된 데이터가 저장된 메모리
			(LPOVERLAPPED*)&pIOData,	//OVERLAPPED 구조체.
			INFINITE);							//이벤트를 무한정 대기.

		if (bResult == TRUE && pIOData != nullptr)
		{
			if(pIOData->opType == opType::IO_SEND)
			{
				delete pIOData;
			}
			else if(pIOData->opType == opType::IO_RECV)
			{
				//수신한 데이터가 0이면 연결 종료.
				if (dwTransferredSize == 0)
				{
					CloseClient(pSession);
					delete pSession;
					GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Close Client Nomally.");
					continue;
				}

				GetPacketDispatcher().Dispatch(pIOData->buffer, dwTransferredSize, pSession);
				pIOData->wsaBuf.len = sizeof(pIOData->buffer);
				DWORD dwRecvBytes = 0;
				DWORD dwFlags = 0;
				if (WSARecv(pSession->hSocket, &pSession->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlags, pIOData, NULL) == SOCKET_ERROR)
				{
					if (::WSAGetLastError() != WSA_IO_PENDING)
						puts("\tGQCS: ERROR: WSARecv()");
				}
			}
		}
		else
		{
			//비정상적인 경우.
			DWORD dwError = GetLastError();
			if (pIOData != nullptr)
			{
				puts("Client terminated abnormally or I/O operation failed.");
				CloseClient(pSession);
				// pIOData가 동적 할당된 경우 여기서 해제
				if (pIOData->opType == opType::IO_SEND)
				{
					delete pIOData;
				}
			}
			else
			{
				puts("ERROR: GetQueuedCompletionStatus() failed.");
				break;
			}
			puts("\tGQCS: 서버 종료 혹은 비정상적 연결 종료");
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
