#pragma once
#include "SkillGemData.h"
#include "SupportGemSystem.h"
#include "../../Components/Item/SkillGem.h"

// Pure level-scaling functions for gem instances (level 1-20), mirroring
// ItemFactory's itemLevel convention (a flat multiplier per level, see
// ItemFactory::GenerateItem's `1.0f + itemLevel * 0.03f`). Applied when a
// GemDefinition is instantiated into a live SkillData (see BuildEquippedSkillData),
// never mutating the static catalog itself.
namespace SkillGemScaling {
    inline float ScaledDamage(float baseDamage, int level) {
        return baseDamage * (1.0f + static_cast<float>(level - 1) * 0.06f);
    }

    inline float ScaledMpCost(float baseMpCost, int level) {
        return baseMpCost * (1.0f + static_cast<float>(level - 1) * 0.05f);
    }

    inline float ScaledSpiritCost(float baseSpiritCost, int level) {
        return baseSpiritCost * (1.0f + static_cast<float>(level - 1) * 0.05f);
    }

    // Requirement grows with level so a high-level drop needs a correspondingly
    // developed character -- matches the "19/20 rare, and demanding to use" request.
    inline int RequiredStat(int baseRequirement, int level) {
        return baseRequirement + (level - 1) * 2;
    }

    inline const OwnedGemInstance* FindOwnedGem(const SkillGemInventoryComponent& inv, int gemId, bool isSupport) {
        for (const auto& owned : inv.ownedGems) {
            if (owned.isSupport == isSupport && owned.gemId == gemId) return &owned;
        }
        return nullptr;
    }

    // Rebuilds a live equipped SkillData from scratch: catalog base -> level scaling ->
    // socketed support modifiers (SupportGemSystem). Never mutates the static
    // SkillGemData catalog. Shared by SkillGemSystem (assign/re-socket) and GameScene's
    // save-load restore path, so both derive the exact same live stats from the same
    // OwnedGemInstance data.
    inline void BuildEquippedSkillData(SkillData& out, int gemId, const SkillGemInventoryComponent& gemInventory) {
        const GemDefinition* def = SkillGemData::Find(gemId);
        if (!def) { out = SkillData{}; return; }
        const OwnedGemInstance* inst = FindOwnedGem(gemInventory, gemId, false);
        int level = inst ? inst->level : 1;

        out = def->skill;
        out.gemId = gemId;
        out.level = level;
        out.isValid = true;
        out.currentCooldown = 0.0f;
        out.damage = ScaledDamage(out.damage, level);
        out.mpCost = static_cast<int>(ScaledMpCost(static_cast<float>(out.mpCost), level));

        if (inst) {
            SupportGemSystem::Apply(out, inst->supportGemIds);
        }
    }
}
