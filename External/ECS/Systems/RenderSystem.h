#pragma once
#include <System/Resource/ResourceManager/ResourceManager.h>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include "../Registry/Registry.h"
#include "../Components/Components.h"
#include "../Components/Physics/BoxCollider/BoxCollider.h"
#include "../Components/Physics/Transform/Transform.h" // 座標

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
        // 画像がないオブジェクトも表示できるようにする
        auto circleEntities = registry.View<CircleComponent>();

        for (auto entity : circleEntities) {
            if (registry.HasComponent<TransformComponent>(entity)) {
                auto& circle = registry.GetComponent<CircleComponent>(entity);
                auto& transform = registry.GetComponent<TransformComponent>(entity);

                if (circle.isVisible) {
                    sf::CircleShape shape(circle.radius);
                    shape.setFillColor(circle.color);

                    // 中心を原点にする
                    shape.setOrigin({ circle.radius, circle.radius });

                    shape.setPosition(transform.position);
                    shape.setRotation(sf::degrees(transform.rotation));
                    shape.setScale(transform.scale);

                    target.draw(shape);
                }
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
//#pragma once
//#include "ECS.h"
//#include "Components.h"
//#include "../../../Programs/System/Resource/ResourceManager/ResourceManager.h"
//
//class RenderSystem {
//public:
//    void Update(Registry& registry, sf::RenderWindow& window) {
//        // 1. 描画対象を収集
//        auto entities = registry.View<SpriteComponent>();
//
//        struct RenderObject {
//            TransformComponent* transform;
//            SpriteComponent* sprite;
//        };
//
//        std::vector<RenderObject> renderQueue;
//        renderQueue.reserve(entities.size()); // メモリ確保の最適化
//
//        for (auto entity : entities) {
//            // TransformとSpriteの両方を持っているか確認
//            if (registry.HasComponent<TransformComponent>(entity)) {
//                auto& sprite = registry.GetComponent<SpriteComponent>(entity);
//                auto& transform = registry.GetComponent<TransformComponent>(entity);
//
//                if (sprite.isVisible) {
//                    renderQueue.push_back({ &transform, &sprite });
//                }
//            }
//        }
//
//        // 2. レイヤー順にソート (値が小さい方が奥、大きい方が手前)
//        std::sort(renderQueue.begin(), renderQueue.end(),
//            [](const RenderObject& a, const RenderObject& b) {
//                return a.sprite->layer < b.sprite->layer;
//            });
//
//        // 3. 描画
//        for (const auto& obj : renderQueue) {
//            // リソースマネージャーからテクスチャを取得
//            auto tex = ResourceManager::Instance().getTexture(obj.sprite->textureName);
//
//            if (tex) {
//                // 【修正点】SFML 3.0.2対応: コンストラクタでテクスチャ(*tex)を渡す
//                sf::Sprite s(*tex);
//
//                // 各種パラメータをセット
//                s.setPosition(obj.transform->position);
//                s.setRotation(sf::degrees(obj.transform->rotation)); // SFML3では角度は sf::degrees() 推奨
//                s.setScale(obj.transform->scale);
//                s.setColor(obj.sprite->color);
//
//                // TextureRectが設定されていれば適用 (幅が0以外なら設定アリとみなす簡易判定)
//                if (obj.sprite->textureRect.size.x != 0) {
//                    s.setTextureRect(obj.sprite->textureRect);
//                }
//
//                window.draw(s);
//            }
//        }
//    }
//};