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
// SpiritGemLoadoutComponent. Unlike the 5 activated skill slots (PlayerSkill), an
// equipped Aura has no cooldown/activation: its effect is always on while equipped,
// exactly like a passive tree node, and is applied/removed the same way (see
// EquipmentSystem::ApplyAffix/RemoveAffix). The stat bonus itself (auraEffect) is not
// level-scaled (only its Spirit cost is -- see SkillGemScaling), since support gems
// (which would otherwise be the level/power knob) only apply to active skills, not auras.
class SpiritAuraSystem {
public:
    // Returns true and updates state (loadout/baseStats/currentSpirit) on success.
    // Returns false with no side effects if the new gem would exceed maxSpirit, is
    // already equipped in another Spirit slot, or the character doesn't meet its
    // level-scaled Str/Dex/Int requirement.
    static bool TryAssign(SpiritGemLoadoutComponent& loadout, int slotIndex, int newGemId,
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

        float reservedByOthers = 0.0f;
        for (int i = 0; i < static_cast<int>(loadout.auraGemIds.size()); ++i) {
            if (i == slotIndex) continue;
            reservedByOthers += SpiritCostOf(loadout.auraGemIds[i], gemInventory);
        }

        float newCost = SpiritCostOf(newGemId, gemInventory);
        if (reservedByOthers + newCost > stats.maxSpirit + 0.001f) {
            outMessage = "Not enough Spirit";
            return false;
        }

        if (currentGemId >= 0) {
            const GemDefinition* oldDef = SkillGemData::Find(currentGemId);
            if (oldDef) EquipmentSystem::RemoveAffix(equipment.baseStats, oldDef->skill.auraEffect);
        }
        if (newGemId >= 0) {
            const GemDefinition* newDef = SkillGemData::Find(newGemId);
            if (newDef) EquipmentSystem::ApplyAffix(equipment.baseStats, newDef->skill.auraEffect);
        }

        loadout.auraGemIds[slotIndex] = newGemId;
        stats.currentSpirit = reservedByOthers + newCost;
        EquipmentSystem::RecalculateStats(stats, equipment);

        const GemDefinition* assigned = newGemId >= 0 ? SkillGemData::Find(newGemId) : nullptr;
        outMessage = "Spirit slot: " + (assigned ? assigned->skill.name : "Empty");
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
};
