#pragma once
#include "SupportGemData.h"
#include "../../Components/Stats/SkillData/Skill.h"
#include <array>

// Applies every support gem socketed into a skill/spirit gem onto its assembled
// SkillData, in socket order. Purely multiplicative (see SupportModifiers), so order
// doesn't change the final result. Called once when SkillGemSystem builds the live
// SkillData for an equipped slot (mirrors EquipmentSystem::ApplyAffix's "apply on
// assembly, never mutate the catalog" pattern). Support gems carry no per-instance state
// (unlike skill/spirit gems, they're not leveled), so sockets store raw SupportGemData
// ids directly -- same convention PlayerSkill.skills[i].gemId already uses for the
// active skill itself.
class SupportGemSystem {
public:
    static void Apply(SkillData& skill, const std::array<int, 5>& supportGemIds) {
        for (int gemId : supportGemIds) {
            if (gemId < 0) continue;
            const SupportGemDefinition* def = SupportGemData::Find(gemId);
            if (!def) continue;
            ApplyOne(skill, def->mods);
        }
    }

private:
    static void ApplyOne(SkillData& skill, const SupportModifiers& mods) {
        bool isPhysical = (skill.element == DamageElement::Physical);
        bool isElemental = (skill.element == DamageElement::Fire || skill.element == DamageElement::Cold
            || skill.element == DamageElement::Lightning);

        bool damageAllowed = (!mods.physicalOnly || isPhysical) && (!mods.elementalOnly || isElemental);
        if (damageAllowed) {
            skill.damage *= mods.damageMult;
        }

        skill.cooldownTime *= mods.cooldownMult;
        skill.mpCost = static_cast<int>(static_cast<float>(skill.mpCost) * mods.mpCostMult);
        skill.range *= mods.rangeMult;
        skill.duration *= mods.durationMult;
    }
};
