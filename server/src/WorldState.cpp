#include "WorldState.h"
#include "f4mp/Logger.h"

namespace f4mp {

WorldState& WorldState::GetInstance() {
    static WorldState instance;
    return instance;
}

void WorldState::UpdateEntity(uint64_t formId, const EntityState& state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entities[formId] = state;
}

EntityState* WorldState::GetEntity(uint64_t formId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entities.find(formId);
    return (it != m_entities.end()) ? &it->second : nullptr;
}

void WorldState::RemoveEntity(uint64_t formId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entities.erase(formId);
}

void WorldState::ForEachEntity(std::function<void(uint64_t, const EntityState&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [formId, state] : m_entities) {
        callback(formId, state);
    }
}

}
