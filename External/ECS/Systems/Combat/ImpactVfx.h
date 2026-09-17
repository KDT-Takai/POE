#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Physics/Velocity/Velocity.h"
#include "../../Components/Physics/Projectile/Projectile.h"
#include "../../Components/PlayerSkill/SparkVisual.h"
#include "../../Components/Combat/DamageType.h"

// Small on-hit VFX burst ("プレイヤーと敵の攻撃をもっと派手に" -- every landed hit gets a
// brief expanding flash at the impact point, on top of the existing HitFlashComponent
// white-tint on the entity itself), reusing the same "0-damage timed projectile +
// SparkVisualComponent" pattern CollisionSystem::SpawnDeathBurst already established for
// death explosions. Crits get a bigger/brighter/longer burst so they read as more
// impactful than a normal hit.
namespace ImpactVfx {
    inline sf::Color ElementColor(DamageElement element) {
        switch (element) {
        case DamageElement::Fire: return sf::Color(255, 140, 40);
        case DamageElement::Cold: return sf::Color(140, 220, 255);
        case DamageElement::Lightning: return sf::Color(255, 240, 100);
        case DamageElement::Chaos: return sf::Color(200, 90, 230);
        default: return sf::Color(240, 240, 240); // Physical
        }
    }

    inline void SpawnHitBurst(Registry& registry, sf::Vector2f pos, sf::Color color, bool isCrit) {
        auto burst = registry.CreateEntityObject();
        burst.AddComponent(TransformComponent{ pos, {1.f, 1.f}, 0.f });
        burst.AddComponent(VelocityComponent{ {0.f, 0.f} });

        ProjectileComponent timerProj;
        timerProj.duration = isCrit ? 0.3f : 0.18f;
        timerProj.damage = 0.0f;
        burst.AddComponent(timerProj);

        SparkVisualComponent sparkVis;
        sparkVis.style = VisualStyle::Explosion;
        sparkVis.maxDuration = timerProj.duration;
        sparkVis.color = isCrit ? sf::Color(255, 255, 255) : color;
        sparkVis.explosionRadius = isCrit ? 36.0f : 20.0f;
        burst.AddComponent(sparkVis);
    }
}
