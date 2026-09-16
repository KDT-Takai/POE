#pragma once
#include <string>
#include "../../Registry/Registry.h"
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Chara/Minion.h"
#include "SkillGemData.h"
#include "SkillGemScaling.h"
#include "../Item/EquipmentSystem.h"
#include "Game/GameScene/Entity/EntitySpawner.h"

// Applies/removes Aura-gem effects (SkillData::auraEffect) on equipment.baseStats, or
// spawns/despawns a Permanent Minion (SkillBehaviorType::Minion, see MinionSystem for its
// AI/respawn loop), and tracks how much of maxSpirit is reserved by the 5 Spirit slots in
// SpiritGemLoadoutComponent. Two separate steps, matching the spec: TryRegister puts a
// gem into a slot with no effect yet (just makes it available), and TryToggleActive
// switches an already-registered gem ON (reserve Spirit, apply the effect/spawn the
// minion) or OFF (release both). An equipped Aura has no cooldown/activation beyond that
// toggle: while ON its effect is always on, exactly like a passive tree node, applied/
// removed the same way (see EquipmentSystem::ApplyAffix/RemoveAffix). The stat bonus
// itself (auraEffect) is not level-scaled (only its Spirit cost is -- see
// SkillGemScaling), since support gems (which would otherwise be the level/power knob)
// only apply to active skills, not auras.
class SpiritAuraSystem {
public:
    // Returns true and updates auraGemIds[slotIndex] on success. If the slot held an
    // active gem, it's turned off first (Spirit released, effect removed/minion despawned)
    // since that gem is being replaced/cleared -- registering never leaves a stale active
    // effect pointing at the wrong gem.
    static bool TryRegister(Registry& registry, Entity ownerEntity, SpiritGemLoadoutComponent& loadout, int slotIndex, int newGemId,
        const SkillGemInventoryComponent& gemInventory, EquipmentComponent& equipment,
        CharacterStatsComponent& stats, std::string& outMessage) {

        int currentGemId = loadout.auraGemIds[slotIndex];
        if (currentGemId == newGemId) return true; // no-op

        if (newGemId >= 0) {
            for (int i = 0; i < static_cast<int>(loadout.auraGemIds.size()); ++i) {
                if (i != slotIndex && loadout.auraGemIds[i] == newGemId) {
                    outMessage = "Already equipped in another Spirit slot";
                    return false;
                }
            }

            const GemDefinition* newDef = SkillGemData::Find(newGemId);
            if (newDef) {
                int level = LevelOf(gemInventory, newGemId);
                int required = SkillGemScaling::RequiredStat(newDef->baseRequirement, level);
                int have = StatValue(stats, newDef->primaryAttribute);
                if (have < required) {
                    outMessage = "Requires " + std::to_string(required) + " " + AttributeName(newDef->primaryAttribute)
                        + " (have " + std::to_string(have) + ")";
                    return false;
                }
            }
        }

        if (loadout.active[slotIndex]) {
            SetActive(registry, ownerEntity, loadout, slotIndex, false, gemInventory, equipment, stats);
        }

        loadout.auraGemIds[slotIndex] = newGemId;
        const GemDefinition* assigned = newGemId >= 0 ? SkillGemData::Find(newGemId) : nullptr;
        outMessage = "Spirit slot: " + (assigned ? assigned->skill.name : "Empty");
        return true;
    }

    // Toggles an already-registered slot ON (reserve Spirit + apply effect/spawn minion,
    // rejected if the budget is exceeded) or OFF (release both). No-op with a message if
    // the slot is empty.
    static bool TryToggleActive(Registry& registry, Entity ownerEntity, SpiritGemLoadoutComponent& loadout, int slotIndex,
        const SkillGemInventoryComponent& gemInventory, EquipmentComponent& equipment,
        CharacterStatsComponent& stats, std::string& outMessage) {

        int gemId = loadout.auraGemIds[slotIndex];
        if (gemId < 0) {
            outMessage = "No gem registered in this slot";
            return false;
        }

        bool turningOn = !loadout.active[slotIndex];
        if (turningOn) {
            // slotIndex itself isn't active yet (that's what turningOn means), so the sum
            // over all active slots is already "reserved by others".
            float reservedByOthers = SumActiveReservations(loadout, gemInventory);
            float thisCost = SpiritCostOf(gemId, gemInventory);
            if (reservedByOthers + thisCost > stats.maxSpirit + 0.001f) {
                outMessage = "Not enough Spirit";
                return false;
            }
        }

        SetActive(registry, ownerEntity, loadout, slotIndex, turningOn, gemInventory, equipment, stats);
        const GemDefinition* def = SkillGemData::Find(gemId);
        outMessage = (def ? def->skill.name : std::string("Spirit")) + (turningOn ? ": ON" : ": OFF");
        return true;
    }

