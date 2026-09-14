#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Physics/BoxCollider/BoxCollider.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Physics/Projectile/Projectile.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "../../Components/Combat/StatusEffects.h"
#include "../../Components/Item/ItemPickup.h"
#include "../../Components/Item/Currency.h"
#include "../../Components/VFX/HitFlash.h"
#include "../../Components/PlayerSkill/SparkVisual.h"
#include <System/CameraManager/CameraManager.h>
#include "../Combat/CombatMath.h"
#include "../Item/ItemFactory.h"
#include "../Item/EquipmentSystem.h"
#include "../Progression/LevelSystem.h"
#include "../Progression/PassiveSystem.h"
#include <vector>
#include <random>
#include <algorithm>

class CollisionSystem {
public:
    std::string levelUpMessage;
    float levelUpMessageTimer = 0.0f;

    void Update(Registry& registry, float dt) {
        if (levelUpMessageTimer > 0.0f) levelUpMessageTimer -= dt;

        auto projectiles = registry.View<ProjectileComponent, BoxColliderComponent, TransformComponent>();
        auto enemies = registry.View<CharacterStatsComponent, BoxColliderComponent, TransformComponent>();

        // �폜�E���S���X�g
        std::vector<unsigned int> deadEntities;
        std::vector<unsigned int> destroyedProjectiles;
		// ���˕��ƓG�̏Փ˔���
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

                    // �܂�����ł��Ȃ��ꍇ�̂݃_���[�W��^����
                    if (stats.currentHP > 0) {
                        bool ownerValid = registry.IsValid(proj.ownerEntity) && registry.HasComponent<CharacterStatsComponent>(proj.ownerEntity);
                        bool isCrit = ownerValid && CombatMath::RollCrit(registry.GetComponent<CharacterStatsComponent>(proj.ownerEntity).critRate);
                        float critMult = ownerValid ? registry.GetComponent<CharacterStatsComponent>(proj.ownerEntity).critDamage : 1.0f;

                        float rawDamage = proj.damage * (isCrit ? critMult : 1.0f);
                        if (registry.HasComponent<StatusEffectsComponent>(enemyEntity)) {
                            auto& targetStatus = registry.GetComponent<StatusEffectsComponent>(enemyEntity);
                            if (targetStatus.shockRemaining > 0.0f) rawDamage *= (1.0f + targetStatus.shockIncreasedDamageTaken);
                        }

                        float dealt = CombatMath::ApplyDamage(stats, rawDamage, proj.damageType);

                        if (dealt > 0.0f) {
                            registry.AddComponent(enemyEntity, HitFlashComponent{ 0.08f });
                        }

                        if (registry.HasComponent<StatusEffectsComponent>(enemyEntity)) {
                            CombatMath::ApplyAilmentOnHit(registry.GetComponent<StatusEffectsComponent>(enemyEntity), proj.damageType, dealt, stats.maxHP, isCrit);
                        }

                        if (ownerValid) {
                            auto& ownerStats = registry.GetComponent<CharacterStatsComponent>(proj.ownerEntity);
                            if (ownerStats.leechPercent > 0.0f) {
                                ownerStats.pendingLeech += dealt * ownerStats.leechPercent;
                            }
                        }

                        // �|�����u�Ԃ̏���
                        if (stats.currentHP <= 0) {
                            deadEntities.push_back(enemyEntity);

                            if (ownerValid && registry.HasComponent<PlayerInputComponent>(proj.ownerEntity) && registry.HasComponent<EquipmentComponent>(proj.ownerEntity)) {
                                auto& ownerStats = registry.GetComponent<CharacterStatsComponent>(proj.ownerEntity);
                                auto& ownerEquip = registry.GetComponent<EquipmentComponent>(proj.ownerEntity);
                                std::string levelMsg;
                                if (LevelSystem::GrantXP(ownerStats, ownerEquip, LevelSystem::CalcKillXP(stats), levelMsg)) {
                                    static std::random_device rdP;
                                    static std::mt19937 rngP(rdP());
                                    std::string passiveMsg = PassiveSystem::ApplyRandomPassive(ownerEquip, rngP);
                                    EquipmentSystem::RecalculateStats(ownerStats, ownerEquip);
                                    ownerStats.currentHP = ownerStats.maxHP;
                                    ownerStats.currentMP = ownerStats.maxMP;
                                    levelUpMessage = levelMsg + "  " + passiveMsg;
                                    levelUpMessageTimer = 2.5f;
                                }
                            }

                            for (auto playerEnt : registry.View<PlayerInputComponent, CharacterStatsComponent, PlayerSkill>()) {

                                if (proj.type == SkillBehaviorType::LightningWarp) {
                                    auto& pStats = registry.GetComponent<CharacterStatsComponent>(playerEnt);
                                    auto& pSkill = registry.GetComponent<PlayerSkill>(playerEnt);
                                    // �}�i�S�� & �N�[���^�C�����Z�b�g
                                    pStats.currentMP = pStats.maxMP;
                                    pSkill.skills[2].currentCooldown = 0.0f;
                                }
                            }
                        }
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

        // �v���C���[�ւ̃_���[�W����
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

            if (pStats.hitInvincibilityTimer > 0.0f) {
                pStats.hitInvincibilityTimer -= dt;
            }

            for (auto e : enemies) {
                if (e == playerEntity) continue;
                if (pStats.hitInvincibilityTimer > 0.0f) break;

                auto& eTrans = registry.GetComponent<TransformComponent>(e);
                auto& eCol = registry.GetComponent<BoxColliderComponent>(e);
                sf::FloatRect enemyRect = GetBounds(eTrans.position, eCol);

                if (playerRect.findIntersection(enemyRect)) {
                    auto& eStats = registry.GetComponent<CharacterStatsComponent>(e);

                    if (!CombatMath::RollHit(eStats.accuracy, pStats.evasion)) continue;

                    bool isCrit = CombatMath::RollCrit(eStats.critRate);
                    float rawDamage = eStats.atk * (isCrit ? eStats.critDamage : 1.0f);

                    bool hasStatus = registry.HasComponent<StatusEffectsComponent>(playerEntity);
                    if (hasStatus) {
                        auto& pStatus = registry.GetComponent<StatusEffectsComponent>(playerEntity);
                        if (pStatus.shockRemaining > 0.0f) rawDamage *= (1.0f + pStatus.shockIncreasedDamageTaken);
                    }

                    float dealt = CombatMath::ApplyDamage(pStats, rawDamage, eStats.contactDamageType);

                    if (dealt > 0.0f) {
                        registry.AddComponent(playerEntity, HitFlashComponent{ 0.08f });
                        float shakeStrength = std::clamp(dealt / pStats.maxHP, 0.0f, 1.0f) * 12.0f;
                        CameraManager::Instance().Shake(shakeStrength, 0.2f);
                    }

                    if (hasStatus) {
                        CombatMath::ApplyAilmentOnHit(registry.GetComponent<StatusEffectsComponent>(playerEntity), eStats.contactDamageType, dealt, pStats.maxHP, isCrit);
                    }

                    pStats.hitInvincibilityTimer = 0.6f;
                }
            }
        }

        for (auto e : destroyedProjectiles) {
            if (registry.IsValid(e)) registry.DestroyEntity(e);
        }
        for (auto e : deadEntities) {
            if (registry.IsValid(e)) {
                SpawnDeathBurst(registry, e);
                TrySpawnItemDrop(registry, e);
                registry.DestroyEntity(e);
            }
        }
    }

private:
    void SpawnDeathBurst(Registry& registry, Entity deadEntity) {
        if (!registry.HasComponent<TransformComponent>(deadEntity)) return;
        auto& trans = registry.GetComponent<TransformComponent>(deadEntity);

        sf::Color burstColor = sf::Color(220, 220, 220);
        if (registry.HasComponent<CircleComponent>(deadEntity)) {
            burstColor = registry.GetComponent<CircleComponent>(deadEntity).color;
        }

        auto burst = registry.CreateEntityObject();
        burst.AddComponent(TransformComponent{ trans.position, {1.f, 1.f}, 0.f });
        burst.AddComponent(VelocityComponent{ {0.f, 0.f} });

        ProjectileComponent timerProj;
        timerProj.duration = 0.35f;
        timerProj.damage = 0.0f;
        burst.AddComponent(timerProj);

        SparkVisualComponent sparkVis;
        sparkVis.style = VisualStyle::Explosion;
        sparkVis.maxDuration = 0.35f;
        sparkVis.color = burstColor;
        sparkVis.explosionRadius = 26.0f;
        burst.AddComponent(sparkVis);
    }


