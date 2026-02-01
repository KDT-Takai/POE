#pragma once
#include "../../ECS/Registry/Registry.h"
#include "../../ECS/Components/Stats/CharacterStats/CharacterStats.h"
#include "../../ECS/Components/Physics/BoxCollider/BoxCollider.h"
#include "../../ECS/Components/Physics/Velocity/Velocity.h"
#include "../../ECS/Components/Physics/Transform/Transform.h"
#include "../../ECS/Components/Components.h"

namespace EnemyFactory {

    static void CreateEnemy(Registry& registry, sf::Vector2f position) {
        auto entity = registry.CreateEntity();

        registry.AddComponent<TransformComponent>(entity, TransformComponent{ position, {1.f, 1.f} });
        registry.AddComponent<CircleComponent>(entity, CircleComponent{ 20.0f, sf::Color::Red });
        float colliderSize = 20.0f;
        float offset = (20 * 2.0f - colliderSize) / 2.0f;
        registry.AddComponent(entity, BoxColliderComponent{ colliderSize, colliderSize, offset, offset, false, false });

        CharacterStatsComponent stats;
        stats.name = "Skeleton";
        stats.maxHP = 50.0f;
        stats.currentHP = 50.0f;
        stats.atk = 5.0f;
        stats.def = 5.0f;

        stats.moveSpeed = 80.0f;
        registry.AddComponent<CharacterStatsComponent>(entity, stats);
        registry.AddComponent<BoxColliderComponent>(entity, BoxColliderComponent{ 40.0f, 40.0f });
        registry.AddComponent<VelocityComponent>(entity, VelocityComponent{ {0.0f, 0.0f} });
    }
}