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

	// recv_io ��� �ʱ�ȭ
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

	// ��Ʈ ���ε� �� ����
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

	// Connect ������ ���� ��� ��û
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

	//MainServer�κ��� ���� ���� ������ ������ DB ������ ��û�մϴ�.
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
		INVALID_HANDLE_VALUE,	//����� ���� ����.
		NULL,			//���� �ڵ� ����.
		0,				//�ĺ���(Key) �ش���� ����.
		0);				//������ ������ OS�� �ñ�.
	if (m_hIocp == NULL)
	{
		GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Can not Create IOCP Handle");
		return false;
	}

	//IOCP ������� ����
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
			m_hIocp,								//Dequeue�� IOCP �ڵ�.
			&dwTransferredSize,				//������ ������ ũ��.
			(PULONG_PTR)&pSession,		//���ŵ� �����Ͱ� ����� �޸�
			(LPOVERLAPPED*)&pIOData,	//OVERLAPPED ����ü.
			INFINITE);							//�̺�Ʈ�� ������ ���.

		if (bResult == TRUE && pIOData != nullptr)
		{
			if (pIOData->opType == opType::IO_SEND)
			{
				// �������� �Ҵ�� SEND�� ��ü�̹Ƿ� �����ؾ� �մϴ�.
				delete pIOData;
			}
			else if (pIOData->opType == opType::IO_RECV)
			{
				// ������ �����Ͱ� 0�̸� ���� ����.
				if (dwTransferredSize == 0)
				{
					// RECV�� pIOData�� USERSESSION�� ����̹Ƿ� �����ϸ� �� �˴ϴ�!
					// CloseClient()���� ���� ��ü ��ü�� �����ؾ� �մϴ�.
					CloseClient(pSession);
					GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Close Client Nomally.");
					continue;
				}

				// GetPacketDispatcher().Dispatch() ȣ�� ��, ��Ŷ ����� ��ȿ���� Ȯ���ϴ� ������ �߰��Ǹ� �����ϴ�.
				GetPacketDispatcher().Dispatch(pIOData->buffer, dwTransferredSize, pSession);

				// ���� ���� �۾��� ���� �غ� �մϴ�.
				// ������ USERSESSION ����� ����մϴ�.
				pIOData->wsaBuf.len = sizeof(pIOData->buffer);
				DWORD dwRecvBytes = 0;
				DWORD dwFlags = 0;
				if (WSARecv(pSession->hSocket, &pIOData->wsaBuf, 1, &dwRecvBytes, &dwFlags, pIOData, NULL) == SOCKET_ERROR)
				{
					if (::WSAGetLastError() != WSA_IO_PENDING)
					{
						puts("\tGQCS: ERROR: WSARecv()");
						CloseClient(pSession); // WSARecv ���� �� ���� ����
					}
				}
			}
		}
		else // bResult == FALSE (����)
		{
			DWORD dwError = GetLastError();
			if (pIOData != nullptr) {
				puts("Ŭ���̾�Ʈ ������ ���� �Ǵ� I/O �۾� ����.");
				CloseClient(pSession);
				// SEND �۾��� ��쿡�� �������� �Ҵ�� �޸𸮸� ����
				if (pIOData->opType == opType::IO_SEND)
				{
					delete pIOData;
				}
			}
			else
			{
				puts("ERROR: GetQueuedCompletionStatus() ����.");
				break;
			}
		}
	}
	puts("[IOCP �۾��� ������ ����]");
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