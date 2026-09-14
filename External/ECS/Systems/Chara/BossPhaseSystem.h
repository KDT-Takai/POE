#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Tags/Boss/BossPhase.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Components.h"
#include <System/CameraManager/CameraManager.h>
#include <string>

class BossPhaseSystem {
public:
    std::string enrageMessage;
    float enrageMessageTimer = 0.0f;

    void Update(Registry& registry, float dt) {
        if (enrageMessageTimer > 0.0f) enrageMessageTimer -= dt;

        for (auto e : registry.View<BossPhaseComponent, CharacterStatsComponent>()) {
            auto& phase = registry.GetComponent<BossPhaseComponent>(e);
            auto& stats = registry.GetComponent<CharacterStatsComponent>(e);

            if (!phase.hasEnraged && stats.currentHP > 0.0f && stats.currentHP <= stats.maxHP * phase.enrageThreshold) {
                phase.hasEnraged = true;
                stats.atk *= 1.6f;
                stats.moveSpeed *= 1.35f;

                if (registry.HasComponent<CircleComponent>(e)) {
                    registry.GetComponent<CircleComponent>(e).color = sf::Color(255, 30, 30);
                }

                enrageMessage = stats.name + " has enraged!";
                enrageMessageTimer = 3.0f;
                CameraManager::Instance().Shake(18.0f, 0.5f);
            }
        }
    }
};
