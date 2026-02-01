#pragma once
#include "../../Registry/Registry.h"
#include "../../ECS/Components/Physics/Velocity/Velocity.h"
#include "../../ECS/Components/Physics/Transform/Transform.h"
#include "../../ECS/Components/Stats/CharacterStats/CharacterStats.h"
#include "../../ECS/Components/Control/PlayerInput/PlayerInput.h"
#include "../../ECS/Components/Physics/Facing/Facing.h"
#include <cmath>

class EnemyAISystem {
public:
    void Update(Registry& registry, float dt, sf::Vector2f playerPos) {
        auto view = registry.View<CharacterStatsComponent, TransformComponent, VelocityComponent>();

        for (auto entity : view) {
            if (registry.HasComponent<PlayerInputComponent>(entity)) {
                continue;
            }

            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);
            auto& trans = registry.GetComponent<TransformComponent>(entity);
            auto& vel = registry.GetComponent<VelocityComponent>(entity);

            sf::Vector2f diff = playerPos - trans.position;
            float distanceSq = diff.x * diff.x + diff.y * diff.y;

            if (distanceSq > 10.0f * 10.0f) {
                float distance = std::sqrt(distanceSq);

                sf::Vector2f direction = diff / distance;

                vel.velocity = direction * stats.moveSpeed;

                if (registry.HasComponent<FacingComponent>(entity)) {
                    auto& facing = registry.GetComponent<FacingComponent>(entity);
                    if (direction.x > 0) facing.direction = 1;
                    else if (direction.x < 0) facing.direction = -1;
                }
            }
            else {
                vel.velocity = { 0.0f, 0.0f };
            }
        }
    }
};