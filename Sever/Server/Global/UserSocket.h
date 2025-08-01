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
            it->second(header, pSession); // 다운캐스팅된 구조체로 전달
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