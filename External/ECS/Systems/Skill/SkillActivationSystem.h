#pragma once
#include <string>
#include "../../Components/Stats/SkillData/Skill.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Item/Equipment.h"
#include "SkillTags.h"

// Single source of truth for "can this skill be used right now" (spec: UI and Combat must
// never diverge). SkillGemSystem's Skills Panel checks are a DIFFERENT, earlier-stage
// question (does the player meet the STR/DEX/INT + weapon requirement to EQUIP this gem
// into a slot at all, checked once at assign time) -- CanUseSkill is the real-time
// cast-time gate (alive/valid/cooldown/mana/weapon-right-now), reusable by both the actual
// input-driven cast path (SkillSystem::Update) and any future HUD skill bar so they can
// never show a skill as usable when combat would actually refuse it, or vice versa.
namespace SkillActivation {
    enum class Reason {
        Ok,
        NotAlive,
        Empty,
        OnCooldown,
        NotEnoughMana,
        NoWeapon,
    };

    inline Reason CanUseSkill(const SkillData& skill, const CharacterStatsComponent& stats, const EquipmentComponent& equipment) {
        if (stats.currentHP <= 0.0f) return Reason::NotAlive;
        if (!skill.isValid) return Reason::Empty;
        if (skill.currentCooldown > 0.0f) return Reason::OnCooldown;
        if (stats.currentMP < static_cast<float>(skill.mpCost)) return Reason::NotEnoughMana;

        unsigned int tags = SkillTags::TagsFor(skill.behaviorType, skill.element);
        bool needsWeapon = (tags & static_cast<unsigned int>(SkillTag::Attack)) != 0;
        if (needsWeapon && !equipment.slots[static_cast<size_t>(EquipSlot::Weapon)].has_value()) {
            return Reason::NoWeapon;
        }

        return Reason::Ok;
    }

    inline std::string ReasonText(Reason reason) {
        switch (reason) {
        case Reason::Ok: return "";
        case Reason::NotAlive: return "Dead";
        case Reason::Empty: return "Empty slot";
        case Reason::OnCooldown: return "On cooldown";
        case Reason::NotEnoughMana: return "Not enough Mana";
        case Reason::NoWeapon: return "Requires a weapon equipped";
        default: return "";
        }
    }
}
