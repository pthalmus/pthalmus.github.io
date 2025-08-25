#include "MainThread.h"

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
	if (StartConnectMainServer() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed MainServer Connection!!");
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
		delete m_pMainSSession;
		::DeleteCriticalSection(&m_cs);
		for (auto& iter : m_vIocpThread)
		{
			iter.join();
		}
		m_hDBThread.join();
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

bool Mainthread::StartConnectMainServer()
{
	std::cout << "Try Connect MainServer..." << std::endl;

	m_pMainSSession = new USERSESSION();
	m_pMainSSession->eLine = NetLine::NetLine_Main_MemCachedS; // MainServer Line

	// recv_io 멤버 초기화
	ZeroMemory(&m_pMainSSession->recv_io, sizeof(IO_DATA));
	m_pMainSSession->recv_io.opType = opType::IO_RECV;
	m_pMainSSession->recv_io.wsaBuf.buf = m_pMainSSession->recv_io.buffer;
	m_pMainSSession->recv_io.wsaBuf.len = sizeof(m_pMainSSession->recv_io.buffer);

	m_pMainSSession->hSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_pMainSSession->hSocket == INVALID_SOCKET)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Create MainServer Socket");
		delete m_pMainSSession;
		m_pMainSSession = nullptr;
		return false;
	}

	HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)m_pMainSSession->hSocket, m_hIocp, (ULONG_PTR)m_pMainSSession, 0);
	if (hIOCPResult == NULL)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Associate socket with IOCP");
		::closesocket(m_pMainSSession->hSocket);
		delete m_pMainSSession;
		m_pMainSSession = nullptr;
		return false;
	}

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
	if (::connect(m_pMainSSession->hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Connect MainServer");
		::closesocket(m_pMainSSession->hSocket);
		delete m_pMainSSession;
		m_pMainSSession = nullptr;
		return false;
	}

	DWORD dwRecvBytes = 0;
	DWORD dwFlags = 0;
	if (::WSARecv(m_pMainSSession->hSocket, &m_pMainSSession->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlags, &m_pMainSSession->recv_io, NULL) == SOCKET_ERROR)
	{
		if (::WSAGetLastError() != WSA_IO_PENDING)
		{
			GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "WSARecv failed on MainServer connection");
			::closesocket(m_pMainSSession->hSocket);
			delete m_pMainSSession;
			m_pMainSSession = nullptr;
			return false;
		}
	}

	// Connect 성공시 서버 등록 요청
	NetMain::request_connect_fromMemCached* pMsg = CREATE_PACKET(NetMain::request_connect_fromMemCached, NetLine::NetLine_Main_MemCachedS, NetMain::eRequest_Connect_FromMemCached);
	NetMsgFunc::Request_Connect_FromMemCached(pMsg, m_pMainSSession);
	return true;
}

std::string Mainthread::GetStrServerType()
{
	return std::string(magic_enum::enum_name(m_enType));
}

void Mainthread::CompleteConnectMainServer()
{
	std::cout << "Connect MainServer Success!!" << std::endl;
	std::thread tHeartBeat(&Mainthread::HeartBeatLoop, this);
	tHeartBeat.detach();

	//MainServer로부터 연결 성공 응답을 받으면 DB 정보를 요청합니다.
	NetMain::request_dbinfo_fromMemCached* pMsg = CREATE_PACKET(NetMain::request_dbinfo_fromMemCached, NetLine::NetLine_Main_MemCachedS, NetMain::eRequest_DBInfo_FromMemCached);
	NetMsgFunc::Request_DBInfo_FromMemCached(pMsg, m_pMainSSession);
}

DWORD WINAPI Mainthread::HeartBeatLoop()
{
	NetMain::inform_heartbeat_fromMemCached* pMsg = CREATE_PACKET(NetMain::inform_heartbeat_fromMemCached, NetLine::NetLine_Main_MemCachedS, NetMain::eInform_Heartbeat_FromMemCached);
	while (m_bRunning)
	{
		NetMsgFunc::Inform_Heartbeat_FromMemCached(pMsg, m_pMainSSession);
		std::this_thread::sleep_for(std::chrono::seconds(600));
	}
	delete pMsg;
	return 0;
}

