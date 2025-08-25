#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // windows.h�� winsock.h�� �������� �ʵ��� ��
#endif

#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")
#include <Types.h>

typedef struct _IO_DATA : public WSAOVERLAPPED
{
    WSABUF              wsaBuf;      // �񵿱� I/O�� ���� ���� �����Ϳ� ũ�� ����
    opType::en           opType;      // �۾� ������ �ĺ� (��: RECV, SEND)
    char                    buffer[8192]; // ���� �����Ͱ� ����� ����
} IO_DATA;

typedef struct _USERSESSION
{
    SOCKET		    hSocket;
    NetLine::en	    eLine;
    SOCKADDR     hAddr;

    IO_DATA         recv_io;
    IO_DATA*        send_io;
} USERSESSION;

struct PACKET
{
    uint8_t ucType1;
    uint8_t ucType2;

    virtual size_t GetSize() { return sizeof(PACKET); }
};