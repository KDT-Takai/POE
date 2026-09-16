#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Chara/Charger.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "../../Components/Components.h"
#include <cmath>

// Owns the telegraph -> charge -> cooldown state machine for ChargerComponent (see
// EnemyAISystem for the matching movement: rooted while telegraphing, bursts along
// chargeDirection while charging, otherwise chases normally like a plain trash monster).
// Deals no damage itself -- a charging monster hits the player through the normal
// contact-damage collision path, same as any other monster touching them.
class EnemyChargeSystem {
public:
    void Update(Registry& registry, float dt, sf::Vector2f playerPos) {
        for (auto entity : registry.View<ChargerComponent, CharacterStatsComponent, TransformComponent>()) {
            if (registry.HasComponent<PlayerInputComponent>(entity)) continue;

            auto& charger = registry.GetComponent<ChargerComponent>(entity);
            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);
            if (stats.currentHP <= 0.0f) continue;

            auto& trans = registry.GetComponent<TransformComponent>(entity);

            if (registry.HasComponent<CircleComponent>(entity)) {
                auto& circle = registry.GetComponent<CircleComponent>(entity);
                circle.color = (charger.currentTelegraph >= 0.0f) ? sf::Color(255, 60, 60) : charger.baseColor;
            }

            if (charger.currentCharge >= 0.0f) {
                charger.currentCharge -= dt;
                if (charger.currentCharge <= 0.0f) {
                    charger.currentCharge = -1.0f;
                    charger.currentCooldown = charger.cooldownTime;
                }
                continue;
            }

            if (charger.currentTelegraph >= 0.0f) {
                charger.currentTelegraph -= dt;
                if (charger.currentTelegraph <= 0.0f) {
                    charger.currentTelegraph = -1.0f;
                    sf::Vector2f diff = playerPos - trans.position;
                    float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                    charger.chargeDirection = (distance > 0.001f) ? diff / distance : sf::Vector2f(1.0f, 0.0f);
                    charger.currentCharge = charger.chargeDuration;
                }
                continue;
            }

            if (charger.currentCooldown > 0.0f) {
                charger.currentCooldown -= dt;
                continue;
            }

            sf::Vector2f diff = playerPos - trans.position;
            float distSq = diff.x * diff.x + diff.y * diff.y;
            if (distSq <= charger.triggerRange * charger.triggerRange) {
                charger.currentTelegraph = charger.telegraphTime;
            }
        }
    }
};
