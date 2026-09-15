#pragma once
#include <ECS.h>
// �v���C���[�p
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
#include "../ECS/Components/Combat/StatusEffects.h"
#include "../ECS/Components/Item/Equipment.h"
#include "../ECS/Components/Item/Inventory.h"

class EntitySpawner {
public:
    static EntityObject CreatePlayer(Registry& registry, float startX, float startY) {
        auto entity = registry.CreateEntityObject();

        entity.AddComponent(PlayerTag{});

        entity.AddComponent(TagComponent{ "Player" });

        // �����E���W�ݒ�
        entity.AddComponent(TransformComponent{ sf::Vector2f(startX, startY), sf::Vector2f(1.f, 1.f),  0.f });

        entity.AddComponent(VelocityComponent{ sf::Vector2f(0.f, 0.f) });
        entity.AddComponent(GravityComponent{ 980.0f });

        // ����E��Ԑݒ�
        entity.AddComponent(PlayerInputComponent{});
        entity.AddComponent(StateComponent{ ActorState::Idle, 0.0f });

        //�X�e�[�^�X�ݒ�
        CharacterStatsComponent stats;
        stats.name = "Hero";
        stats.moveSpeed = 350.0f;
        stats.maxHP = 100.0f;
        stats.currentHP = stats.maxHP;
        stats.rollSpeed = 800.0f;
        stats.rollDuration = 0.3f;
        stats.rollCooldownMax = 2.0f;
        stats.rollCooldownTimer = 0.0f;
        stats.evasion = 200.0f;
        stats.armour = 50.0f;
        stats.accuracy = 200.0f;
        stats.maxES = 40.0f;
        stats.currentES = stats.maxES;
        stats.leechPercent = 0.05f;
        entity.AddComponent(stats);
        entity.AddComponent(StatusEffectsComponent{});

        EquipmentComponent equipment;
        equipment.baseStats = stats;
        entity.AddComponent(equipment);
        entity.AddComponent(InventoryComponent{});

        // �`��ݒ�
        float radius = 16.0f;
        CircleComponent circleVis;
        circleVis.radius = radius; // ��32px����
        circleVis.color = sf::Color::Blue;
        circleVis.isVisible = true;
        entity.AddComponent(circleVis);

        // �����蔻��
        float colliderSize = 20.0f;
        float offset = (radius * 2.0f - colliderSize) / 2.0f;
        entity.AddComponent(BoxColliderComponent{ colliderSize, colliderSize, offset, offset, false, false });

		// �X�L���P�ǉ�
        PlayerSkill skillComp;

        SkillData& spark = skillComp.skills[0];
        spark.name = "Spark";
        spark.level = 1;
        spark.behaviorType = SkillBehaviorType::Spark;
        spark.cooldownTime = 0.3f;
        spark.mpCost = 4.0f;
        spark.damage = 25.0f;
        spark.duration = 3.5f;
        spark.element = DamageElement::Lightning;
        spark.isValid = true;

        SkillData& Slam = skillComp.skills[1];
        Slam.name = "Thunder Slam";
        Slam.level = 1;
        Slam.behaviorType = SkillBehaviorType::GroundSlam;
        Slam.cooldownTime = 5.0f;
        Slam.mpCost = 35.0f;
        Slam.damage = 120.0f;
        Slam.element = DamageElement::Physical;
        Slam.isValid = true;

        SkillData& Warp = skillComp.skills[2];
        Warp.name = "Lightning Warp";
        Warp.level = 1;
        Warp.behaviorType = SkillBehaviorType::LightningWarp;
        Warp.range = 350.0f;
        Warp.damage = 80.0f;
        Warp.cooldownTime = 8.0f;
        Warp.mpCost = 35;
        Warp.element = DamageElement::Lightning;
        Warp.isValid = true;


        SkillData& ball = skillComp.skills[3];
        ball.name = "Lightning ball";
        ball.level = 1;
        ball.behaviorType = SkillBehaviorType::LightningBall;
        ball.damage = 40.0f;
        ball.cooldownTime = 12.0f;
        ball.mpCost = 40;
        ball.element = DamageElement::Lightning;
        ball.isValid = true;

        entity.AddComponent(skillComp);

        return entity;
    }
    // �G���쐬
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
        stats.evasion = 30.0f;
        stats.armour = 10.0f;
        stats.accuracy = 80.0f;

        registry.AddComponent<CharacterStatsComponent>(entity, stats);
        registry.AddComponent<StatusEffectsComponent>(entity, StatusEffectsComponent{});

        registry.AddComponent<VelocityComponent>(entity, VelocityComponent{ {0.0f, 0.0f} });

        return EntityObject(entity , &registry);
    }
    // �����쐬
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