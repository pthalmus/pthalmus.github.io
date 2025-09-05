#include "MainClient.h"

bool MainClient::ConnectToLoginServer()
{
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

	m_pSession->eLine = NetLine::NetLine_LoginS_User;
	m_pSession->hSocket = hSocket;

	HANDLE hIOCPResult = ::CreateIoCompletionPort((HANDLE)m_pSession->hSocket, hCompletionPort, (ULONG_PTR)m_pSession, 0);
	if (hIOCPResult == NULL)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Associate socket with IOCP");
		::closesocket(m_pSession->hSocket);
		delete m_pSession;
		m_pSession = nullptr;
		return false;
	}
	m_pSession->connect_io.opType = opType::IO_CONNECT;
	m_pSession->connect_io.eLine = NetLine::NetLine_LoginS_User;
	m_pSession->connect_io.hSocket = hSocket;

	// 포트 바인딩 및 연결
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(m_nLoginSPort);
	if (inet_pton(AF_INET, m_strLoginSIP.c_str(), &svraddr.sin_addr.S_un.S_addr) != 1)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Main Server IP Convert error!!");
		::closesocket(m_pSession->hSocket);
		delete m_pSession;
		m_pSession = nullptr;
		return false;
	}
	DWORD dwBytes = 0;
	if (!ConnectExPtr(m_pSession->hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr), NULL, 0, &dwBytes, (LPOVERLAPPED)&m_pSession->connect_io))
	{
		int nError = ::WSAGetLastError();
		if (nError != WSA_IO_PENDING)
		{
			GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "Can Not Connect MainServer");
			::closesocket(m_pSession->hSocket);
			delete m_pSession;
			m_pSession = nullptr;
			return false;
		}
	}
	return true;
}

bool MainClient::StartNetwork()
{
	if (LoadConnectEx() == false)
	{
		return false;
	}
	USERSESSION* session = new USERSESSION();
	if (session == nullptr)
	{
		return false;
	}
	ZeroMemory(&session->recv_io, sizeof(IO_DATA));
	session->recv_io.opType = opType::IO_RECV;
	session->recv_io.wsaBuf.buf = session->recv_io.buffer;
	session->recv_io.wsaBuf.len = sizeof(session->recv_io.buffer);

	this->m_pSession = session;
	this->hCompletionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (this->hCompletionPort == NULL)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "CreateIoCompletionPort() failed");
		return false;
	}

	m_pSession->hSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_pSession->hSocket == INVALID_SOCKET)
	{
		GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "WSASocket() failed");
		return false;
	}

	GetThreadPool().enqueue([this]() { this->ThreadComplete(); });
	return true;
}

DWORD WINAPI MainClient::ThreadComplete()
{
	DWORD			dwTransferredSize = 0;
	USERSESSION* pSession = NULL;
	IO_DATA* pIOData = NULL;
	BOOL				bResult;

	GetLogManager().SystemLog(__FUNCTION__, __LINE__, "IOCP WorkerThread Start!!");
	while (this->m_bRun)
	{
		bResult = ::GetQueuedCompletionStatus(
			hCompletionPort,					//Dequeue할 IOCP 핸들.
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
					delete pIOData;
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
			}
			break;
			case opType::IO_ACCEPT:
			{
				//Client는 Aceept를 받지 않습니다.
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
bool MainClient::LoadConnectEx()
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
};