#pragma once
#include "../../Registry/Registry.h"
#include "../../ECS/Components/Physics/Velocity/Velocity.h"
#include "../../ECS/Components/Physics/Transform/Transform.h"
#include "../../ECS/Components/Stats/CharacterStats/CharacterStats.h"
#include "../../ECS/Components/Control/PlayerInput/PlayerInput.h"
#include "../../ECS/Components/Physics/Facing/Facing.h"
#include "../../ECS/Components/Chara/RangedAttacker.h"
#include "../../ECS/Components/Chara/AreaAttacker.h"
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
            float distance = std::sqrt(distanceSq);

            if (registry.HasComponent<RangedAttackerComponent>(entity)) {
                auto& ranged = registry.GetComponent<RangedAttackerComponent>(entity);

                if (distance > 0.001f) {
                    sf::Vector2f direction = diff / distance;
                    if (distance < ranged.preferredRange) {
                        vel.velocity = -direction * stats.moveSpeed; // kite away
                    } else if (distance > ranged.attackRange) {
                        vel.velocity = direction * stats.moveSpeed; // close the gap
                    } else {
                        vel.velocity = { 0.0f, 0.0f }; // in firing band, hold position
                    }

                    if (registry.HasComponent<FacingComponent>(entity)) {
                        auto& facing = registry.GetComponent<FacingComponent>(entity);
                        if (direction.x > 0) facing.direction = 1;
                        else if (direction.x < 0) facing.direction = -1;
                    }
                } else {
                    vel.velocity = { 0.0f, 0.0f };
                }
                continue;
            }

            if (registry.HasComponent<AreaAttackerComponent>(entity)) {
                auto& area = registry.GetComponent<AreaAttackerComponent>(entity);

                // Rooted while winding up the slam; otherwise closes to triggerRange like a
                // normal chaser and then holds position for EnemyAreaAttackSystem to fire.
                if (area.currentTelegraph >= 0.0f || distance <= area.triggerRange) {
                    vel.velocity = { 0.0f, 0.0f };
                } else if (distance > 0.001f) {
                    sf::Vector2f direction = diff / distance;
                    vel.velocity = direction * stats.moveSpeed;

                    if (registry.HasComponent<FacingComponent>(entity)) {
                        auto& facing = registry.GetComponent<FacingComponent>(entity);
                        if (direction.x > 0) facing.direction = 1;
                        else if (direction.x < 0) facing.direction = -1;
                    }
                }
                continue;
            }

            if (distanceSq > 10.0f * 10.0f) {
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