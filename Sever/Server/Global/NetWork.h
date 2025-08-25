#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // windows.h가 winsock.h를 포함하지 않도록 함
#endif

#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")
#include <Types.h>

typedef struct _IO_DATA : public WSAOVERLAPPED
{
    WSABUF              wsaBuf;      // 비동기 I/O를 위한 버퍼 포인터와 크기 정보
    opType::en           opType;      // 작업 종류를 식별 (예: RECV, SEND)
    char                    buffer[8192]; // 실제 데이터가 저장될 버퍼
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