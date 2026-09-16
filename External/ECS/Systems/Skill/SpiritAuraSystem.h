#pragma once
#include <string>
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "SkillGemData.h"
#include "SkillGemScaling.h"
#include "../Item/EquipmentSystem.h"

// Applies/removes Aura-gem effects (SkillData::auraEffect) on equipment.baseStats and
// tracks how much of maxSpirit is reserved by the 5 Spirit slots in
// SpiritGemLoadoutComponent. Two separate steps, matching the spec: TryRegister puts a
// gem into a slot with no effect yet (just makes it available), and TryToggleActive
// switches an already-registered gem ON (reserve Spirit, apply the effect) or OFF
// (release both). An equipped Aura has no cooldown/activation beyond that toggle: while
// ON its effect is always on, exactly like a passive tree node, applied/removed the same
// way (see EquipmentSystem::ApplyAffix/RemoveAffix). The stat bonus itself (auraEffect)
// is not level-scaled (only its Spirit cost is -- see SkillGemScaling), since support
// gems (which would otherwise be the level/power knob) only apply to active skills, not
// auras.
class SpiritAuraSystem {
public:
    // Returns true and updates auraGemIds[slotIndex] on success. If the slot held an
    // active gem, it's turned off first (Spirit released, effect removed) since that
    // gem is being replaced/cleared -- registering never leaves a stale active effect
    // pointing at the wrong gem.
    static bool TryRegister(SpiritGemLoadoutComponent& loadout, int slotIndex, int newGemId,
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
            SetActive(loadout, slotIndex, false, gemInventory, equipment, stats);
        }

        loadout.auraGemIds[slotIndex] = newGemId;
        const GemDefinition* assigned = newGemId >= 0 ? SkillGemData::Find(newGemId) : nullptr;
        outMessage = "Spirit slot: " + (assigned ? assigned->skill.name : "Empty");
        return true;
    }

    // Toggles an already-registered slot ON (reserve Spirit + apply effect, rejected if
    // the budget is exceeded) or OFF (release both). No-op with a message if the slot is
    // empty.
    static bool TryToggleActive(SpiritGemLoadoutComponent& loadout, int slotIndex,
        const SkillGemInventoryComponent& gemInventory, EquipmentComponent& equipment,
        CharacterStatsComponent& stats, std::string& outMessage) {

        int gemId = loadout.auraGemIds[slotIndex];
        if (gemId < 0) {
            outMessage = "No gem registered in this slot";
            return false;
        }

        bool turningOn = !loadout.active[slotIndex];
        if (turningOn) {
            float reservedByOthers = 0.0f;
            for (int i = 0; i < static_cast<int>(loadout.auraGemIds.size()); ++i) {
                if (i != slotIndex && loadout.active[i]) {
                    reservedByOthers += SpiritCostOf(loadout.auraGemIds[i], gemInventory);
                }
            }
            float thisCost = SpiritCostOf(gemId, gemInventory);
            if (reservedByOthers + thisCost > stats.maxSpirit + 0.001f) {
                outMessage = "Not enough Spirit";
                return false;
            }
        }

        SetActive(loadout, slotIndex, turningOn, gemInventory, equipment, stats);
        const GemDefinition* def = SkillGemData::Find(gemId);
        outMessage = (def ? def->skill.name : std::string("Spirit")) + (turningOn ? ": ON" : ": OFF");
        return true;
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
    // Flips active[slotIndex], applying/removing the CURRENTLY-registered gem's effect
    // (read before the flip so an OFF correctly removes the gem that was actually on),
    // then recomputes currentSpirit as the sum of every active slot's cost.
    static void SetActive(SpiritGemLoadoutComponent& loadout, int slotIndex, bool active,
        const SkillGemInventoryComponent& gemInventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        int gemId = loadout.auraGemIds[slotIndex];
        const GemDefinition* def = gemId >= 0 ? SkillGemData::Find(gemId) : nullptr;

        if (loadout.active[slotIndex] && def) {
            EquipmentSystem::RemoveAffix(equipment.baseStats, def->skill.auraEffect);
        }
        loadout.active[slotIndex] = active;
        if (active && def) {
            EquipmentSystem::ApplyAffix(equipment.baseStats, def->skill.auraEffect);
        }

        float total = 0.0f;
        for (int i = 0; i < static_cast<int>(loadout.auraGemIds.size()); ++i) {
            if (loadout.active[i]) total += SpiritCostOf(loadout.auraGemIds[i], gemInventory);
        }
        stats.currentSpirit = total;
        EquipmentSystem::RecalculateStats(stats, equipment);
    }
};
