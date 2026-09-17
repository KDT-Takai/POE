#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Chara/AreaAttacker.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "../../Components/Combat/StatusEffects.h"
#include "../../Components/VFX/HitFlash.h"
#include "../../Components/Components.h"
#include "../Combat/CombatMath.h"
#include "../Combat/ImpactVfx.h"
#include <System/CameraManager/CameraManager.h>
#include <cmath>
#include <algorithm>

// Handles the telegraph/impact lifecycle for monsters with an AreaAttackerComponent
// (see EnemyAISystem for the matching hold-at-range movement behavior: unlike
// RangedAttackerComponent it closes to melee range rather than kiting).
class EnemyAreaAttackSystem {
public:
    void Update(Registry& registry, float dt, sf::Vector2f playerPos) {
        unsigned int playerEntity = -1;
        bool playerFound = false;
        for (auto entity : registry.View<PlayerInputComponent>()) {
            playerEntity = entity;
            playerFound = true;
            break;
        }

        for (auto entity : registry.View<AreaAttackerComponent, CharacterStatsComponent, TransformComponent>()) {
            if (registry.HasComponent<PlayerInputComponent>(entity)) continue;

            auto& area = registry.GetComponent<AreaAttackerComponent>(entity);
            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);
            if (stats.currentHP <= 0.0f) continue;

            auto& trans = registry.GetComponent<TransformComponent>(entity);

            // Pulse the monster's color while winding up so the attack reads as telegraphed.
            if (registry.HasComponent<CircleComponent>(entity)) {
                auto& circle = registry.GetComponent<CircleComponent>(entity);
                circle.color = (area.currentTelegraph >= 0.0f)
                    ? sf::Color(255, 60, 60)
                    : area.baseColor;
            }

            if (area.currentTelegraph >= 0.0f) {
                area.currentTelegraph -= dt;
                if (area.currentTelegraph > 0.0f) continue;

                area.currentTelegraph = -1.0f;
                area.currentCooldown = area.cooldownTime;

                if (!playerFound) continue;

                sf::Vector2f diff = playerPos - trans.position;
                float distanceSq = diff.x * diff.x + diff.y * diff.y;
                if (distanceSq > area.radius * area.radius) continue;

                auto& pStats = registry.GetComponent<CharacterStatsComponent>(playerEntity);
                if (pStats.hitInvincibilityTimer > 0.0f) continue;

                bool isCrit = CombatMath::RollCrit(stats.critRate);
                float rawDamage = stats.atk * (area.damagePercent / 100.0f) * (isCrit ? stats.critDamage : 1.0f);

                bool hasStatus = registry.HasComponent<StatusEffectsComponent>(playerEntity);
                if (hasStatus) {
                    auto& pStatus = registry.GetComponent<StatusEffectsComponent>(playerEntity);
                    if (pStatus.shockRemaining > 0.0f) rawDamage *= (1.0f + pStatus.shockIncreasedDamageTaken);
                }

                float dealt = CombatMath::ApplyDamage(pStats, rawDamage, stats.contactDamageType);

                if (dealt > 0.0f) {
                    registry.AddComponent(playerEntity, HitFlashComponent{ 0.08f });
                    float shakeStrength = std::clamp(dealt / pStats.maxHP, 0.0f, 1.0f) * 16.0f;
                    CameraManager::Instance().Shake(shakeStrength, 0.25f);
                    ImpactVfx::SpawnHitBurst(registry, trans.position, ImpactVfx::ElementColor(stats.contactDamageType), true);
                }

                if (hasStatus) {
                    CombatMath::ApplyAilmentOnHit(registry.GetComponent<StatusEffectsComponent>(playerEntity), stats.contactDamageType, dealt, pStats.maxHP, isCrit);
                }

                pStats.hitInvincibilityTimer = 0.6f;
                continue;
            }

            if (area.currentCooldown > 0.0f) {
                area.currentCooldown -= dt;
                continue;
            }

            if (!playerFound) continue;

            sf::Vector2f diff = playerPos - trans.position;
            float distanceSq = diff.x * diff.x + diff.y * diff.y;
            if (distanceSq <= area.triggerRange * area.triggerRange) {
                area.currentTelegraph = area.telegraphTime;
            }
        }
    }
};
