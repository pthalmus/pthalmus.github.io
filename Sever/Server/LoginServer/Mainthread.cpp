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

	if (StartConnectMainServer() == false)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Failed MainServer Connection!!");
		return 0;
	}

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
	std::string strFilePath = "./Config/LoginServerConfig.ini";
	bool bResult = false;
	char strTemp[256] = { 0, };
	int nTemp = 0;

	//MainServer Connection Config
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

	m_nMainSPort = GetPrivateProfileIntA("MainServer", "PORT", 9973, strFilePath.c_str());

	//User Connection Config
	m_nUserPort = GetPrivateProfileIntA("User", "PORT", 10443, strFilePath.c_str());

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

	SOCKET hListenUser = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//bind()/listen() User
	SOCKADDR_IN addrUser;
	addrUser.sin_family = AF_INET;
	addrUser.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	addrUser.sin_port = ::htons(m_nUserPort);

	if (::bind(hListenUser,(SOCKADDR*)&addrUser, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		return false;
	}

	if (::listen(hListenUser, SOMAXCONN) == SOCKET_ERROR)
	{
		return false;
	}

	m_umListenSocket.insert(std::make_pair(NetLine::NetLine_LoginS_User, hListenUser));

	std::thread tAcceptUser(&Mainthread::UserAcceptLoop, this);

	tAcceptUser.detach();

	return true;
}

bool Mainthread::StartConnectMainServer()
{
	std::cout << "Try Connect MainServer..." << std::endl;

	m_pMainSSession = new USERSESSION();
	m_pMainSSession->eLine = NetLine::NetLine_Main_LoginS; //MainServer Line
	m_pMainSSession->hSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_pMainSSession->hSocket == INVALID_SOCKET)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Create MainServer Socket");
		return false;
	}

	HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)m_pMainSSession->hSocket, m_hIocp, (ULONG_PTR)m_pMainSSession, 0);
	if (hIOCPResult == NULL)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Associate socket with IOCP");
		return false;
	}

	//포트 바인딩 및 연결
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(m_nMainSPort);
	if (inet_pton(AF_INET, m_strMainSIP.c_str(), &svraddr.sin_addr.S_un.S_addr) != 1)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Main Server IP Convert error!!");
	}
	if (::connect(m_pMainSSession->hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Connect MainServer");
		// 에러 발생 시 소켓 닫기
		::closesocket(m_pMainSSession->hSocket);
		delete m_pMainSSession;
		m_pMainSSession = nullptr;
		return false;
	}

	DWORD dwRecvBytes = 0;
	DWORD dwFlags = 0;
	LPWSAOVERLAPPED	pWol = NULL;
	if (::WSARecv(m_pMainSSession->hSocket, &m_pMainSSession->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlags, &m_pMainSSession->recv_io, NULL) == SOCKET_ERROR)
	{
		if (::WSAGetLastError() != WSA_IO_PENDING)
		{
			GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "WSARecv failed on MainServer connection");
			// 에러 발생 시 소켓 닫기
			::closesocket(m_pMainSSession->hSocket);
			delete m_pMainSSession;
			m_pMainSSession = nullptr;
			return false;
		}
	}

	//Connect 성공시 서버 등록 요청
	NetMain::request_connect_fromLogin* pMsg = CREATE_PACKET(NetMain::request_connect_fromLogin, NetLine::NetLine_Main_LoginS, NetMain::eRequest_Connect_FromLogin);
	NetMsgFunc::Request_Connect_FromLogin(pMsg, m_pMainSSession);
	return true;
}

std::string Mainthread::GetStrServerType()
{
	return std::string(magic_enum::enum_name(m_enType));
}

void Mainthread::CompleteConnectMainServer()
{
	std::thread tHeartBeat(&Mainthread::HeartBeatLoop, this);
	tHeartBeat.detach();
}

DWORD WINAPI Mainthread::UserAcceptLoop()
{
	USERSESSION* pNewUser;
	int					nAddrSize = sizeof(SOCKADDR);
	SOCKADDR		ClientAddr;
	SOCKET			hClient;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_LoginS_User];

	while (this->m_bRunning && (hClient = ::accept(hTargetSocket,&ClientAddr, &nAddrSize)) != INVALID_SOCKET)
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
		pNewUser->eLine = NetLine::NetLine_LoginS_User;
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
		m_UserList.push_back(hClient);
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

DWORD WINAPI Mainthread::HeartBeatLoop()
{
	NetMain::inform_heartbeat_fromLogin* pMsg = CREATE_PACKET(NetMain::inform_heartbeat_fromLogin, NetLine::NetLine_Main_LoginS, NetMain::eInform_Heartbeat_FromLogin);
	while (m_bRunning)
	{
		NetMsgFunc::Inform_Heartbeat_FromLogin(pMsg, m_pMainSSession);
		std::this_thread::sleep_for(std::chrono::seconds(60));
	}

	delete pMsg;
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
			if (pIOData->opType == opType::IO_SEND)
			{
				delete pIOData;
			}
			else if (pIOData->opType == opType::IO_RECV)
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
	case NetLine::NetLine_LoginS_User:
		m_UserList.remove(pSession->hSocket);
		break;
	default:
		m_UserList.remove(pSession->hSocket);
		break;
	}
	::LeaveCriticalSection(&m_cs);
}

USERSESSION* Mainthread::GetMainServer()
{
	return m_pMainSSession;
}

