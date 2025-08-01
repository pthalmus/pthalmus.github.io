#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // windows.h가 winsock.h를 포함하지 않도록 함
#endif

#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")
#include <Types.h>

typedef struct _USERSESSION
{
    SOCKET		    hSocket;
    NetLine::en	    eLine;
    char			    strRecvBuffer[8192];
    char			    strSendBuffer[8192];
    SOCKADDR     hAddr;
} USERSESSION;

struct PACKET
{
    uint8_t ucType1;
    uint8_t ucType2;
};