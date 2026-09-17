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

    inline float ScaledMinionHp(float baseHp, int level) {
        return baseHp * (1.0f + static_cast<float>(level - 1) * 0.08f);
    }

    // Requirement grows with level so a high-level drop needs a correspondingly
    // developed character -- matches the "19/20 rare, and demanding to use" request.
    inline int RequiredStat(int baseRequirement, int level) {
        return baseRequirement + (level - 1) * 2;
    }

    // Rebuilds a live equipped SkillData from scratch: catalog base -> level scaling ->
    // socketed support modifiers (SupportGemSystem). Never mutates the static
    // SkillGemData catalog. Shared by SkillGemSystem (assign/re-socket) and GameScene's
    // save-load restore path, so both derive the exact same live stats from the same
    // equipped item. `gemItem` is the actual bag item backing the slot (see
    // PlayerSkill::equippedItems) -- its skillGemLevel/skillGemSupportIds are the source
    // of truth now that gems are inventory items rather than a separate owned-gems list.
    inline void BuildEquippedSkillData(SkillData& out, const ItemComponent& gemItem) {
        const GemDefinition* def = SkillGemData::Find(gemItem.skillGemId);
        if (!def) { out = SkillData{}; return; }
        int level = gemItem.skillGemLevel;

        out = def->skill;
        out.gemId = gemItem.skillGemId;
        out.level = level;
        out.isValid = true;
        out.currentCooldown = 0.0f;
        out.damage = ScaledDamage(out.damage, level);
        out.mpCost = static_cast<int>(ScaledMpCost(static_cast<float>(out.mpCost), level));

        SupportGemSystem::Apply(out, gemItem.skillGemSupportIds);
    }
}
