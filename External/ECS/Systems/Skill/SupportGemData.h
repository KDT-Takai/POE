#pragma once
#include "SkillGemData.h"
#include "SkillTags.h"
#include <string>
#include <vector>

// Multiplicative modifiers a support gem applies to the linked skill's assembled
// SkillData (see SupportGemSystem::Apply). 1.0f = no change. Kept purely multiplicative
// (no added flat stats) so applying N of them in any order/combination is trivial.
struct SupportModifiers {
    float damageMult = 1.0f;
    float cooldownMult = 1.0f;
    float mpCostMult = 1.0f;
    float rangeMult = 1.0f;
    float durationMult = 1.0f;
};

// Groups thematically-redundant supports so a skill can't stack more than one of the same
// kind (spec: "同一Skill内で同一Categoryを複数使用できない", gated in
// SkillGemSystem::AssignSupport, NOT a character-wide limit -- the same Category can still
// be socketed into two different skills at once).
enum class SupportCategory { DamageMult, Speed, Utility, AreaMod, Duration };

struct SupportGemDefinition {
    int id;
    std::string name;
    std::string description;
    GemAttribute primaryAttribute;
    int requirement; // flat -- support gems aren't leveled, unlike SkillGemData entries
    // Which SkillTag the linked skill must have for this support to be compatible
    // (SkillTag::None = universally compatible). Matches the spec's "Skill Tags確認
    // →Support条件と比較→使用可能Supportのみ表示" -- gated at socket-assignment time
    // (SkillGemSystem::SupportOptions/AssignSupport), not inside SupportGemSystem::Apply.
    SkillTag requiredTag;
    SupportCategory category;
    SupportModifiers mods;
};

// Separate id space from SkillGemData (support gems are a distinct GemAttribute-tagged
// catalog, referenced by ItemComponent::skillGemIsSupport, never mixed with skill/aura ids).
class SupportGemData {
public:
    static const std::vector<SupportGemDefinition>& Gems() {
        static std::vector<SupportGemDefinition> gems = BuildGems();
        return gems;
    }

    static const SupportGemDefinition* Find(int id) {
        for (const auto& gem : Gems()) {
            if (gem.id == id) return &gem;
        }
        return nullptr;
    }

private:
    static std::vector<SupportGemDefinition> BuildGems() {
        std::vector<SupportGemDefinition> gems;

        SupportModifiers addedDamage;
        addedDamage.damageMult = 1.25f;
        gems.push_back({ 0, "Added Damage Support", "Increases the linked skill's damage.",
            GemAttribute::Str, 20, SkillTag::None, SupportCategory::DamageMult, addedDamage });

        SupportModifiers brutality;
        brutality.damageMult = 1.35f;
        gems.push_back({ 1, "Brutality Support", "Increases damage, but only supports skills that deal physical damage.",
            GemAttribute::Str, 25, SkillTag::Physical, SupportCategory::DamageMult, brutality });

        SupportModifiers fasterAttacks;
        fasterAttacks.cooldownMult = 0.80f;
        gems.push_back({ 2, "Faster Attacks Support", "Reduces the linked skill's cooldown. Only supports Attack skills.",
            GemAttribute::Dex, 20, SkillTag::Attack, SupportCategory::Speed, fasterAttacks });

        SupportModifiers efficiency;
        efficiency.mpCostMult = 0.75f;
        gems.push_back({ 3, "Efficiency Support", "Reduces the linked skill's mana cost.",
            GemAttribute::Dex, 20, SkillTag::None, SupportCategory::Utility, efficiency });

        SupportModifiers increasedArea;
        increasedArea.rangeMult = 1.30f;
        gems.push_back({ 4, "Increased Area Support", "Increases the linked skill's area of effect. Only supports Area skills.",
            GemAttribute::Int, 20, SkillTag::AreaEffect, SupportCategory::AreaMod, increasedArea });

        SupportModifiers increasedDuration;
        increasedDuration.durationMult = 1.40f;
        gems.push_back({ 5, "Increased Duration Support", "Increases the linked skill's duration. Only supports skills with a duration.",
            GemAttribute::Int, 20, SkillTag::Duration, SupportCategory::Duration, increasedDuration });

        SupportModifiers concentratedEffect;
        concentratedEffect.damageMult = 1.40f;
        concentratedEffect.rangeMult = 0.70f;
        gems.push_back({ 6, "Concentrated Effect Support", "Increases damage but shrinks the area of effect. Only supports Area skills.",
            GemAttribute::Int, 25, SkillTag::AreaEffect, SupportCategory::AreaMod, concentratedEffect });

        SupportModifiers elementalFocus;
        elementalFocus.damageMult = 1.30f;
        gems.push_back({ 7, "Elemental Focus Support", "Increases damage, but only supports skills that deal elemental (non-physical) damage.",
            GemAttribute::Int, 25, SkillTag::Elemental, SupportCategory::DamageMult, elementalFocus });

        return gems;
    }
};