    // Recomputes reserved Spirit from scratch and, if it now exceeds maxSpirit (e.g. a
    // Spirit-granting item was unequipped while a Spirit skill was ON), turns off active
    // slots from the highest index down until it fits again -- exactly like a normal OFF
    // (effect removed/minion despawned + Spirit released), never left silently over-
    // reserved. Safe to call every frame regardless of what last changed maxSpirit/
    // baseStats (equip swap, level up, passive spend, save load), so callers don't need to
    // hook every RecalculateStats call site individually -- see GameScene::Update.
    static void ReevaluateReservations(Registry& registry, Entity ownerEntity, SpiritGemLoadoutComponent& loadout,
        const SkillGemInventoryComponent& gemInventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        float total = SumActiveReservations(loadout, gemInventory);

        for (int i = static_cast<int>(loadout.active.size()) - 1; i >= 0 && total > stats.maxSpirit + 0.001f; --i) {
            if (!loadout.active[i]) continue;
            SetActive(registry, ownerEntity, loadout, i, false, gemInventory, equipment, stats);
            total = SumActiveReservations(loadout, gemInventory);
        }

        stats.currentSpirit = total;
    }

    static float SpiritCostOf(int gemId, const SkillGemInventoryComponent& gemInventory) {
        if (gemId < 0) return 0.0f;
        const GemDefinition* def = SkillGemData::Find(gemId);
        if (!def) return 0.0f;
        return SkillGemScaling::ScaledSpiritCost(def->skill.spiritCost, LevelOf(gemInventory, gemId));
    }

    static int LevelOf(const SkillGemInventoryComponent& gemInventory, int gemId) {
        const OwnedGemInstance* owned = SkillGemScaling::FindOwnedGem(gemInventory, gemId, false);
        return owned ? owned->level : 1;
    }

    static int StatValue(const CharacterStatsComponent& stats, GemAttribute attr) {
        switch (attr) {
        case GemAttribute::Str: return stats.str;
        case GemAttribute::Dex: return stats.dex;
        case GemAttribute::Int: return stats.intelligence;
        default: return 0;
        }
    }

    static std::string AttributeName(GemAttribute attr) {
        switch (attr) {
        case GemAttribute::Str: return "Str";
        case GemAttribute::Dex: return "Dex";
        case GemAttribute::Int: return "Int";
        default: return "";
        }
    }

private:
    // Flips active[slotIndex], applying/removing the CURRENTLY-registered gem's effect (or
    // spawning/despawning its minion, read before the flip so an OFF correctly removes the
    // gem that was actually on), then recomputes currentSpirit as the sum of every active
    // slot's cost.
    static void SetActive(Registry& registry, Entity ownerEntity, SpiritGemLoadoutComponent& loadout, int slotIndex, bool active,
        const SkillGemInventoryComponent& gemInventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        int gemId = loadout.auraGemIds[slotIndex];
        const GemDefinition* def = gemId >= 0 ? SkillGemData::Find(gemId) : nullptr;
        bool isMinion = def && def->skill.behaviorType == SkillBehaviorType::Minion;

        if (loadout.active[slotIndex] && def) {
            if (isMinion) DespawnMinion(registry, loadout, slotIndex);
            else EquipmentSystem::RemoveAffix(equipment.baseStats, def->skill.auraEffect);
        }
        loadout.active[slotIndex] = active;
        if (active && def) {
            if (isMinion) SpawnMinion(registry, ownerEntity, loadout, slotIndex, gemId, LevelOf(gemInventory, gemId), stats.atk);
            else EquipmentSystem::ApplyAffix(equipment.baseStats, def->skill.auraEffect);
        }
        if (!active) loadout.minionRespawnTimer[slotIndex] = 0.0f;

        stats.currentSpirit = SumActiveReservations(loadout, gemInventory);
        EquipmentSystem::RecalculateStats(stats, equipment);
    }

    static void SpawnMinion(Registry& registry, Entity ownerEntity, SpiritGemLoadoutComponent& loadout, int slotIndex,
        int gemId, int level, float ownerAtk) {
        if (loadout.minionEntity[slotIndex] != SpiritGemLoadoutComponent::kInvalidMinion
            && registry.IsValid(loadout.minionEntity[slotIndex])) {
            return; // already alive (e.g. re-toggling ON without an OFF in between)
        }
        sf::Vector2f spawnPos(0.0f, 0.0f);
        if (registry.HasComponent<TransformComponent>(ownerEntity)) {
            spawnPos = registry.GetComponent<TransformComponent>(ownerEntity).position;
        }
        auto minion = EntitySpawner::CreateMinion(registry, spawnPos, ownerEntity, slotIndex, gemId, level, ownerAtk);
        loadout.minionEntity[slotIndex] = minion.GetID();
        loadout.minionRespawnTimer[slotIndex] = 0.0f;
    }

    static void DespawnMinion(Registry& registry, SpiritGemLoadoutComponent& loadout, int slotIndex) {
        Entity minion = loadout.minionEntity[slotIndex];
        if (minion != SpiritGemLoadoutComponent::kInvalidMinion && registry.IsValid(minion)) {
            registry.DestroyEntity(minion);
        }
        loadout.minionEntity[slotIndex] = SpiritGemLoadoutComponent::kInvalidMinion;
        loadout.minionRespawnTimer[slotIndex] = 0.0f;
    }

    static float SumActiveReservations(const SpiritGemLoadoutComponent& loadout, const SkillGemInventoryComponent& gemInventory) {
        float total = 0.0f;
        for (int i = 0; i < static_cast<int>(loadout.auraGemIds.size()); ++i) {
            if (loadout.active[i]) total += SpiritCostOf(loadout.auraGemIds[i], gemInventory);
        }
        return total;
    }
};
