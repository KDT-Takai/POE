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
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Tags/Boss/Boss.h"
#include "../../Components/Chara/Minion.h"
#include <System/Campaign/CampaignManager.h>
#include "../../Components/VFX/HitFlash.h"
#include "../../Components/PlayerSkill/SparkVisual.h"
#include "../Combat/ImpactVfx.h"
#include <System/CameraManager/CameraManager.h>
#include "../Combat/CombatMath.h"
#include "../Item/ItemFactory.h"
#include "../Item/EquipmentSystem.h"
#include "../Progression/LevelSystem.h"
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

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

            // Snapshot everything needed from `proj`/`pTrans` by value, once, before the
            // enemy-hit loop below -- a piercing/bouncy projectile can hit several
            // enemies in the same frame, and each hit's ImpactVfx::SpawnHitBurst call
            // adds components (Transform/Velocity/Projectile/SparkVisual) that can
            // reallocate those pools' backing storage, silently invalidating `proj`/
            // `pTrans` themselves after the FIRST hit. Every use below reads these
            // locals, never `proj`/`pTrans` again (this is exactly what caused the
            // "スキル使うと落ちる" crash once ImpactVfx started running on every hit).
            Entity projOwner = proj.ownerEntity;
            DamageElement projDamageType = proj.damageType;
            SkillBehaviorType projType = proj.type;
            bool projIsBouncy = proj.isBouncy;
            float projDamage = proj.damage;
            sf::Vector2f projPos = pTrans.position;

            bool hitSomething = false;

            for (auto enemyEntity : enemies) {
                if (registry.HasComponent<PlayerInputComponent>(enemyEntity)) continue;
                // Permanent Minions (AllyTagComponent) are friendly -- the player's own
                // projectiles must never hit their own minion. MinionSystem handles
                // minion-vs-real-enemy combat separately.
                if (registry.HasComponent<AllyTagComponent>(enemyEntity)) continue;

                auto& eTrans = registry.GetComponent<TransformComponent>(enemyEntity);
                auto& eCol = registry.GetComponent<BoxColliderComponent>(enemyEntity);
                sf::FloatRect enemyRect = GetBounds(eTrans.position, eCol);

                if (bulletRect.findIntersection(enemyRect)) {
                    auto& stats = registry.GetComponent<CharacterStatsComponent>(enemyEntity);

                    // �܂�����ł��Ȃ��ꍇ�̂݃_���[�W��^����
                    if (stats.currentHP > 0) {
                        bool ownerValid = registry.IsValid(projOwner) && registry.HasComponent<CharacterStatsComponent>(projOwner);
                        bool isCrit = ownerValid && CombatMath::RollCrit(registry.GetComponent<CharacterStatsComponent>(projOwner).critRate);
                        float critMult = ownerValid ? registry.GetComponent<CharacterStatsComponent>(projOwner).critDamage : 1.0f;

                        float rawDamage = projDamage * (isCrit ? critMult : 1.0f);
                        if (registry.HasComponent<StatusEffectsComponent>(enemyEntity)) {
                            auto& targetStatus = registry.GetComponent<StatusEffectsComponent>(enemyEntity);
                            if (targetStatus.shockRemaining > 0.0f) rawDamage *= (1.0f + targetStatus.shockIncreasedDamageTaken);
                        }

                        float dealt = CombatMath::ApplyDamage(stats, rawDamage, projDamageType);

                        if (dealt > 0.0f) {
                            registry.AddComponent(enemyEntity, HitFlashComponent{ 0.08f });
                            ImpactVfx::SpawnHitBurst(registry, projPos, ImpactVfx::ElementColor(projDamageType), isCrit);
                            // A small punch of feedback on landing a crit -- taking damage
                            // already shakes the camera (below), dealing damage previously
                            // didn't at all.
                            if (isCrit) CameraManager::Instance().Shake(5.0f, 0.15f);
                        }

                        if (registry.HasComponent<StatusEffectsComponent>(enemyEntity)) {
                            CombatMath::ApplyAilmentOnHit(registry.GetComponent<StatusEffectsComponent>(enemyEntity), projDamageType, dealt, stats.maxHP, isCrit);
                        }

                        if (ownerValid) {
                            auto& ownerStats = registry.GetComponent<CharacterStatsComponent>(projOwner);
                            if (ownerStats.leechPercent > 0.0f) {
                                ownerStats.pendingLeech += dealt * ownerStats.leechPercent;
                            }
                        }

                        // �|�����u�Ԃ̏���
                        if (stats.currentHP <= 0) {
                            deadEntities.push_back(enemyEntity);

                            if (ownerValid && registry.HasComponent<PlayerInputComponent>(projOwner) && registry.HasComponent<EquipmentComponent>(projOwner)) {
                                auto& ownerStats = registry.GetComponent<CharacterStatsComponent>(projOwner);
                                auto& ownerEquip = registry.GetComponent<EquipmentComponent>(projOwner);
                                std::string levelMsg;
                                if (LevelSystem::GrantXP(ownerStats, ownerEquip, LevelSystem::CalcKillXP(stats), levelMsg)) {
                                    EquipmentSystem::RecalculateStats(ownerStats, ownerEquip);
                                    ownerStats.currentHP = ownerStats.maxHP;
                                    ownerStats.currentMP = ownerStats.maxMP;
                                    levelUpMessage = levelMsg;
                                    levelUpMessageTimer = 2.5f;
                                }
                            }

                            for (auto playerEnt : registry.View<PlayerInputComponent, CharacterStatsComponent, PlayerSkill>()) {

                                if (projType == SkillBehaviorType::LightningWarp) {
                                    auto& pStats = registry.GetComponent<CharacterStatsComponent>(playerEnt);
                                    auto& pSkill = registry.GetComponent<PlayerSkill>(playerEnt);
                                    // �}�i�S�� & �N�[���^�C�����Z�b�g (�X�L���X���b�g�͎��R�Ɋ��蓖�Ĉʒu���ς��̂ŁA
                                    // �Œ�C���f�b�N�X�ł͂Ȃ�behaviorType��T���čX�V����)
                                    pStats.currentMP = pStats.maxMP;
                                    for (auto& s : pSkill.skills) {
                                        if (s.isValid && s.behaviorType == SkillBehaviorType::LightningWarp) {
                                            s.currentCooldown = 0.0f;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (!projIsBouncy) {
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
                // A Permanent Minion is friendly -- it must never contact-damage the
                // player it belongs to.
                if (registry.HasComponent<AllyTagComponent>(e)) continue;
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
                        ImpactVfx::SpawnHitBurst(registry, pTrans.position, ImpactVfx::ElementColor(eStats.contactDamageType), isCrit);
                    }

                    if (hasStatus) {
                        CombatMath::ApplyAilmentOnHit(registry.GetComponent<StatusEffectsComponent>(playerEntity), eStats.contactDamageType, dealt, pStats.maxHP, isCrit);
                    }

                    pStats.hitInvincibilityTimer = 0.6f;
                }
            }

            if (pStats.hitInvincibilityTimer <= 0.0f) {
                for (auto projEntity : projectiles) {
                    auto& proj = registry.GetComponent<ProjectileComponent>(projEntity);
                    if (!proj.isEnemy) continue;

                    auto& pTrans2 = registry.GetComponent<TransformComponent>(projEntity);
                    auto& pCol2 = registry.GetComponent<BoxColliderComponent>(projEntity);
                    sf::FloatRect boltRect = GetBounds(pTrans2.position, pCol2);

                    if (!boltRect.findIntersection(playerRect)) continue;

                    // Snapshot before any registry mutation below -- see the matching
                    // comment in the player-projectile-vs-enemy loop above for why
                    // (ImpactVfx::SpawnHitBurst can reallocate the pool `proj`/`pTrans2`
                    // reference into, invalidating them).
                    Entity projOwner = proj.ownerEntity;
                    DamageElement projDamageType = proj.damageType;
                    float projDamage = proj.damage;
                    sf::Vector2f projPos = pTrans2.position;

                    bool ownerValid = registry.IsValid(projOwner) && registry.HasComponent<CharacterStatsComponent>(projOwner);
                    bool isCrit = ownerValid && CombatMath::RollCrit(registry.GetComponent<CharacterStatsComponent>(projOwner).critRate);
                    float critMult = ownerValid ? registry.GetComponent<CharacterStatsComponent>(projOwner).critDamage : 1.0f;

                    float rawDamage = projDamage * (isCrit ? critMult : 1.0f);
                    bool hasStatus = registry.HasComponent<StatusEffectsComponent>(playerEntity);
                    if (hasStatus) {
                        auto& pStatus = registry.GetComponent<StatusEffectsComponent>(playerEntity);
                        if (pStatus.shockRemaining > 0.0f) rawDamage *= (1.0f + pStatus.shockIncreasedDamageTaken);
                    }

                    float dealt = CombatMath::ApplyDamage(pStats, rawDamage, projDamageType);

                    if (dealt > 0.0f) {
                        registry.AddComponent(playerEntity, HitFlashComponent{ 0.08f });
                        float shakeStrength = std::clamp(dealt / pStats.maxHP, 0.0f, 1.0f) * 12.0f;
                        CameraManager::Instance().Shake(shakeStrength, 0.2f);
                        ImpactVfx::SpawnHitBurst(registry, projPos, ImpactVfx::ElementColor(projDamageType), isCrit);
                    }

                    if (hasStatus) {
                        CombatMath::ApplyAilmentOnHit(registry.GetComponent<StatusEffectsComponent>(playerEntity), projDamageType, dealt, pStats.maxHP, isCrit);
                    }

                    pStats.hitInvincibilityTimer = 0.6f;
                    destroyedProjectiles.push_back(projEntity);
                    break;
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

        if (CampaignManager::Instance().CurrentAct().isEndgame) {
            TrySpawnWaystoneDrop(registry, deadEntity, trans.position, stats.rarity);
        }

        // The Waystone that opened this map may roll "Increased Item Quantity/Rarity"
        // (see Item.h / ItemFactory::WaystoneModPool) -- unlike the monster-stat mods
        // (applied once at spawn time in ZoneBuilder), these describe loot itself, so
        // they're read here at drop time instead.
        float quantityBonus = 0.0f;
        float rarityBonus = 0.0f;
        for (const auto& mod : CampaignManager::Instance().GetActiveMapMods()) {
            if (mod.stat == WaystoneModStat::IncreasedItemQuantity) quantityBonus += mod.value;
            else if (mod.stat == WaystoneModStat::IncreasedItemRarity) rarityBonus += mod.value;
        }

        float dropChance = 0.15f;
        ItemRarity minRarity = ItemRarity::Normal;
        switch (stats.rarity) {
        case MonsterRarity::Magic: dropChance = 0.4f; break;
        case MonsterRarity::Rare: dropChance = 0.8f; minRarity = ItemRarity::Magic; break;
        case MonsterRarity::Unique: dropChance = 1.0f; minRarity = ItemRarity::Rare; break;
        default: break;
        }
        dropChance = (std::min)(1.0f, dropChance * (1.0f + quantityBonus / 100.0f));

        if (chanceRoll(rng) > dropChance) return;

        int itemLevel = std::clamp(static_cast<int>(stats.maxHP / 15.0f), 1, 100);

        if (chanceRoll(rng) < 0.08f) {
            // Uncut gem: level 1-20, cubic-skewed toward low rolls (so 19/20 are rare),
            // nudged up slightly by monster power -- mirrors itemLevel's role for gear
            // drops, but this doesn't pick a specific gem; see GemIdentifySystem for that.
            int monsterLevelFactor = std::clamp(itemLevel / 5, 1, 20);
            float skewed = std::pow(chanceRoll(rng), 3.0f);
            int gemLevel = std::clamp(1 + static_cast<int>(19.0f * skewed) + monsterLevelFactor / 4, 1, 20);
            // 60% Skill / 20% Support / 20% Spirit -- Skill gems stay the common case since
            // they're the only kind that directly expands the 5 active skill slots.
            float kindRoll = chanceRoll(rng);
            GemPickupKind kind = kindRoll < 0.6f ? GemPickupKind::Skill
                : kindRoll < 0.8f ? GemPickupKind::Support
                : GemPickupKind::Spirit;

            // Uncut Gem is a normal SkillGem-category item now (see Item.h/AI/DECISIONS.md
            // "スキルジェムもウェイストーンみたいにアイテム欄に置く") -- picked up through
            // the same ItemPickupComponent path as gear/Waystones, not a separate component.
            ItemComponent gemItem = ItemFactory::GenerateUncutSkillGem(gemLevel, kind);
            auto gemPickup = registry.CreateEntityObject();
            gemPickup.AddComponent(TransformComponent{ trans.position, {1.f, 1.f}, 0.f });
            gemPickup.AddComponent(CircleComponent{ 9.0f, sf::Color(255, 90, 220), true });
            gemPickup.AddComponent(ItemPickupComponent{ gemItem });
            return;
        }

        if (chanceRoll(rng) < 0.3f) {
            std::uniform_int_distribution<int> currencyPick(0, static_cast<int>(CurrencyType::Count) - 1);
            CurrencyType currencyType = static_cast<CurrencyType>(currencyPick(rng));

            auto currencyPickup = registry.CreateEntityObject();
            currencyPickup.AddComponent(TransformComponent{ trans.position, {1.f, 1.f}, 0.f });
            currencyPickup.AddComponent(CircleComponent{ 7.0f, sf::Color(255, 140, 220), true });
            currencyPickup.AddComponent(CurrencyPickupComponent{ currencyType });
            return;
        }

        ItemRarity itemRarity = ItemFactory::RollRarity(rng, rarityBonus);
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

    // Endgame-only: map bosses guarantee a Waystone one tier higher than the map just
    // run (matches real PoE2), and Rare monsters have a chance to drop one at the same
    // tier so mapping can be sustained without always needing a higher-tier stash.
    // Waystones are a normal ItemPickupComponent (ItemCategory::Waystone, see Item.h) --
    // held in the bag exactly like gear, so they compete for inventory space too and use
    // the exact same pickup/full-bag-sells-it path (ItemPickupSystem) as everything else.
    void TrySpawnWaystoneDrop(Registry& registry, Entity deadEntity, sf::Vector2f pos, MonsterRarity rarity) {
        int currentTier = CampaignManager::Instance().GetEndgameMapTier();
        bool isBoss = registry.HasComponent<BossTag>(deadEntity);

        static std::random_device rd;
        static std::mt19937 rng(rd());

        int dropTier = 0;
        if (isBoss) {
            dropTier = (std::min)(currentTier + 1, kMaxWaystoneTier);
        } else if (rarity == MonsterRarity::Rare) {
            std::uniform_real_distribution<float> roll(0.0f, 1.0f);
            if (roll(rng) < 0.35f) dropTier = currentTier;
        }
        if (dropTier <= 0) return;

        ItemComponent waystone = ItemFactory::GenerateWaystone(dropTier, rng);

        auto pickup = registry.CreateEntityObject();
        pickup.AddComponent(TransformComponent{ pos, {1.f, 1.f}, 0.f });
        pickup.AddComponent(CircleComponent{ 10.0f, sf::Color(0, 210, 255), true });
        pickup.AddComponent(ItemPickupComponent{ waystone });
    }

    sf::FloatRect GetBounds(const sf::Vector2f& pos, const BoxColliderComponent& col) {
        return sf::FloatRect(
            { pos.x + col.offsetX, pos.y + col.offsetY }, // �ʒu�x�N�g��
            { col.width, col.height }                     // �T�C�Y�x�N�g��
        );
    }
};