#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Chara/RangedAttacker.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Physics/Velocity/Velocity.h"
#include "../../Components/Physics/BoxCollider/BoxCollider.h"
#include "../../Components/Physics/Projectile/Projectile.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "../../Components/PlayerSkill/SparkVisual.h"
#include "../Combat/ImpactVfx.h"
#include <cmath>

// Fires a projectile at the player for every monster with a RangedAttackerComponent
// (see EnemyAISystem for the matching kite/hold-distance movement behavior).
class EnemyRangedAttackSystem {
public:
    void Update(Registry& registry, float dt, sf::Vector2f playerPos) {
        for (auto entity : registry.View<RangedAttackerComponent, CharacterStatsComponent, TransformComponent>()) {
            if (registry.HasComponent<PlayerInputComponent>(entity)) continue;

            auto& ranged = registry.GetComponent<RangedAttackerComponent>(entity);
            if (ranged.currentCooldown > 0.0f) {
                ranged.currentCooldown -= dt;
                continue;
            }

            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);
            if (stats.currentHP <= 0.0f) continue;

            auto& trans = registry.GetComponent<TransformComponent>(entity);
            sf::Vector2f diff = playerPos - trans.position;
            float distanceSq = diff.x * diff.x + diff.y * diff.y;
            if (distanceSq > ranged.attackRange * ranged.attackRange) continue;

            float distance = std::sqrt(distanceSq);
            if (distance < 0.001f) continue;
            sf::Vector2f direction = diff / distance;

            auto bolt = registry.CreateEntity();
            registry.AddComponent<TransformComponent>(bolt, TransformComponent{ trans.position, {1.0f, 1.0f} });
            registry.AddComponent<VelocityComponent>(bolt, VelocityComponent{ direction * ranged.projectileSpeed });
            registry.AddComponent<BoxColliderComponent>(bolt, BoxColliderComponent{ 10.0f, 10.0f, -5.0f, -5.0f, false, false });

            ProjectileComponent proj;
            proj.duration = 3.0f;
            proj.damage = stats.atk * (ranged.damagePercent / 100.0f);
            proj.isBouncy = false;
            proj.isEnemy = true;
            proj.ownerEntity = entity;
            proj.damageType = stats.contactDamageType;
            registry.AddComponent<ProjectileComponent>(bolt, proj);

            // 弾自体に見た目を持たせる(プレイヤースキルの投射物と同じSparkVisualComponent
            // 経路、以前は矢/魔弾が完全に不可視だった)。属性色は着弾VFXと同じ
            // ImpactVfx::ElementColorを再利用し、雷は稲妻描画(electric)にする。
            SparkVisualComponent sparkVis;
            sparkVis.trailHistory.push_back(trans.position);
            sparkVis.color = ImpactVfx::ElementColor(proj.damageType);
            sparkVis.thickness = 3.0f;
            sparkVis.electric = (proj.damageType == DamageElement::Lightning);
            sparkVis.seed = static_cast<float>(entity) * 37.0f + proj.duration;
            registry.AddComponent<SparkVisualComponent>(bolt, sparkVis);

            ranged.currentCooldown = ranged.cooldownTime;
        }
    }
};
