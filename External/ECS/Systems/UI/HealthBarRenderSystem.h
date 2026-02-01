#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Physics/BoxCollider/BoxCollider.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include <SFML/Graphics.hpp>

class HealthBarRenderSystem {
public:
    void Render(Registry& registry, sf::RenderTarget& target) {
        auto view = registry.View<TransformComponent, CharacterStatsComponent, BoxColliderComponent>();

        const float barWidth = 32.0f;
        const float barHeight = 4.0f;
        const float yOffset = 10.0f;

        sf::RectangleShape backgroundRect({ barWidth, barHeight });
        backgroundRect.setFillColor(sf::Color(20, 20, 20));
        backgroundRect.setOutlineColor(sf::Color::Black);
        backgroundRect.setOutlineThickness(1.0f);

        sf::RectangleShape hpRect({ barWidth, barHeight });
        hpRect.setFillColor(sf::Color(220, 40, 40));

        for (auto entity : view) {
            if (registry.HasComponent<PlayerInputComponent>(entity)) continue;

            auto& trans = registry.GetComponent<TransformComponent>(entity);
            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);
            auto& col = registry.GetComponent<BoxColliderComponent>(entity);

            if (stats.currentHP >= stats.maxHP) continue;

            float ratio = (float)stats.currentHP / (float)stats.maxHP;
            if (ratio < 0) ratio = 0;

            float centerX = trans.position.x + col.offsetX + (col.width / 2.0f);
            float barX = centerX - (barWidth / 2.0f);
            float barY = trans.position.y + col.offsetY - yOffset;

            // ”wŒi
            backgroundRect.setPosition({ barX, barY });
            target.draw(backgroundRect);

            // Ôƒo[
            hpRect.setSize({ barWidth * ratio, barHeight });
            hpRect.setPosition({ barX, barY });
            target.draw(hpRect);
        }
    }
};