#include "MainThread.h"
#include "MainThread.h"
#include "Mainthread.h"

void MainThread::StartMainThread()
{
	m_bRunning = true;
	if (StartLogSetting() == false)
	{
		m_bRunning = false;
		return;
	}

	if (LoadConfigSetting() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed Load Config!!");
		m_bRunning = false;
		return;
	}

	if (StartNetSetting() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed Net Setting!!");
		m_bRunning = false;
		return;
	}

	if (StartDBConnection() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed DB Setting!!");
		m_bRunning = false;
		return;
	}

	std::cout << "Main Thread Start Complete!!" << std::endl;
	while (m_bRunning)
	{
		Sleep(1);
	}
}

bool WINAPI MainThread::Release(DWORD dwType)
{
	if (dwType == CTRL_C_EVENT)
	{
		m_bRunning = false;
		GetLogManager().Release();
		::DeleteCriticalSection(&m_cs);

		//1. IOCP Thread 종료
		if (m_hIocp)
		{
			//IOCP 핸들 닫기
			::CloseHandle(m_hIocp);
			m_hIocp = NULL;
		}
		//2. Listen Socket 종료
		for (auto& iter : m_umListenSocket)
		{
			::closesocket(iter.second);
		}
		m_umListenSocket.clear();

		//3. Socket 리스트 정리
		for (auto& iter : m_UserSList)
		{
			::shutdown(iter, SD_BOTH);
			::closesocket(iter);
		}
		m_UserSList.clear();
		for (auto& iter : m_ChatSList)
		{
			::shutdown(iter, SD_BOTH);
			::closesocket(iter);
		}
		m_ChatSList.clear();
		for (auto& iter : m_LoginSList)
		{
			::shutdown(iter, SD_BOTH);
			::closesocket(iter);
		}
		m_LoginSList.clear();
		for (auto& iter : m_MemCachedSList)
		{
			::shutdown(iter, SD_BOTH);
			::closesocket(iter);
		}
		m_MemCachedSList.clear();

		//4. DB매니저 종료
		GetDBManager().Release();

		return true;
	}
	return false;
}

bool MainThread::StartLogSetting()
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

bool MainThread::LoadConfigSetting()
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

