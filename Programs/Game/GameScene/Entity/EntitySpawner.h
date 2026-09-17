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
#include "../ECS/Components/Item/Stash.h"
#include "../ECS/Components/Progression/PassiveTree.h"
#include "../ECS/Components/Progression/Atlas.h"
#include "../ECS/Components/Item/SkillGem.h"
#include "../ECS/Components/Chara/Minion.h"
#include "../ECS/Systems/Skill/SkillGemScaling.h"
#include "../ECS/Systems/Item/ItemFactory.h"
#include <random>

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
        stats.maxSpirit = 100.0f;
        stats.currentSpirit = 0.0f; // amount reserved by equipped Aura gems, not "remaining"
        entity.AddComponent(stats);
        entity.AddComponent(StatusEffectsComponent{});

        EquipmentComponent equipment;
        equipment.baseStats = stats;
        entity.AddComponent(equipment);
        entity.AddComponent(InventoryComponent{});
        entity.AddComponent(StashComponent{});
        entity.AddComponent(PassiveTreeComponent{});
        entity.AddComponent(AtlasComponent{});

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
        // Starting loadout is derived from the shared SkillGemData catalog (previously
        // hand-duplicated field-for-field here, which could silently drift from the
        // catalog -- see AI/DECISIONS.md). All start at gem level 1 / 2 sockets.
        // All start at gem level 1 / 2 sockets, equipped directly into PlayerSkill (not
        // sitting in the bag) since these 4 slots are always full at character creation.
        PlayerSkill skillComp;
        const int starterGemIds[4] = { 0, 1, 2, 3 };
        for (size_t i = 0; i < 4; ++i) {
            const GemDefinition* def = SkillGemData::Find(starterGemIds[i]);
            if (!def) continue;
            ItemComponent gemItem;
            gemItem.category = ItemCategory::SkillGem;
            gemItem.skillGemIdentified = true;
            gemItem.skillGemId = starterGemIds[i];
            gemItem.skillGemLevel = 1;
            gemItem.skillGemMaxSockets = 2;
            gemItem.baseName = def->skill.name;
            skillComp.equippedItems[i] = gemItem;
            SkillGemScaling::BuildEquippedSkillData(skillComp.skills[i], gemItem);
        }
        entity.AddComponent(skillComp);
        entity.AddComponent(SpiritGemLoadoutComponent{});

        // Starting Tier 1 Waystone so the endgame is reachable right away (real PoE2
        // grants this as a campaign-completion quest reward; we simplify by just
        // starting new characters with one). Waystones are a normal inventory item
        // (ItemCategory::Waystone, see Item.h), so this just places one in the empty
        // starting bag rather than needing a separate "waystone count" component.
        {
            std::random_device rd;
            std::mt19937 rng(rd());
            ItemComponent starterWaystone = ItemFactory::GenerateWaystone(1, rng);
            starterWaystone.gridCol = 0;
            starterWaystone.gridRow = 0;
            entity.GetComponent<InventoryComponent>().items.push_back(starterWaystone);
        }

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

    // Permanent Minion (see MinionSystem for AI/respawn, SpiritAuraSystem for the Spirit-
    // reservation-gated spawn/despawn trigger). ownerAtk is passed in rather than read from
    // the owner here so this stays a pure "given these numbers, build an entity" factory
    // like CreateEnemy/CreateMonument; damage type comes from the gem itself (skill.element),
    // matching how any other skill gem's damage type is its own, not the caster's.
    static EntityObject CreateMinion(Registry& registry, sf::Vector2f position, Entity owner, int spiritSlotIndex,
        int gemId, int level, float ownerAtk) {

        auto entity = registry.CreateEntityObject();

        entity.AddComponent(TransformComponent{ position, {1.f, 1.f}, 0.f });
        entity.AddComponent(CircleComponent{ 18.0f, sf::Color(80, 220, 220), true });

        float colliderSize = 20.0f;
        float offset = (36.0f - colliderSize) / 2.0f;
        entity.AddComponent(BoxColliderComponent{ colliderSize, colliderSize, offset, offset, false, false });

        const GemDefinition* def = SkillGemData::Find(gemId);

        CharacterStatsComponent stats;
        stats.name = def ? def->skill.name : "Minion";
        stats.maxHP = def ? SkillGemScaling::ScaledMinionHp(def->skill.minionMaxHp, level) : 50.0f;
        stats.currentHP = stats.maxHP;
        stats.atk = def ? ownerAtk * (def->skill.damage / 100.0f) : 0.0f;
        stats.moveSpeed = 220.0f;
        stats.evasion = 30.0f;
        stats.armour = 10.0f;
        stats.accuracy = 100.0f;
        stats.contactDamageType = def ? def->skill.element : DamageElement::Physical;
        entity.AddComponent(stats);

        entity.AddComponent(VelocityComponent{ {0.0f, 0.0f} });
        entity.AddComponent(AllyTagComponent{});
        entity.AddComponent(PermanentMinionComponent{ owner, spiritSlotIndex, gemId, 0.0f });

        return entity;
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