    void TrySpawnItemDrop(Registry& registry, Entity deadEntity) {
        if (!registry.HasComponent<CharacterStatsComponent>(deadEntity) || !registry.HasComponent<TransformComponent>(deadEntity)) return;

        auto& stats = registry.GetComponent<CharacterStatsComponent>(deadEntity);
        auto& trans = registry.GetComponent<TransformComponent>(deadEntity);

        static std::random_device rd;
        static std::mt19937 rng(rd());
        std::uniform_real_distribution<float> chanceRoll(0.0f, 1.0f);

        float dropChance = 0.15f;
        ItemRarity minRarity = ItemRarity::Normal;
        switch (stats.rarity) {
        case MonsterRarity::Magic: dropChance = 0.4f; break;
        case MonsterRarity::Rare: dropChance = 0.8f; minRarity = ItemRarity::Magic; break;
        case MonsterRarity::Unique: dropChance = 1.0f; minRarity = ItemRarity::Rare; break;
        default: break;
        }

        if (chanceRoll(rng) > dropChance) return;

        int itemLevel = std::clamp(static_cast<int>(stats.maxHP / 15.0f), 1, 100);

        if (chanceRoll(rng) < 0.3f) {
            std::uniform_int_distribution<int> currencyPick(0, 2);
            CurrencyType currencyType = static_cast<CurrencyType>(currencyPick(rng));

            auto currencyPickup = registry.CreateEntityObject();
            currencyPickup.AddComponent(TransformComponent{ trans.position, {1.f, 1.f}, 0.f });
            currencyPickup.AddComponent(CircleComponent{ 7.0f, sf::Color(255, 140, 220), true });
            currencyPickup.AddComponent(CurrencyPickupComponent{ currencyType });
            return;
        }

        ItemRarity itemRarity = ItemFactory::RollRarity(rng);
        if (static_cast<int>(itemRarity) < static_cast<int>(minRarity)) itemRarity = minRarity;

        EquipSlot slot = ItemFactory::RollSlot(rng);
        ItemComponent item = ItemFactory::GenerateItem(slot, itemRarity, itemLevel, rng);

        auto pickup = registry.CreateEntityObject();
        pickup.AddComponent(TransformComponent{ trans.position, {1.f, 1.f}, 0.f });
        sf::Color dropColor = itemRarity == ItemRarity::Rare ? sf::Color(255, 200, 40)
            : itemRarity == ItemRarity::Magic ? sf::Color(80, 120, 255)
            : sf::Color(220, 220, 220);
        pickup.AddComponent(CircleComponent{ 8.0f, dropColor, true });
        pickup.AddComponent(ItemPickupComponent{ item });
    }

    sf::FloatRect GetBounds(const sf::Vector2f& pos, const BoxColliderComponent& col) {
        return sf::FloatRect(
            { pos.x + col.offsetX, pos.y + col.offsetY }, // �ʒu�x�N�g��
            { col.width, col.height }                     // �T�C�Y�x�N�g��
        );
    }
};