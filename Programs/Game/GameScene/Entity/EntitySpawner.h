#pragma once
#include <ECS.h>
// プレイヤー用
#include "../ECS/Components/Tags/Player/Player.h"
#include "../ECS/Components/Physics/Transform/Transform.h"
#include "../ECS/Components/Physics/Velocity/Velocity.h"
#include "../ECS/Components/Physics/Gravity/Gravity.h"
#include "../ECS/Components/Physics/Facing/Facing.h"
#include "../ECS/Components/Physics/BoxCollider/BoxCollider.h"
#include "../ECS/Components/Control/PlayerInput/PlayerInput.h"
#include "../ECS/Components/Control/State/State.h"
#include "../ECS/Components/Stats/CharacterStats/CharacterStats.h"
#include "../ECS/Components/Interaction/Interaction.h"
#include "../ECS/Components/PlayerSkill/PlayerSkill.h"
#include "../ECS/Components/Stats/SkillData/Skill.h"

class EntitySpawner {
public:
    static EntityObject CreatePlayer(Registry& registry, float startX, float startY) {
        auto entity = registry.CreateEntityObject();

        entity.AddComponent(PlayerTag{});

        entity.AddComponent(TagComponent{ "Player" });

        // 物理・座標設定
        entity.AddComponent(TransformComponent{ sf::Vector2f(startX, startY), sf::Vector2f(1.f, 1.f),  0.f });

        entity.AddComponent(VelocityComponent{ sf::Vector2f(0.f, 0.f) });
        entity.AddComponent(GravityComponent{ 980.0f });

        // 制御・状態設定
        entity.AddComponent(PlayerInputComponent{});
        entity.AddComponent(StateComponent{ ActorState::Idle, 0.0f });

        //ステータス設定
        CharacterStatsComponent stats;
        stats.name = "Hero";
        stats.moveSpeed = 350.0f;
        stats.maxHP = 100.0f;
        stats.currentHP = stats.maxHP;
        stats.rollSpeed = 800.0f;
        stats.rollDuration = 0.3f;
        stats.rollCooldownMax = 2.0f;
        stats.rollCooldownTimer = 0.0f;
        entity.AddComponent(stats);

        // 描画設定
        float radius = 16.0f;
        CircleComponent circleVis;
        circleVis.radius = radius; // 幅32px相当
        circleVis.color = sf::Color::Blue;
        circleVis.isVisible = true;
        entity.AddComponent(circleVis);

        // 当たり判定
        float colliderSize = 20.0f;
        float offset = (radius * 2.0f - colliderSize) / 2.0f;
        entity.AddComponent(BoxColliderComponent{ colliderSize, colliderSize, offset, offset, false, false });

		// スキル１つ追加
        PlayerSkill skillComp;

        SkillData& spark = skillComp.skills[0];
        spark.name = "Spark";
        spark.level = 1;
        spark.behaviorType = SkillBehaviorType::Spark;
        spark.cooldownTime = 0.3f;
        spark.mpCost = 4.0f;
        spark.damage = 25.0f;
        spark.duration = 3.5f;     
        spark.isValid = true;

        SkillData& Slam = skillComp.skills[1];
        Slam.name = "Thunder Slam";
        Slam.level = 1;
        Slam.behaviorType = SkillBehaviorType::GroundSlam;
        Slam.cooldownTime = 5.0f;
        Slam.mpCost = 35.0f;
        Slam.damage = 120.0f;
        Slam.isValid = true;

        SkillData& Warp = skillComp.skills[2];
        Warp.name = "Lightning Warp";
        Warp.level = 1;
        Warp.behaviorType = SkillBehaviorType::LightningWarp;
        Warp.range = 350.0f;
        Warp.damage = 80.0f;
        Warp.cooldownTime = 8.0f;
        Warp.mpCost = 35;
        Warp.isValid = true;


        SkillData& ball = skillComp.skills[3];
        ball.name = "Lightning ball";
        ball.level = 1;
        ball.behaviorType = SkillBehaviorType::LightningBall;
        ball.damage = 40.0f;
        ball.cooldownTime = 12.0f;
        ball.mpCost = 40;
        ball.isValid = true;

        entity.AddComponent(skillComp);

        return entity;
    }
    // 敵を作成
    static EntityObject CreateEnemy(Registry& registry, sf::Vector2f position) {

        auto entity = registry.CreateEntity();

        registry.AddComponent<TransformComponent>(entity, TransformComponent{ position, {1.f, 1.f} });
        registry.AddComponent<CircleComponent>(entity, CircleComponent{ 20.0f, sf::Color::Red });

        float colliderWidth = 20.0f;
        float colliderHeight = 20.0f;
        float offsetX = (40.0f - colliderWidth) / 2.0f;
        float offsetY = (40.0f - colliderHeight) / 2.0f;

        registry.AddComponent<BoxColliderComponent>(entity, BoxColliderComponent{
            colliderWidth, colliderHeight,
            offsetX, offsetY,
            false, false
            });

        CharacterStatsComponent stats;
        stats.name = "Skeleton";
        stats.maxHP = 50.0f;
        stats.currentHP = 50.0f;
        stats.atk = 5.0f;
        stats.def = 1.0f;
        stats.moveSpeed = 80.0f;

        registry.AddComponent<CharacterStatsComponent>(entity, stats);

        registry.AddComponent<VelocityComponent>(entity, VelocityComponent{ {0.0f, 0.0f} });

        return EntityObject(entity , &registry);
    }
    // 精霊碑作成
    static Entity CreateMonument(Registry& registry, sf::Vector2f position, int spiritID) {
        Entity entity = registry.CreateEntity();

        TransformComponent trans;
        trans.position = position;
        registry.AddComponent(entity, trans);

        InteractableComponent interact;
        interact.type = InteractType::SpiritMonument;
        interact.targetID = spiritID;
        registry.AddComponent(entity, interact);

        return entity;
    }
};