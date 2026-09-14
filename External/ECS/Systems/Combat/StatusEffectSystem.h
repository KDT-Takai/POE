#pragma once
#include "../../Registry/Registry.h"
#include "Components/Combat/StatusEffects.h"
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "Components/Physics/Velocity/Velocity.h"
#include "Components/VFX/HitFlash.h"
#include <algorithm>

class StatusEffectSystem {
public:
    void Update(Registry& registry, float dt) {
        for (auto entity : registry.View<CharacterStatsComponent>()) {
            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);

            if (registry.HasComponent<HitFlashComponent>(entity)) {
                auto& flash = registry.GetComponent<HitFlashComponent>(entity);
                if (flash.timer > 0.0f) flash.timer -= dt;
            }

            if (stats.maxES > 0.0f) {
                if (stats.esRegenDelay > 0.0f) {
                    stats.esRegenDelay -= dt;
                } else if (stats.currentES < stats.maxES) {
                    stats.currentES += stats.maxES * 0.33f * dt;
                    if (stats.currentES > stats.maxES) stats.currentES = stats.maxES;
                }
            }

            if (stats.pendingLeech > 0.0f && stats.currentHP > 0.0f) {
                float maxDrain = stats.maxHP * stats.leechRateCap * dt;
                float drain = (std::min)(maxDrain, stats.pendingLeech);
                stats.pendingLeech -= drain;
                stats.currentHP = (std::min)(stats.maxHP, stats.currentHP + drain);
            }

            if (!registry.HasComponent<StatusEffectsComponent>(entity)) continue;
            auto& status = registry.GetComponent<StatusEffectsComponent>(entity);

            if (status.igniteRemaining > 0.0f) {
                stats.currentHP -= status.igniteDps * dt;
                status.igniteRemaining -= dt;
            }
            if (status.bleedRemaining > 0.0f) {
                stats.currentHP -= status.bleedDps * dt;
                status.bleedRemaining -= dt;
            }
            for (auto& stack : status.poisonStacks) {
                stats.currentHP -= stack.damagePerSecond * dt;
                stack.remaining -= dt;
            }
            status.poisonStacks.erase(
                std::remove_if(status.poisonStacks.begin(), status.poisonStacks.end(),
                    [](const PoisonStack& p) { return p.remaining <= 0.0f; }),
                status.poisonStacks.end());

            if (stats.currentHP < 0.0f) stats.currentHP = 0.0f;

            if (status.chillRemaining > 0.0f) status.chillRemaining -= dt;
            if (status.freezeRemaining > 0.0f) status.freezeRemaining -= dt;
            if (status.shockRemaining > 0.0f) status.shockRemaining -= dt;
            if (status.stunRemaining > 0.0f) status.stunRemaining -= dt;

            if (!registry.HasComponent<VelocityComponent>(entity)) continue;
            auto& vel = registry.GetComponent<VelocityComponent>(entity);

            if (status.IsHardLocked()) {
                vel.velocity = { 0.0f, 0.0f };
            } else if (status.chillRemaining > 0.0f) {
                vel.velocity *= (1.0f - status.chillSlowPercent);
            }
        }
    }
};
