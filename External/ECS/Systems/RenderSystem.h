#pragma once
#include <System/Resource/ResourceManager/ResourceManager.h>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include "../Registry/Registry.h"
#include "../Components/Components.h"
#include "../Components/Physics/BoxCollider/BoxCollider.h"
#include "../Components/Physics/Transform/Transform.h"
#include "../Components/Physics/Projectile/Projectile.h"
#include "../Components/PlayerSkill/SparkVisual.h"

class RenderSystem {
public:
    void Render(Registry& registry, sf::RenderTarget& target) {
        // スプライトの描画
        auto spriteEntities = registry.View<SpriteComponent>();
        struct RenderObject {
            TransformComponent* transform;
            SpriteComponent* sprite;
        };
        std::vector<RenderObject> spriteQueue;
        spriteQueue.reserve(spriteEntities.size());

        for (auto entity : spriteEntities) {
            if (registry.HasComponent<TransformComponent>(entity)) {
                auto& sprite = registry.GetComponent<SpriteComponent>(entity);
                auto& transform = registry.GetComponent<TransformComponent>(entity);
                if (sprite.isVisible) spriteQueue.push_back({ &transform, &sprite });
            }
        }

        std::sort(spriteQueue.begin(), spriteQueue.end(),
            [](const RenderObject& a, const RenderObject& b) {
                return a.sprite->layer < b.sprite->layer;
            });

        for (const auto& obj : spriteQueue) {
            auto tex = ResourceManager::Instance().getTexture(obj.sprite->textureName);
            if (tex) {
                sf::Sprite s(*tex);
                s.setPosition(obj.transform->position);
                s.setRotation(sf::degrees(obj.transform->rotation));
                s.setScale(obj.transform->scale);
                s.setColor(obj.sprite->color);
                if (obj.sprite->textureRect.size.x != 0) s.setTextureRect(obj.sprite->textureRect);
                target.draw(s);
            }
        }

        // 円の描画
        auto circleEntities = registry.View<CircleComponent>();
        for (auto entity : circleEntities) {
            if (registry.HasComponent<TransformComponent>(entity)) {
                auto& circle = registry.GetComponent<CircleComponent>(entity);
                auto& transform = registry.GetComponent<TransformComponent>(entity);

                if (circle.isVisible) {
                    sf::CircleShape shape(circle.radius);
                    shape.setFillColor(circle.color);

                    shape.setOrigin({ circle.radius, circle.radius });

                    shape.setPosition({ transform.position.x + circle.radius, transform.position.y + circle.radius });

                    shape.setRotation(sf::degrees(transform.rotation));
                    shape.setScale(transform.scale);

                    target.draw(shape);
                }
            }
        }
        auto sparkView = registry.View<SparkVisualComponent>();
        for (auto entity : sparkView) {
            auto& sparkVis = registry.GetComponent<SparkVisualComponent>(entity);

            if (!registry.HasComponent<TransformComponent>(entity)) continue;
            auto& transform = registry.GetComponent<TransformComponent>(entity);

            if (sparkVis.style == VisualStyle::Explosion) {

                float radius = 150.0f;

                if (registry.HasComponent<BoxColliderComponent>(entity)) {
                    radius = registry.GetComponent<BoxColliderComponent>(entity).width / 2.0f;
                }

                float alphaRatio = 1.0f;
                if (registry.HasComponent<ProjectileComponent>(entity)) {
                    auto& proj = registry.GetComponent<ProjectileComponent>(entity);
                    if (sparkVis.maxDuration > 0) {
                        alphaRatio = proj.duration / sparkVis.maxDuration;
                        if (alphaRatio < 0) alphaRatio = 0;
                    }
                }

                sf::Color currentColor = sparkVis.color;
                currentColor.a = static_cast<std::uint8_t>(255 * alphaRatio);

                sf::CircleShape circle(radius);
                circle.setOrigin({ radius, radius });
                circle.setPosition(transform.position);
                circle.setFillColor(currentColor);

                // アニメーション（だんだん大きくなる）
                float progress = 1.0f - alphaRatio;
                float scale = 0.1f + progress * 0.9f;
                circle.setScale({ scale, scale });

                target.draw(circle);
            }
            else {
                if (sparkVis.trailHistory.size() < 2) continue;

                sf::VertexArray lines(sf::PrimitiveType::LineStrip);

                for (size_t i = 0; i < sparkVis.trailHistory.size(); ++i) {
                    float ratio = static_cast<float>(i) / (sparkVis.trailHistory.size() - 1);

                    sf::Color fadedColor = sparkVis.color;
                    fadedColor.a = static_cast<std::uint8_t>(255 * (1.0f - ratio * ratio));

                    lines.append(sf::Vertex(sparkVis.trailHistory[i], fadedColor));
                }
                target.draw(lines);
            }
        }
    }
    // 当たり判定
    void RenderDebug(Registry& registry, sf::RenderTarget& target)
    {
        auto view = registry.View<BoxColliderComponent>();

        for (auto entity : view)
        {
            if (!registry.HasComponent<TransformComponent>(entity)) continue;

            const auto& trans = registry.GetComponent<TransformComponent>(entity);
            const auto& col = registry.GetComponent<BoxColliderComponent>(entity);

            sf::RectangleShape rect;

            rect.setSize({ col.width, col.height });

            float left = std::floor(trans.position.x + col.offsetX);
            float top = std::floor(trans.position.y + col.offsetY);

            rect.setPosition({ left, top });

            rect.setFillColor(sf::Color::Transparent); // 中身は透明
            rect.setOutlineColor(sf::Color::Red);      // 赤枠
            rect.setOutlineThickness(-3.0f);           // 内側に1pxの線（サイズが変わらない）

            target.draw(rect);
        }
    }
};