bool Mainthread::LoadConfigSetting()
{
	std::string strFilePath = "./Config/MemCachedServerConfig.ini";
	bool bResult = false;
	char strTemp[256] = { 0, };
	int nTemp = 0;

	//Main Server Connection Config
	bResult = GetPrivateProfileStringA("MainServer", "IP", "", strTemp, sizeof(strTemp), strFilePath.c_str());
	if (bResult == false || strTemp[0] == '\0')
	{
		return false;
	}
	m_strMainSIP = strTemp;
	m_nMainSPort = GetPrivateProfileIntA("MainServer", "PORT", 9979, strFilePath.c_str());
	m_nUserSPort = GetPrivateProfileIntA("UserServer", "PORT", 10479, strFilePath.c_str());
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
	m_hListenSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_hListenSocket == INVALID_SOCKET)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can not Create Listen Socket");
		return false;
	}
	SOCKADDR_IN addrMemCachedS;
	addrMemCachedS.sin_family = AF_INET;
	addrMemCachedS.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	addrMemCachedS.sin_port = ::htons(m_nUserSPort);
	if (::bind(m_hListenSocket, (SOCKADDR*)&addrMemCachedS, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		return false;
	}
	if (::listen(m_hListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}

bool Mainthread::StartDBThread()
{
	m_hDBThread = std::thread(&DBThread::StartDBThread, &m_DBThread);
	m_hDBThread.detach();
	return true;
}

DWORD WINAPI Mainthread::ThreadComplete()
{
	DWORD			dwTransferredSize = 0;
	USERSESSION* pSession = NULL;
	IO_DATA* pIOData = NULL;
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
			if (pIOData->opType == opType::IO_SEND)
			{
				// 동적으로 할당된 SEND용 객체이므로 해제해야 합니다.
				delete pIOData;
			}
			else if (pIOData->opType == opType::IO_RECV)
			{
				// 수신한 데이터가 0이면 연결 종료.
				if (dwTransferredSize == 0)
				{
					// RECV용 pIOData는 USERSESSION의 멤버이므로 삭제하면 안 됩니다!
					// CloseClient()에서 세션 객체 전체를 해제해야 합니다.
					CloseClient(pSession);
					GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Close Client Nomally.");
					continue;
				}

				// GetPacketDispatcher().Dispatch() 호출 전, 패킷 헤더가 유효한지 확인하는 로직이 추가되면 좋습니다.
				GetPacketDispatcher().Dispatch(pIOData->buffer, dwTransferredSize, pSession);

				// 다음 수신 작업을 위한 준비를 합니다.
				// 동일한 USERSESSION 멤버를 사용합니다.
				pIOData->wsaBuf.len = sizeof(pIOData->buffer);
				DWORD dwRecvBytes = 0;
				DWORD dwFlags = 0;
				if (WSARecv(pSession->hSocket, &pIOData->wsaBuf, 1, &dwRecvBytes, &dwFlags, pIOData, NULL) == SOCKET_ERROR)
				{
					if (::WSAGetLastError() != WSA_IO_PENDING)
					{
						puts("\tGQCS: ERROR: WSARecv()");
						CloseClient(pSession); // WSARecv 실패 시 연결 종료
					}
				}
			}
		}
		else // bResult == FALSE (오류)
		{
			DWORD dwError = GetLastError();
			if (pIOData != nullptr) {
				puts("클라이언트 비정상 종료 또는 I/O 작업 실패.");
				CloseClient(pSession);
				// SEND 작업일 경우에만 동적으로 할당된 메모리를 해제
				if (pIOData->opType == opType::IO_SEND)
				{
					delete pIOData;
				}
			}
			else
			{
				puts("ERROR: GetQueuedCompletionStatus() 실패.");
				break;
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
	case NetLine::NetLine_UserS_MemCachedS:
		m_UserSList.remove(pSession->hSocket);
		break;
	default:
		m_UserSList.remove(pSession->hSocket);
		break;
	}
	::LeaveCriticalSection(&m_cs);
}

bool Mainthread::AddDBRequest(SQLDATA* pData)
{
	if (pData == nullptr)
	{
		return false;
	}
	m_DBThread.AddRequest(pData);
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