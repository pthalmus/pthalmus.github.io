#pragma once

#include <winsock2.h>
#pragma comment(lib, "ws2_32")

#include <unordered_map>
#include <functional>
#include <memory>
#include <iostream>

#include <magic_enum/magic_enum.hpp>
#include <Singleton.h>

typedef struct _USERSESSION
{
    SOCKET		hSocket;
    NetLine::en	eLine;
    char			strRecvBuffer[8192];
    char			strSendBuffer[8192];
} USERSESSION;

struct PACKET
{
    uint8_t ucType1;
    uint8_t ucType2;
};

class PacketDispatcher : public Singleton<PacketDispatcher>
{
public:
	using HandlerFunc = std::function<void(PACKET*)>;

    void Register(uint8_t type, uint8_t opcode, HandlerFunc func)
    {
        uint16_t key = (type << 8) | opcode;
        handlers_[key] = std::move(func);
    }

    void Dispatch(void* data, size_t size)
    {
        if (size < sizeof(PACKET)) return;

        auto* header = reinterpret_cast<PACKET*>(data);
        uint16_t key = (header->ucType1 << 8) | header->ucType2;

        auto it = handlers_.find(key);
        if (it != handlers_.end())
        {
            it->second(header); // 다운캐스팅된 구조체로 전달
        }
        else
        {
            std::cout << "No handler for ucType1=" << int(header->ucType1) << ", ucType2=" << int(header->ucType2) << "\n";
        }
    }

private:
    std::unordered_map<uint16_t, HandlerFunc> handlers_;
};

#define GetPacketDispatcher() PacketDispatcher::Instance()

#define AUTO_REGISTER_PACKET_HANDLER(TYPE, OPCODE, STRUCT, FUNC) \
    namespace {                                                                               \
        struct AutoRegister_Helper_##__COUNTER__ {                                 \
            AutoRegister_Helper_##__COUNTER__() {                                   \
                GetPacketDispatcher().Register(                              \
                    static_cast<uint8_t>(TYPE),                                      \
                    static_cast<uint8_t>(OPCODE),                                    \
                    [](PACKET* header) {                                       \
                        FUNC(static_cast<STRUCT*>(header));                          \
                    }                                                                \
                );                                                                   \
            }                                                                        \
        };                                                                           \
        static AutoRegister_Helper_##__COUNTER__ _autoRegisterInstance_##__COUNTER__; \
    }