bool MainThread::StartNetSetting()
{
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	if(LoadAcceptEx() == false)
	{
		return false;
	}
	if (LoadConnectEx() == false)
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
	for (int i = 0; i < IOCP_THREAD_CNT; ++i)
	{
		GetThreadPool().enqueue([this]() { this->ThreadComplete(); });
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

	CreateIoCompletionPort((HANDLE)m_umListenSocket[NetLine::NetLine_Main_LoginS], m_hIocp, (ULONG_PTR)m_umListenSocket[NetLine::NetLine_Main_LoginS], 0);
	CreateIoCompletionPort((HANDLE)m_umListenSocket[NetLine::NetLine_Main_UserS], m_hIocp, (ULONG_PTR)m_umListenSocket[NetLine::NetLine_Main_UserS], 0);
	CreateIoCompletionPort((HANDLE)m_umListenSocket[NetLine::NetLine_Main_ChatS], m_hIocp, (ULONG_PTR)m_umListenSocket[NetLine::NetLine_Main_ChatS], 0);
	CreateIoCompletionPort((HANDLE)m_umListenSocket[NetLine::NetLine_Main_MemCachedS], m_hIocp, (ULONG_PTR)m_umListenSocket[NetLine::NetLine_Main_MemCachedS], 0);

	//Accept Thread Start
	PostAccept(NetLine::NetLine_Main_LoginS);
	PostAccept(NetLine::NetLine_Main_UserS);
	PostAccept(NetLine::NetLine_Main_ChatS);
	PostAccept(NetLine::NetLine_Main_MemCachedS);

	return true;
}

bool MainThread::StartDBConnection()
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

void MainThread::AddServerList(SOCKET hSocket, NetLine::en eLine)
{
	::EnterCriticalSection(&m_cs);
	switch (eLine)
	{
	case NetLine::NetLine_Main_LoginS:
		m_LoginSList.push_back(hSocket);
		break;
	case NetLine::NetLine_Main_MemCachedS:
		m_MemCachedSList.push_back(hSocket);
		break;
	case NetLine::NetLine_Main_UserS:
		m_UserSList.push_back(hSocket);
		break;
	case NetLine::NetLine_Main_ChatS:
		m_ChatSList.push_back(hSocket);
		break;
	case NetLine::NetLine_Main_GameS:
		m_GameSSList.push_back(hSocket);
		break;
	default:
		break;
	}
	::LeaveCriticalSection(&m_cs);
}

DWORD WINAPI MainThread::ThreadComplete()
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
			switch (pIOData->opType)
			{
			case opType::IO_ACCEPT:
			{
				// 비동기 연결 완료 처리
				SOCKET hClientSocket = ((IO_DATA*)pIOData)->hSocket;

				// 1. GetQueuedCompletionStatus가 반환하는 USERSESSION 포인터는 이 시점에 유효하지 않음
				// 2. AcceptEx를 통해 연결된 클라이언트 소켓을 IOCP에 등록
				USERSESSION* pNewUser = new USERSESSION;
				::ZeroMemory(pNewUser, sizeof(USERSESSION));
				pNewUser->hSocket = hClientSocket;
				pNewUser->eLine = pIOData->eLine; // AcceptEx 호출 시 지정한 라인 정보

				SOCKADDR_IN* pLocalAddr = nullptr;
				SOCKADDR_IN* pRemoteAddr = nullptr;
				int localAddrLen = sizeof(SOCKADDR_IN);
				int remoteAddrLen = sizeof(SOCKADDR_IN);
				::GetAcceptExSockaddrs(
					pIOData->buffer, 0,
					sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
					(LPSOCKADDR*)&pLocalAddr, &localAddrLen,
					(LPSOCKADDR*)&pRemoteAddr, &remoteAddrLen);

				if (pRemoteAddr)
				{
					if (remoteAddrLen >= sizeof(SOCKADDR_IN))
					{
						memcpy(&pNewUser->hAddr, pRemoteAddr, sizeof(SOCKADDR_IN));
					}
					else if (remoteAddrLen > 0)
					{
						// remoteAddrLen이 sizeof(SOCKADDR_IN)보다 작을 때는 읽을 수 있는 만큼만 복사
						memcpy(&pNewUser->hAddr, pRemoteAddr, remoteAddrLen);
						// 나머지 영역은 0으로 채움
						if (remoteAddrLen < sizeof(SOCKADDR_IN))
						{
							memset(((char*)&pNewUser->hAddr) + remoteAddrLen, 0, sizeof(SOCKADDR_IN) - remoteAddrLen);
						}
					}
					else
					{
						ZeroMemory(&pNewUser->hAddr, sizeof(SOCKADDR_IN));
					}
				}
				else
				{
					GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "GetAcceptExSockaddrs Failed");
				}

				::CreateIoCompletionPort((HANDLE)hClientSocket, m_hIocp, (ULONG_PTR)pNewUser, 0);

				// 3. WSARecv를 등록하여 데이터 수신 시작
				pNewUser->recv_io.opType = opType::IO_RECV;
				pNewUser->recv_io.wsaBuf.buf = pNewUser->recv_io.buffer;
				pNewUser->recv_io.wsaBuf.len = sizeof(pNewUser->recv_io.buffer);
				DWORD			dwRecvBytes = 0;
				DWORD			dwFlags = 0;
				if (WSARecv(pNewUser->hSocket, &pNewUser->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlags, &pNewUser->recv_io, NULL) == SOCKET_ERROR)
				{
					if (::WSAGetLastError() != WSA_IO_PENDING)
						puts("\tGQCS: ERROR: WSARecv()");
				}

				// 4. 다음 연결을 받기 위해 다시 PostAccept 호출
				PostAccept(pIOData->eLine);
				delete pIOData; // Accept 작업에 사용된 IO_DATA 객체 해제
			}
				break;
			case opType::IO_RECV:
			{
				//수신한 데이터가 0이면 연결 종료.
				if (dwTransferredSize == 0)
				{
					CloseClient(pSession);
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
				break;
			case opType::IO_SEND:
			{
				delete pIOData;
			}
				break;
			case opType::IO_CONNECT:
			{
				//MainServer의 경우 Connect 요청을 보내지 않음.
			}
				break;
			default:
				break;
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

void MainThread::CloseClient(USERSESSION* pSession)
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
	delete pSession;
}

bool MainThread::LoadConnectEx()
{
	SOCKET hDummySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	GUID guidConnectEx = WSAID_CONNECTEX;
	DWORD dwBytes = 0;
	WSAIoctl(hDummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx),
		&ConnectExPtr, sizeof(ConnectExPtr), &dwBytes, NULL, NULL);

	if( ConnectExPtr == nullptr )
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Load ConnectEx Function:", WSAGetLastError());
		return false;
	}

	return true;
}

bool MainThread::LoadAcceptEx()
{
	SOCKET hDummySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes = 0;
	WSAIoctl(hDummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx),
		&lpfnAcceptEx, sizeof(lpfnAcceptEx), &dwBytes, NULL, NULL);
	if (lpfnAcceptEx == nullptr)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Load AcceptEx Function:", WSAGetLastError());
		return false;
	}

	return true;
}

bool MainThread::PostAccept(NetLine::en eLine)
{
	SOCKET hAcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hAcceptSocket == INVALID_SOCKET) {
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Create Accept Socket");
		return false;
	}

	// Accept 작업을 위한 IO_DATA 객체 동적 할당
	IO_DATA* pIOData = new IO_DATA;
	if (pIOData == nullptr) {
		::closesocket(hAcceptSocket);
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Allocate Memory for IO_DATA");
		return false;
	}
	ZeroMemory(pIOData, sizeof(IO_DATA));
	pIOData->opType = opType::IO_ACCEPT;
	pIOData->hSocket = hAcceptSocket;
	pIOData->eLine = eLine;
	pIOData->wsaBuf.buf = pIOData->buffer;
	pIOData->wsaBuf.len = sizeof(pIOData->buffer);

	// listen 소켓에 미리 생성한 소켓을 연결하여 비동기 accept 작업 등록
	DWORD dwBytes = 0;
	if (AcceptEx(m_umListenSocket[eLine], hAcceptSocket, pIOData->buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwBytes, (LPOVERLAPPED)pIOData) == FALSE)
	{
		if (::WSAGetLastError() != WSA_IO_PENDING)
		{
			puts("ERROR: AcceptEx() failed.");
			::closesocket(hAcceptSocket);
			delete pIOData;
			return false;
		}
	}
	return true;
}