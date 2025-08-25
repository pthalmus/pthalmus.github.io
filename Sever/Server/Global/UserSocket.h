#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <iostream>
#include <utility>

#include <Singleton.h>
#include <NetWork.h>

class PacketDispatcher : public Singleton<PacketDispatcher>
{
public:
	using HandlerFunc = std::function<void(PACKET*, USERSESSION* pSession)>;

    void Register(uint8_t type, uint8_t opcode, HandlerFunc func)
    {
        uint16_t key = (type << 8) | opcode;
        handlers_[key] = std::move(func);
    }

    void Dispatch(void* data, size_t size, USERSESSION* pSession)
    {
        if (size < sizeof(PACKET)) return;

        auto* header = reinterpret_cast<PACKET*>(data);
        uint16_t key = (header->ucType1 << 8) | header->ucType2;

        auto it = handlers_.find(key);
        if (it != handlers_.end())
        {
            std::cout << "Find handler for ucType1=" << int(header->ucType1) << ", ucType2=" << int(header->ucType2) << "\n";
            it->second(header, pSession);
        }
        else
        {
            std::cout << "No handler for ucType1=" << int(header->ucType1) << ", ucType2=" << int(header->ucType2) << "\n";
        }
    }
    bool DispatchSend(USERSESSION* pSession, const char* pstrPacketData, size_t nPacketSize)
    {
        IO_DATA* pIOData = new IO_DATA;
        if (pIOData == nullptr) {
            return false;
        }

        // IO_DATA 구조체 초기화
        ::ZeroMemory(pIOData, sizeof(IO_DATA));
        pIOData->opType = opType::IO_SEND;

        // 보낼 패킷 데이터를 IO_DATA의 버퍼에 복사합니다.
        // 이 방식은 패킷의 크기가 작을 때 유용합니다.
        // 큰 패킷의 경우, 버퍼를 따로 할당하고 포인터를 넘기는 것이 효율적일 수 있습니다.
        memcpy(pIOData->buffer, pstrPacketData, nPacketSize);

        // WSABUF 구조체 설정
        pIOData->wsaBuf.buf = pIOData->buffer;
        pIOData->wsaBuf.len = nPacketSize;

        // WSASend 호출
        DWORD dwBytesSent = 0;
        DWORD dwFlags = 0;
        int nResult = ::WSASend(pSession->hSocket, &pIOData->wsaBuf, 1, &dwBytesSent, dwFlags, pIOData, nullptr);

        if (nResult == SOCKET_ERROR)
        {
            int nError = ::WSAGetLastError();
            if (nError != WSA_IO_PENDING)
            {
                // 오류가 즉시 발생한 경우, 할당된 메모리를 해제합니다.
                delete pIOData;
                return false;
            }
            // WSA_IO_PENDING은 정상적인 비동기 동작이므로 아무것도 하지 않습니다.
        }

        return true;
    }

private:
    std::unordered_map<uint16_t, HandlerFunc> handlers_;
};

#define GetPacketDispatcher() PacketDispatcher::Instance()

#define AUTO_REGISTER_PACKET_HANDLER(TYPE, OPCODE, CODENAME, STRUCT, FUNC) \
    namespace {                                                                               \
        struct AutoRegister_Helper_##TYPE_##CODENAME {                                 \
            AutoRegister_Helper_##TYPE_##CODENAME() {                                   \
                GetPacketDispatcher().Register(                              \
                    static_cast<uint8_t>(NetLine::TYPE),                                      \
                    static_cast<uint8_t>(OPCODE),                                    \
                    [](PACKET* header, USERSESSION* pSession) {                                       \
                        FUNC(static_cast<STRUCT*>(header), pSession);                          \
                    }                                                                \
                );                                                                   \
            }                                                                        \
        };                                                                           \
        static AutoRegister_Helper_##TYPE_##CODENAME _autoRegisterInstance_##TYPE_##CODENAME; \
    }

#define SET_PACKET_TYPE(pMsg, TYPE_ENUM, OPCODE_ENUM) \
    do {                                              \
        (pMsg)->ucType1 = static_cast<uint8_t>(TYPE_ENUM);   \
        (pMsg)->ucType2 = static_cast<uint8_t>(OPCODE_ENUM); \
    } while(0)

#define CREATE_PACKET(PACKET_TYPE, TYPE_ENUM, OPCODE_ENUM)             \
    ([]() -> PACKET_TYPE* {                                            \
        auto* _msg = new PACKET_TYPE();                                \
        SET_PACKET_TYPE(_msg, TYPE_ENUM, OPCODE_ENUM);                 \
        return _msg;                                                   \
    })()