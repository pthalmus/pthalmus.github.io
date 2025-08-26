#include "MainClient.h"

bool MainClient::ConnectToLoginServer()
{
	//포트 바인딩 및 연결
	SOCKADDR_IN	svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(m_nLoginSPort);
	if (inet_pton(AF_INET, m_strLoginSIP.c_str(), &svraddr.sin_addr.S_un.S_addr) != 1)
	{
		puts("Can Not Create Socket");
		return false;
	}
	if (::connect(m_pSession->hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		puts("Can not Connect to LoginServer");
		return false;
	}

	DWORD dwRecvBytes = 0;
	DWORD dwFlags = 0;
	if (::WSARecv(m_pSession->hSocket, &m_pSession->recv_io.wsaBuf, 1, &dwRecvBytes, &dwFlags, &m_pSession->recv_io, NULL) == SOCKET_ERROR)
	{
		if (::WSAGetLastError() != WSA_IO_PENDING)
		{
			GetLogManager().ErrorLog(__FUNCTION__, __LINE__, "WSARecv failed on MainServer connection");
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

	std::thread t(&MainClient::ThreadComplete, this);
	t.detach();
	return true;
}

DWORD WINAPI MainClient::ThreadComplete()
{
	DWORD			dwTransferredSize = 0;
	USERSESSION* pSession = NULL;
	IO_DATA* pIOData = NULL;
	BOOL				bResult;

	while (this->m_bRun)
	{
		bResult = ::GetQueuedCompletionStatus(
			this->hCompletionPort,					//Dequeue할 IOCP 핸들.
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
					delete pSession;
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
};