#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Physics/BoxCollider/BoxCollider.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Physics/Projectile/Projectile.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include <vector>

class CollisionSystem {
public:
    void Update(Registry& registry) {
        auto projectiles = registry.View<ProjectileComponent, BoxColliderComponent, TransformComponent>();
        auto enemies = registry.View<CharacterStatsComponent, BoxColliderComponent, TransformComponent>();

        // 削除・死亡リスト
        std::vector<unsigned int> deadEntities;
        std::vector<unsigned int> destroyedProjectiles;

        for (auto projEntity : projectiles) {

            auto& proj = registry.GetComponent<ProjectileComponent>(projEntity);
            auto& pTrans = registry.GetComponent<TransformComponent>(projEntity);
            auto& pCol = registry.GetComponent<BoxColliderComponent>(projEntity);
            sf::FloatRect bulletRect = GetBounds(pTrans.position, pCol);

            bool hitSomething = false;

            for (auto enemyEntity : enemies) {
                if (registry.HasComponent<PlayerInputComponent>(enemyEntity)) continue;

                auto& eTrans = registry.GetComponent<TransformComponent>(enemyEntity);
                auto& eCol = registry.GetComponent<BoxColliderComponent>(enemyEntity);
                sf::FloatRect enemyRect = GetBounds(eTrans.position, eCol);

                if (bulletRect.findIntersection(enemyRect)) {
                    auto& stats = registry.GetComponent<CharacterStatsComponent>(enemyEntity);
                    stats.currentHP -= proj.damage;

                    if (stats.currentHP <= 0) {
                        deadEntities.push_back(enemyEntity);
                    }
                    if (!proj.isBouncy) {
                        hitSomething = true;
                        break;
                    }
                }
            }

            if (hitSomething) {
                destroyedProjectiles.push_back(projEntity);
            }
        }

        // プレイヤーへのダメージ判定
        unsigned int playerEntity = -1;
        bool playerFound = false;

        for (auto entity : registry.View<PlayerInputComponent>()) {
            playerEntity = entity;
            playerFound = true;
            break;
        }

        if (playerFound && registry.HasComponent<BoxColliderComponent>(playerEntity)) {
            auto& pTrans = registry.GetComponent<TransformComponent>(playerEntity);
            auto& pCol = registry.GetComponent<BoxColliderComponent>(playerEntity);
            auto& pStats = registry.GetComponent<CharacterStatsComponent>(playerEntity);
            sf::FloatRect playerRect = GetBounds(pTrans.position, pCol);

            for (auto enemyEntity : enemies) {
                if (enemyEntity == playerEntity) continue;

                auto& eTrans = registry.GetComponent<TransformComponent>(enemyEntity);
                auto& eCol = registry.GetComponent<BoxColliderComponent>(enemyEntity);
                sf::FloatRect enemyRect = GetBounds(eTrans.position, eCol);

                if (playerRect.findIntersection(enemyRect)) {
                    auto& eStats = registry.GetComponent<CharacterStatsComponent>(enemyEntity);

                    pStats.currentHP -= eStats.atk * 0.1f;
                    if (pStats.currentHP < 0) pStats.currentHP = 0;
                }
            }
        }

        for (auto e : destroyedProjectiles) {
            if (registry.IsValid(e)) registry.DestroyEntity(e);
        }
        for (auto e : deadEntities) {
            if (registry.IsValid(e)) registry.DestroyEntity(e);
        }
    }

private:
    sf::FloatRect GetBounds(const sf::Vector2f& pos, const BoxColliderComponent& col) {
        return sf::FloatRect(
            { pos.x + col.offsetX, pos.y + col.offsetY }, // 位置ベクトル
            { col.width, col.height }                     // サイズベクトル
        );
    }
};