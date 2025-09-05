#include "MainThread.h"

bool WINAPI MainThread::Release(DWORD dwType)
{
	if (dwType == CTRL_C_EVENT)
	{
		m_bRunning = false;
		GetLogManager().Release();
		delete m_pMainSSession;
		delete m_pMemCachedSSession;
		::DeleteCriticalSection(&m_cs);

		::CloseHandle(m_hIocp);
		m_hIocp = NULL;

		for (auto& iter : m_umListenSocket)
		{
			::closesocket(iter.second);
		}
		m_umListenSocket.clear();

		WSACleanup();
		return true;
	}
	return false;
}

void MainThread::CloseClient(USERSESSION* pSession)
{
	::shutdown(pSession->hSocket, SD_BOTH);
	::closesocket(pSession->hSocket);

	::EnterCriticalSection(&m_cs);
	switch (pSession->eLine)
	{
	case NetLine::NetLine_UserS_User:
		m_UserList.remove(pSession->hSocket);
		break;
	default:
		m_UserList.remove(pSession->hSocket);
		break;
	}
	::LeaveCriticalSection(&m_cs);
	delete pSession;
}

void MainThread::StartMainThread()
{
	m_bRunning = true;
	if (StartLogSetting() == false)
	{
		return;
	}
	if (LoadConfigSetting() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed Load Config!!");
		return;
	}
	if (StartNetSetting() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed Net Setting!!");
		return;
	}
	if (StartConnectMainServer() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed MainServer Connection!!");
		return;
	}
	if (StartConnectMemCachedServer() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed MemCachedServer Connection!!");
		return;
	}

	std::cout << "Main Thread Start Complete!!" << std::endl;
	while (m_bRunning)
	{
		Sleep(1000);
	}
	return;
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
	std::string strFilePath = "./Config/MemCachedServerConfig.ini";
	bool bResult = false;
	char strTemp[256] = { 0, };
	int nTemp = 0;

	//Main Server Connection Config
	bResult = GetPrivateProfileStringA("MainServer", "IP", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		m_strMainSIP = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load MainServer Connection Config(IP)");
		return false;
	}
	m_nMainSPort = GetPrivateProfileIntA("MainServer", "PORT", 9979, strFilePath.c_str());

	bResult = GetPrivateProfileStringA("MemCachedServer", "IP", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult)
	{
		m_strMemCachedSIP = strTemp;
	}
	else
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Error Occur in Load MainServer Connection Config(IP)");
		return false;
	}
	m_nMemCachedSPort = GetPrivateProfileIntA("MemCachedServer", "PORT", 10479, strFilePath.c_str());
	m_nMemCachedSPort = GetPrivateProfileIntA("User", "PORT", 10465, strFilePath.c_str());
	return true;
}

