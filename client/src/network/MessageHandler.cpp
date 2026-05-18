#include "MessageHandler.h"
#include "NetworkClient.h"
#include "f4mp/Logger.h"

namespace f4mp {

MessageHandler& MessageHandler::GetInstance() {
    static MessageHandler instance;
    return instance;
}

void MessageHandler::Initialize() {
    Logger::Info("Initializing message handlers");

    RegisterHandler(MessageType::ConnectionAccepted, [this](const void* p) { HandleConnectionAccepted(p); });
    RegisterHandler(MessageType::PlayerPosition, [this](const void* p) { HandlePlayerPosition(p); });
    RegisterHandler(MessageType::ChatMessage, [this](const void* p) { HandleChatMessage(p); });
    RegisterHandler(MessageType::DamageDealt, [this](const void* p) { HandleDamageDealt(p); });
}

void MessageHandler::HandleMessage(const MessageHeader& header, const void* payload) {
    Logger::Debug("Received message type: %d, size: %d", static_cast<int>(header.type), header.size);

    int idx = static_cast<int>(header.type);
    if (idx > 0 && idx < 1000 && m_handlers[idx]) {
        m_handlers[idx](payload);
    } else {
        Logger::Warn("No handler for message type: %d", static_cast<int>(header.type));
    }
}

void MessageHandler::RegisterHandler(MessageType type, std::function<void(const void*)> handler) {
    int idx = static_cast<int>(type);
    if (idx > 0 && idx < 1000) {
        m_handlers[idx] = std::move(handler);
    }
}

void MessageHandler::HandleConnectionAccepted(const void* payload) {
    const auto* msg = reinterpret_cast<const ConnectionAcceptedMsg*>(payload);
    Logger::Info("Connection accepted. Player ID: %d, Players: %d/%d",
        msg->playerId, msg->currentPlayers, msg->maxPlayers);

    NetworkClient::GetInstance().SetPlayerId(msg->playerId);
}

void MessageHandler::HandlePlayerPosition(const void* payload) {
    const auto* msg = reinterpret_cast<const PlayerPositionMsg*>(payload);
    Logger::Debug("Player %d position: (%.2f, %.2f, %.2f)", msg->playerId, msg->x, msg->y, msg->z);
}

void MessageHandler::HandleChatMessage(const void* payload) {
    const auto* msg = reinterpret_cast<const ChatMessageMsg*>(payload);
    Logger::Info("[Chat] Player %d: %s", msg->senderId, msg->message);
}

void MessageHandler::HandleDamageDealt(const void* payload) {
    const auto* msg = reinterpret_cast<const DamageDealtMsg*>(payload);
    Logger::Debug("Damage: Player %d -> Target %d, Amount: %.1f",
        msg->attackerId, msg->targetId, msg->damageAmount);
}

}
