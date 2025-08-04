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

	m_umListenSocket.insert(std::make_pair(NetLine::NetLine_UserS, hListenUser));

	std::thread tAcceptUser(&Mainthread::UserAcceptLoop, this);

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
	SOCKADDR_IN	svraddr = { 0 };
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
	NetLogin::request_connect_fromLogin* pMsg = CREATE_PACKET(NetLogin::request_connect_fromLogin, NetLine::NetLine_LoginS, NetLogin::eRequest_Connect_FromLogin);
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
	LPWSAOVERLAPPED	pWol = NULL;
	DWORD			dwReceiveSize, dwFlag;
	USERSESSION* pNewUser;
	int				nAddrSize = sizeof(SOCKADDR);
	WSABUF			wsaBuf;
	SOCKADDR		ClientAddr;
	SOCKET			hClient;
	int				nRecvResult = 0;

	SOCKET hTargetSocket = m_umListenSocket[NetLine::NetLine_User];

	while ((hClient = ::accept(hTargetSocket,&ClientAddr, &nAddrSize)) != INVALID_SOCKET)
	{
		puts("�� Ŭ���̾�Ʈ�� ����ƽ��ϴ�.");
		::EnterCriticalSection(&m_cs);
		m_UserList.push_back(hClient);
		::LeaveCriticalSection(&m_cs);

		//�� Ŭ���̾�Ʈ�� ���� ���� ��ü ����
		pNewUser = new USERSESSION;
		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;
		pNewUser->eLine = NetLine::NetLine_User;
		pNewUser->hAddr = ClientAddr;

		//�񵿱� ���� ó���� ���� OVERLAPPED ����ü ����.
		pWol = new WSAOVERLAPPED;
		::ZeroMemory(pWol, sizeof(WSAOVERLAPPED));

		//(�����) Ŭ���̾�Ʈ ���� �ڵ��� IOCP�� ����.
		::CreateIoCompletionPort((HANDLE)hClient, m_hIocp,
			(ULONG_PTR)pNewUser,		//KEY!!!
			0);

		dwReceiveSize = 0;
		dwFlag = 0;
		wsaBuf.buf = pNewUser->strRecvBuffer;
		wsaBuf.len = sizeof(pNewUser->strRecvBuffer);

		//Ŭ���̾�Ʈ�� ���� ������ �񵿱� �����Ѵ�.
		nRecvResult = ::WSARecv(hClient, &wsaBuf, 1, &dwReceiveSize,
			&dwFlag, pWol, NULL);
		if (::WSAGetLastError() != WSA_IO_PENDING)
			puts("ERROR: WSARecv() != WSA_IO_PENDING");
	}

	return 0;
}

DWORD WINAPI Mainthread::HeartBeatLoop()
{
	NetLogin::inform_heartbeat_fromLogin* pMsg = CREATE_PACKET(NetLogin::inform_heartbeat_fromLogin, NetLine::NetLine_LoginS, NetLogin::eInform_Heartbeat_FromLogin);
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
	DWORD			dwFlag = 0;
	USERSESSION* pSession = NULL;
	LPWSAOVERLAPPED	pWol = NULL;
	BOOL			bResult;

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "IOCP WorkerThread Start!!");
	while (true)
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
				{
					GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "GQCS: ERROR: WSARecv()");
				}
					
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
	case NetLine::NetLine_User:
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

