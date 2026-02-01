#pragma once
#include <SFML/Graphics.hpp>
#include "../../Registry/Registry.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/PlayerSkill/SparkVisual.h"

class SparkRenderSystem {
public:
    void Render(Registry& registry, sf::RenderTarget& target) {
        auto view = registry.View<SparkVisualComponent>();
        for (auto entity : view) {
            if (!registry.HasComponent<TransformComponent>(entity)) continue;
            auto& transform = registry.GetComponent<TransformComponent>(entity);
            auto& spark = registry.GetComponent<SparkVisualComponent>(entity);

            // õ–½‚Ìæ“¾
            float alphaRatio = 1.0f;
            if (registry.HasComponent<ProjectileComponent>(entity)) {
                auto& proj = registry.GetComponent<ProjectileComponent>(entity);
                if (spark.maxDuration > 0) {
                    alphaRatio = proj.duration / spark.maxDuration;
                    if (alphaRatio < 0) alphaRatio = 0;
                }
            }

            sf::Color currentColor = spark.color;
            currentColor.a = static_cast<std::uint8_t>(255 * alphaRatio);

            if (spark.style == VisualStyle::Explosion) {
                float radius = 20.0f;

                if (registry.HasComponent<BoxColliderComponent>(entity)) {
                    // •‚Ì”¼•ª‚ğ”¼Œa‚É‚·‚é (’¼Œa = width)
                    radius = registry.GetComponent<BoxColliderComponent>(entity).width / 2.0f;
                }

                sf::CircleShape circle(radius);
                circle.setOrigin({ radius, radius });
                circle.setPosition(transform.position);
                circle.setFillColor(currentColor);

                float progress = 1.0f - alphaRatio;

                // šC³: 0.1”{‚©‚çn‚Ü‚èAÅ‘å‚Å‚à 1.0”{(Collider‚Æ“¯‚¶‘å‚«‚³) ‚Å~‚ß‚é
                float scale = 0.1f + progress * 0.9f;

                circle.setScale({ scale, scale });

                target.draw(circle);
            }
            else {
                if (spark.trailHistory.empty()) continue;

                sf::Vector2f start = spark.trailHistory[0];
                sf::Vector2f end = transform.position;
                sf::Vector2f diff = end - start;
                float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                if (length < 1.0f) continue;
                sf::Vector2f dir = diff / length;
                sf::Vector2f normal(-dir.y, dir.x);
                // ‘¾‚³‚Ì”¼•ª
                float halfThick = spark.thickness * 0.5f;
                sf::Vector2f offset = normal * halfThick;
                sf::Vertex vertices[4];
                // n“_‘¤
                vertices[0].position = start + offset;
                vertices[1].position = start - offset;
                // I“_‘¤
                vertices[2].position = end - offset;
                vertices[3].position = end + offset;
                // Fİ’è
                for (int i = 0; i < 4; ++i) vertices[i].color = currentColor;

                target.draw(vertices, 4, sf::PrimitiveType::TriangleStrip);
            }
        }
    }
};