bool MainThread::StartNetSetting()
{
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	if (LoadAcceptEx() == false)
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

	SOCKET hListenUser = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//bind()/listen() User
	SOCKADDR_IN addrUser;
	addrUser.sin_family = AF_INET;
	addrUser.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	addrUser.sin_port = ::htons(m_nUserPort);

	if (::bind(hListenUser, (SOCKADDR*)&addrUser, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		return false;
	}

	if (::listen(hListenUser, SOMAXCONN) == SOCKET_ERROR)
	{
		return false;
	}

	m_umListenSocket.insert(std::make_pair(NetLine::NetLine_UserS_User, hListenUser));
	CreateIoCompletionPort((HANDLE)m_umListenSocket[NetLine::NetLine_UserS_User], m_hIocp, (ULONG_PTR)m_umListenSocket[NetLine::NetLine_UserS_User], 0);
	if (PostAccept(NetLine::NetLine_UserS_User) == false)
	{
		return false;
	}

	return true;
}

bool MainThread::StartConnectMainServer()
{
	std::cout << "Try Connect MainServer..." << std::endl;

	// 연결을 위한 새로운 소켓 생성
	SOCKET hSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSocket == INVALID_SOCKET)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Create MainServer Socket");
		return false;
	}

	// 소켓을 로컬 주소에 바인딩
	SOCKADDR_IN localAddr;
	ZeroMemory(&localAddr, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 시스템이 적절한 IP를 선택하도록 설정
	localAddr.sin_port = 0; // 시스템이 적절한 포트를 할당하도록 설정
	::bind(hSocket, (SOCKADDR*)&localAddr, sizeof(localAddr));

	m_pMainSSession = new USERSESSION();
	ZeroMemory(m_pMainSSession, sizeof(USERSESSION));
	m_pMainSSession->eLine = NetLine::NetLine_Main; //MainServer Line
	m_pMainSSession->hSocket = hSocket;

	HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)m_pMainSSession->hSocket, m_hIocp, (ULONG_PTR)m_pMainSSession, 0);
	if (hIOCPResult == NULL)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Associate socket with IOCP");
		::closesocket(m_pMainSSession->hSocket);
		delete m_pMainSSession;
		m_pMainSSession = nullptr;
		return false;
	}
	m_pMainSSession->connect_io.opType = opType::IO_CONNECT;
	m_pMainSSession->connect_io.eLine = NetLine::NetLine_Main;
	m_pMainSSession->connect_io.hSocket = hSocket;

	// 포트 바인딩 및 연결
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(m_nMainSPort);
	if (inet_pton(AF_INET, m_strMainSIP.c_str(), &svraddr.sin_addr.S_un.S_addr) != 1)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Main Server IP Convert error!!");
		::closesocket(m_pMainSSession->hSocket);
		delete m_pMainSSession;
		m_pMainSSession = nullptr;
		return false;
	}

	DWORD dwBytes = 0;
	if (!ConnectExPtr(m_pMainSSession->hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr), NULL, 0, &dwBytes, (LPOVERLAPPED)&m_pMainSSession->connect_io))
	{
		int nError = ::WSAGetLastError();
		if (nError != WSA_IO_PENDING)
		{
			GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Connect MainServer");
			::closesocket(m_pMainSSession->hSocket);
			delete m_pMainSSession;
			m_pMainSSession = nullptr;
			return false;
		}
	}
	return true;
}

void MainThread::StartHeartBeatLoop()
{
	GetThreadPool().enqueue([this]() {
		NetMain::inform_heartbeat_fromUserS* pMsg = CREATE_PACKET(NetMain::inform_heartbeat_fromUserS, NetLine::NetLine_Main, NetMain::eInform_Heartbeat_FromUserS);
		while (m_bRunning)
		{
			NetMsgFunc::Inform_Heartbeat_FromUserS(pMsg, m_pMainSSession);
			std::this_thread::sleep_for(std::chrono::seconds(60));
		}

		delete pMsg;
		});
}

