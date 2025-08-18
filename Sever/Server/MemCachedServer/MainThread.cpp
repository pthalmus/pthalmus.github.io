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
	m_pMainSSession = new USERSESSION();
	m_pMainSSession->hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (m_pMainSSession->hSocket == INVALID_SOCKET)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Create MainServer Socket");
		return false;
	}
	//��Ʈ ���ε� �� ����
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
		return false;
	}
	//Connect ������ ���� ��� ��û
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
		std::this_thread::sleep_for(std::chrono::seconds(60));
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
	addrMemCachedS.sin_port = ::htons(m_nMainSPort);
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
	DWORD			dwFlag = 0;
	USERSESSION* pSession = NULL;
	LPWSAOVERLAPPED	pWol = NULL;
	BOOL			bResult;

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "IOCP WorkerThread Start!!");
	while (this->m_bRunning)
	{
		bResult = ::GetQueuedCompletionStatus(
			m_hIocp,							//Dequeue�� IOCP �ڵ�.
			&dwTransferredSize,			//������ ������ ũ��.
			(PULONG_PTR)&pSession,	//���ŵ� �����Ͱ� ����� �޸�
			&pWol,							//OVERLAPPED ����ü.
			INFINITE);						//�̺�Ʈ�� ������ ���.

		if (bResult == TRUE)
		{
			//�������� ���.

			/////////////////////////////////////////////////////////////
			//1. Ŭ���̾�Ʈ�� ������ ���������� �ݰ� ������ ���� ���.
			if (dwTransferredSize == 0)
			{
				CloseClient(pSession);
				delete pWol;
				delete pSession;
				GetLogManager().SystemLog(__FUNCTION__, __LINE__, "Close Client Nomally.");
			}

			/////////////////////////////////////////////////////////////
			//2. Ŭ���̾�Ʈ�� ���� �����͸� ������ ���.
			else
			{
				GetPacketDispatcher().Dispatch(pSession->strRecvBuffer, dwTransferredSize, pSession);
				memset(pSession->strRecvBuffer, 0, sizeof(pSession->strRecvBuffer));

				//�ٽ� IOCP�� ���.
				DWORD dwReceiveSize = 0;
				DWORD dwFlag = 0;
				WSABUF wsaBuf = { 0 };
				wsaBuf.buf = pSession->strRecvBuffer;
				wsaBuf.len = sizeof(pSession->strRecvBuffer);

				::WSARecv(
					pSession->hSocket,	//Ŭ���̾�Ʈ ���� �ڵ�
					&wsaBuf,			//WSABUF ����ü �迭�� �ּ�
					1,					//�迭 ����� ����
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
			//���������� ���.

			/////////////////////////////////////////////////////////////
			//3. �Ϸ� ť���� �Ϸ� ��Ŷ�� ������ ���ϰ� ��ȯ�� ���.
			if (pWol == NULL)
			{
				//IOCP �ڵ��� ���� ���(������ �����ϴ� ���)�� �ش�ȴ�.
				puts("\tGQCS: IOCP �ڵ��� �������ϴ�.");
				break;
			}

			/////////////////////////////////////////////////////////////
			//4. Ŭ���̾�Ʈ�� ������������ ����ưų�
			//   ������ ���� ������ ������ ���.
			else
			{
				if (pSession != NULL)
				{
					CloseClient(pSession);
					delete pWol;
					delete pSession;
				}

				puts("\tGQCS: ���� ���� Ȥ�� �������� ���� ����");
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