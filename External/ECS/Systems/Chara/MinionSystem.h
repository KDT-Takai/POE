#pragma once
#include <cmath>
#include <vector>
#include "../../Registry/Registry.h"
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Physics/Velocity/Velocity.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "../../Components/Chara/Minion.h"
#include "../../Components/VFX/HitFlash.h"
#include "../Skill/SkillGemData.h"
#include "../Skill/SkillGemScaling.h"
#include "../Combat/CombatMath.h"
#include "Game/GameScene/Entity/EntitySpawner.h"

// Drives the two halves of a Permanent Minion's lifecycle (see PermanentMinionComponent /
// SpiritGemLoadoutComponent::minionEntity/minionRespawnTimer, and SpiritAuraSystem for the
// spawn/despawn trigger on Spirit-skill ON/OFF):
//  1. Respawn ticking: for the player's active Minion-type Spirit slots with no live
//     minion entity, count down minionRespawnTimer and respawn once it elapses. Runs
//     whether or not a minion died this frame -- also naturally covers loading a save
//     with an active Minion slot, since minionEntity/minionRespawnTimer both restore as
//     their defaults (kInvalidMinion/0), so a fresh minion spawns on the first tick with
//     no special-cased load-time logic needed.
//  2. AI/combat for minions that currently exist: chase the nearest living non-ally
//     within aggro range and trade contact damage with it, otherwise follow the owner.
//     On death, notify the owning slot (clear minionEntity, start minionRespawnTimer)
//     instead of releasing the Spirit reservation -- per spec, only turning the skill
//     OFF does that; the skill stays Active the whole time a minion is dead and waiting
//     to respawn.
class MinionSystem {
public:
    void Update(Registry& registry, float dt) {
        TickRespawns(registry, dt);
        UpdateLivingMinions(registry, dt);
    }

private:
    static constexpr float kRespawnDelay = 8.0f;
    static constexpr float kAggroRange = 260.0f;
    static constexpr float kAttackRange = 34.0f;
    static constexpr float kAttackCooldown = 1.0f;
    static constexpr float kFollowStopRadius = 60.0f;

    void TickRespawns(Registry& registry, float dt) {
        for (auto player : registry.View<PlayerInputComponent, SpiritGemLoadoutComponent, SkillGemInventoryComponent, TransformComponent, CharacterStatsComponent>()) {
            auto& loadout = registry.GetComponent<SpiritGemLoadoutComponent>(player);
            auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);
            auto& playerTrans = registry.GetComponent<TransformComponent>(player);
            auto& playerStats = registry.GetComponent<CharacterStatsComponent>(player);

            for (int i = 0; i < static_cast<int>(loadout.auraGemIds.size()); ++i) {
                if (!loadout.active[i]) continue;
                int gemId = loadout.auraGemIds[i];
                const GemDefinition* def = gemId >= 0 ? SkillGemData::Find(gemId) : nullptr;
                if (!def || def->skill.behaviorType != SkillBehaviorType::Minion) continue;

                bool alive = loadout.minionEntity[i] != SpiritGemLoadoutComponent::kInvalidMinion && registry.IsValid(loadout.minionEntity[i]);
                if (alive) continue;

                if (loadout.minionRespawnTimer[i] > 0.0f) {
                    loadout.minionRespawnTimer[i] -= dt;
                    continue;
                }

                int level = 1;
                if (const OwnedGemInstance* owned = SkillGemScaling::FindOwnedGem(gemInventory, gemId, false)) level = owned->level;

                auto minion = EntitySpawner::CreateMinion(registry, playerTrans.position, player, i, gemId, level, playerStats.atk);
                loadout.minionEntity[i] = minion.GetID();
            }
        }
    }

    void UpdateLivingMinions(Registry& registry, float dt) {
        std::vector<Entity> toDestroy;

        for (auto entity : registry.View<PermanentMinionComponent, CharacterStatsComponent, TransformComponent, VelocityComponent>()) {
            auto& minion = registry.GetComponent<PermanentMinionComponent>(entity);
            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);
            auto& trans = registry.GetComponent<TransformComponent>(entity);
            auto& vel = registry.GetComponent<VelocityComponent>(entity);

            if (minion.attackCooldown > 0.0f) minion.attackCooldown -= dt;

            Entity target = static_cast<Entity>(-1);
            sf::Vector2f targetPos(0.0f, 0.0f);
            float bestDistSq = kAggroRange * kAggroRange;
            for (auto e : registry.View<CharacterStatsComponent, TransformComponent>()) {
                if (e == entity) continue;
                if (registry.HasComponent<PlayerInputComponent>(e)) continue;
                if (registry.HasComponent<AllyTagComponent>(e)) continue;
                auto& es = registry.GetComponent<CharacterStatsComponent>(e);
                if (es.currentHP <= 0.0f) continue;

                auto& et = registry.GetComponent<TransformComponent>(e);
                float dx = et.position.x - trans.position.x;
                float dy = et.position.y - trans.position.y;
                float distSq = dx * dx + dy * dy;
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    target = e;
                    targetPos = et.position;
                }
            }

            bool hasTarget = registry.IsValid(target);
            sf::Vector2f desired = trans.position;
            if (hasTarget) {
                desired = targetPos;
            } else if (registry.IsValid(minion.ownerEntity) && registry.HasComponent<TransformComponent>(minion.ownerEntity)) {
                desired = registry.GetComponent<TransformComponent>(minion.ownerEntity).position;
            }

            sf::Vector2f diff = desired - trans.position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            float stopRadius = hasTarget ? kAttackRange * 0.8f : kFollowStopRadius;

            if (dist > stopRadius && dist > 0.0001f) {
                vel.velocity = (diff / dist) * stats.moveSpeed;
            } else {
                vel.velocity = { 0.0f, 0.0f };
            }

            if (hasTarget && dist <= kAttackRange && minion.attackCooldown <= 0.0f) {
                auto& targetStats = registry.GetComponent<CharacterStatsComponent>(target);

                if (CombatMath::RollHit(stats.accuracy, targetStats.evasion)) {
                    float dealt = CombatMath::ApplyDamage(targetStats, stats.atk, stats.contactDamageType);
                    if (dealt > 0.0f) registry.AddComponent(target, HitFlashComponent{ 0.08f });
                }
                if (CombatMath::RollHit(targetStats.accuracy, stats.evasion)) {
                    float dealt = CombatMath::ApplyDamage(stats, targetStats.atk, targetStats.contactDamageType);
                    if (dealt > 0.0f) registry.AddComponent(entity, HitFlashComponent{ 0.08f });
                }
                minion.attackCooldown = kAttackCooldown;
            }

            if (stats.currentHP <= 0.0f) {
                if (registry.IsValid(minion.ownerEntity) && registry.HasComponent<SpiritGemLoadoutComponent>(minion.ownerEntity)) {
                    auto& loadout = registry.GetComponent<SpiritGemLoadoutComponent>(minion.ownerEntity);
                    if (minion.spiritSlotIndex >= 0 && minion.spiritSlotIndex < static_cast<int>(loadout.minionEntity.size())
                        && loadout.minionEntity[minion.spiritSlotIndex] == entity) {
                        loadout.minionEntity[minion.spiritSlotIndex] = SpiritGemLoadoutComponent::kInvalidMinion;
                        loadout.minionRespawnTimer[minion.spiritSlotIndex] = kRespawnDelay;
                    }
                }
                toDestroy.push_back(entity);
            }
        }

        for (auto e : toDestroy) {
            if (registry.IsValid(e)) registry.DestroyEntity(e);
        }
    }
};