bool MainThread::StartConnectMemCachedServer()
{
	std::cout << "Try Connect MainServer..." << std::endl;

	// 연결을 위한 새로운 소켓 생성
	SOCKET hSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSocket == INVALID_SOCKET)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Create MainServer Socket");
		return false;
	}

	// 소켓을 로컬 주소에 바인딩
	SOCKADDR_IN localAddr;
	ZeroMemory(&localAddr, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 시스템이 적절한 IP를 선택하도록 설정
	localAddr.sin_port = 0; // 시스템이 적절한 포트를 할당하도록 설정
	::bind(hSocket, (SOCKADDR*)&localAddr, sizeof(localAddr));

	m_pMemCachedSSession = new USERSESSION();
	m_pMemCachedSSession->eLine = NetLine::NetLine_MemCachedS_UserS;
	m_pMemCachedSSession->hSocket = hSocket;

	HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)m_pMemCachedSSession->hSocket, m_hIocp, (ULONG_PTR)m_pMemCachedSSession, 0);
	if (hIOCPResult == NULL)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Associate socket with IOCP");
		::closesocket(m_pMemCachedSSession->hSocket);
		delete m_pMemCachedSSession;
		m_pMemCachedSSession = nullptr;
		return false;
	}
	m_pMemCachedSSession->connect_io.opType = opType::IO_CONNECT;
	m_pMemCachedSSession->connect_io.eLine = NetLine::NetLine_MemCachedS_UserS;
	m_pMemCachedSSession->connect_io.hSocket = hSocket;

	// 포트 바인딩 및 연결
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(m_nMemCachedSPort);
	if (inet_pton(AF_INET, m_strMemCachedSIP.c_str(), &svraddr.sin_addr.S_un.S_addr) != 1)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Main Server IP Convert error!!");
		::closesocket(m_pMemCachedSSession->hSocket);
		delete m_pMemCachedSSession;
		m_pMemCachedSSession = nullptr;
		return false;
	}
	
	DWORD dwBytes = 0;
	if (!ConnectExPtr(m_pMemCachedSSession->hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr), NULL, 0, &dwBytes, (LPOVERLAPPED)&m_pMemCachedSSession->connect_io))
	{
		int nError = ::WSAGetLastError();
		if (nError != WSA_IO_PENDING)
		{
			GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Connect MainServer");
			::closesocket(m_pMemCachedSSession->hSocket);
			delete m_pMemCachedSSession;
			m_pMemCachedSSession = nullptr;
			return false;
		}
	}
	return true;
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
				case opType::IO_SEND:
				{
					delete pIOData;
				}
					break;
				case opType::IO_RECV:
				{
					//수신한 데이터가 0이면 연결 종료.
					if (dwTransferredSize == 0)
					{
						CloseClient(pSession);
						delete pIOData;
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
					break;
				case opType::IO_CONNECT:
				{
					// ConnectEx 완료 처리
					// SO_UPDATE_CONNECT_CONTEXT 호출 (선택 사항이지만 권장)
					setsockopt(pSession->hSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);

					// 연결 완료 후, Recv 작업을 등록하여 데이터 수신 준비
					pSession->recv_io.opType = opType::IO_RECV;
					pSession->recv_io.wsaBuf.buf = pSession->recv_io.buffer;
					pSession->recv_io.wsaBuf.len = sizeof(pSession->recv_io.buffer);
					DWORD dwRecvBytes = 0;
					DWORD dwFlags = 0;
					if (WSARecv(pSession->hSocket, &pSession->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlags, &pSession->recv_io, NULL) == SOCKET_ERROR)
					{
						if (::WSAGetLastError() != WSA_IO_PENDING)
							puts("\tGQCS: ERROR: WSARecv()");
					}
					if (pSession->eLine == NetLine::NetLine_Main)
					{
						//MainServer로부터 연결 성공 응답을 받으면 서버 등록을 요청합니다.
					}
					else if(pSession->eLine == NetLine::NetLine_MemCachedS_UserS)
					{
						//MemCachedServer로부터 연결 성공 응답을 받으면 서버 등록을 요청합니다.
					}
				}
					break;
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
					DWORD dwRecvBytes = 0;
					DWORD dwFlags = 0;
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

bool MainThread::LoadConnectEx()
{
	SOCKET hDummySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	GUID guidConnectEx = WSAID_CONNECTEX;
	DWORD dwBytes = 0;
	WSAIoctl(hDummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx),
		&ConnectExPtr, sizeof(ConnectExPtr), &dwBytes, NULL, NULL);

	::closesocket(hDummySocket);

	if (ConnectExPtr == nullptr)
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
	// 연결을 받을 소켓을 미리 생성
	SOCKET hAcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hAcceptSocket == INVALID_SOCKET) {
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Create Accept Socket");
		return false;
	}

	// Accept 작업을 위한 IO_DATA 객체 동적 할당
	IO_DATA* pIOData = new IO_DATA;
	if (pIOData == nullptr) {
		::closesocket(hAcceptSocket);
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

void MainThread::CompleteConnectMainServer()
{
	std::cout << "Connect MainServer Success!!" << std::endl;
}