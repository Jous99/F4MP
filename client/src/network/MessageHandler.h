#pragma once

#include "f4mp/NetworkProtocol.h"
#include <functional>

namespace f4mp {

class MessageHandler {
public:
    static MessageHandler& GetInstance();

    void Initialize();

    void HandleMessage(const MessageHeader& header, const void* payload);

    void RegisterHandler(MessageType type, std::function<void(const void*)> handler);

private:
    MessageHandler() = default;

    void HandleConnectionAccepted(const void* payload);
    void HandlePlayerPosition(const void* payload);
    void HandleChatMessage(const void* payload);
    void HandleDamageDealt(const void* payload);

    std::function<void(MessageType, const void*)> m_handlers[1000];
};

}
