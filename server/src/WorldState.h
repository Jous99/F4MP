#pragma once

#include <cstdint>
#include <unordered_map>
#include <mutex>

namespace f4mp {

struct EntityState {
    uint64_t formId;
    float x, y, z;
    float yaw, pitch, roll;
    uint32_t health;
    uint32_t maxHealth;
    bool isActive;
};

class WorldState {
public:
    static WorldState& GetInstance();

    void UpdateEntity(uint64_t formId, const EntityState& state);
    EntityState* GetEntity(uint64_t formId);

    void RemoveEntity(uint64_t formId);

    void ForEachEntity(std::function<void(uint64_t, const EntityState&)> callback);

private:
    WorldState() = default;

    std::unordered_map<uint64_t, EntityState> m_entities;
    std::mutex m_mutex;
};

}
