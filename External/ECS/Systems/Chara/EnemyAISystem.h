#pragma once
#include "../../Registry/Registry.h"
#include "../../ECS/Components/Physics/Velocity/Velocity.h"
#include "../../ECS/Components/Physics/Transform/Transform.h"
#include "../../ECS/Components/Stats/CharacterStats/CharacterStats.h"
#include "../../ECS/Components/Control/PlayerInput/PlayerInput.h"
#include "../../ECS/Components/Physics/Facing/Facing.h"
#include "../../ECS/Components/Chara/RangedAttacker.h"
#include "../../ECS/Components/Chara/AreaAttacker.h"
#include "../../ECS/Components/Chara/Charger.h"
#include "../../ECS/Components/Chara/EnemyAIState.h"
#include <cmath>

class EnemyAISystem {
public:
    // Detect within this range -- deliberately >= every archetype's own attack/trigger
    // range (RangedAttackerComponent::attackRange=320 is the largest) so an enemy is
    // always already aggro by the time it could otherwise fire/telegraph from outside its
    // own detection radius; EnemyRangedAttackSystem/EnemyAreaAttackSystem/EnemyChargeSystem
    // don't separately check aggro state, they rely on this margin instead. Lose range is
    // bigger still so an already-aggro'd enemy doesn't flicker in/out right at the
    // detection boundary. After losing sight for kLeashDelay seconds it stands down back
    // to idle (see EnemyAIStateComponent) -- it does NOT walk back to its spawn point,
    // just stops.
    static constexpr float kDetectRange = 320.0f;
    static constexpr float kLoseRange = 480.0f;
    static constexpr float kLeashDelay = 3.5f;

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

            if (registry.HasComponent<EnemyAIStateComponent>(entity)) {
                auto& ai = registry.GetComponent<EnemyAIStateComponent>(entity);
                if (!ai.aggro) {
                    if (distance <= kDetectRange) {
                        ai.aggro = true;
                        ai.timeSinceLastSeen = 0.0f;
                    } else {
                        vel.velocity = { 0.0f, 0.0f };
                        continue; // still idle, skip archetype behavior entirely
                    }
                } else if (distance > kLoseRange) {
                    ai.timeSinceLastSeen += dt;
                    if (ai.timeSinceLastSeen >= kLeashDelay) {
                        ai.aggro = false;
                        vel.velocity = { 0.0f, 0.0f };
                        continue; // just leashed back to idle this frame
                    }
                } else {
                    ai.timeSinceLastSeen = 0.0f;
                }
            }

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

            if (registry.HasComponent<ChargerComponent>(entity)) {
                auto& charger = registry.GetComponent<ChargerComponent>(entity);

                if (charger.currentCharge >= 0.0f) {
                    vel.velocity = charger.chargeDirection * stats.moveSpeed * charger.chargeSpeedMultiplier;
                } else if (charger.currentTelegraph >= 0.0f) {
                    vel.velocity = { 0.0f, 0.0f }; // rooted while winding up
                } else if (distance > 0.001f) {
                    sf::Vector2f direction = diff / distance;
                    vel.velocity = direction * stats.moveSpeed; // chase normally otherwise
                } else {
                    vel.velocity = { 0.0f, 0.0f };
                }

                if (distance > 0.001f && registry.HasComponent<FacingComponent>(entity)) {
                    auto& facing = registry.GetComponent<FacingComponent>(entity);
                    if (diff.x > 0) facing.direction = 1;
                    else if (diff.x < 0) facing.direction = -1;
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