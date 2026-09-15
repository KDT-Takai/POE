#pragma once
#include <string>
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "SkillGemData.h"
#include "../Item/EquipmentSystem.h"

// Applies/removes Aura-gem effects (SkillData::auraEffect) on equipment.baseStats and
// tracks how much of maxSpirit is reserved by the two Spirit slots in
// SpiritGemLoadoutComponent. Unlike the 5 activated skill slots (PlayerSkill), an
// equipped Aura has no cooldown/activation: its effect is always on while equipped,
// exactly like a passive tree node, and is applied/removed the same way (see
// EquipmentSystem::ApplyAffix/RemoveAffix).
class SpiritAuraSystem {
public:
    // Returns true and updates state (loadout/baseStats/currentSpirit) on success.
    // Returns false with no side effects if the new gem would exceed maxSpirit.
    static bool TryAssign(SpiritGemLoadoutComponent& loadout, int slotIndex, int newGemId,
        EquipmentComponent& equipment, CharacterStatsComponent& stats, std::string& outMessage) {

        int currentGemId = loadout.auraGemIds[slotIndex];
        if (currentGemId == newGemId) return true; // no-op

        float reservedByOthers = 0.0f;
        for (int i = 0; i < static_cast<int>(loadout.auraGemIds.size()); ++i) {
            if (i == slotIndex) continue;
            reservedByOthers += SpiritCostOf(loadout.auraGemIds[i]);
        }

        float newCost = SpiritCostOf(newGemId);
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

    static float SpiritCostOf(int gemId) {
        if (gemId < 0) return 0.0f;
        const GemDefinition* def = SkillGemData::Find(gemId);
        return def ? def->skill.spiritCost : 0.0f;
    }
};
