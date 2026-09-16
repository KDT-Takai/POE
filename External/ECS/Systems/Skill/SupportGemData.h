#pragma once
#include "SkillGemData.h"
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
    bool physicalOnly = false;  // only applies its damageMult to Physical-element skills
    bool elementalOnly = false; // only applies its damageMult to non-Physical/non-Chaos skills
};

struct SupportGemDefinition {
    int id;
    std::string name;
    GemAttribute primaryAttribute;
    int requirement; // flat -- support gems aren't leveled, unlike SkillGemData entries
    SupportModifiers mods;
};

// Separate id space from SkillGemData (support gems are a distinct GemAttribute-tagged
// catalog, referenced by OwnedGemInstance::isSupport, never mixed with skill/aura ids).
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
        gems.push_back({ 0, "Added Damage Support", GemAttribute::Str, 20, addedDamage });

        SupportModifiers brutality;
        brutality.damageMult = 1.35f;
        brutality.physicalOnly = true;
        gems.push_back({ 1, "Brutality Support", GemAttribute::Str, 25, brutality });

        SupportModifiers fasterAttacks;
        fasterAttacks.cooldownMult = 0.80f;
        gems.push_back({ 2, "Faster Attacks Support", GemAttribute::Dex, 20, fasterAttacks });

        SupportModifiers efficiency;
        efficiency.mpCostMult = 0.75f;
        gems.push_back({ 3, "Efficiency Support", GemAttribute::Dex, 20, efficiency });

        SupportModifiers increasedArea;
        increasedArea.rangeMult = 1.30f;
        gems.push_back({ 4, "Increased Area Support", GemAttribute::Int, 20, increasedArea });

        SupportModifiers increasedDuration;
        increasedDuration.durationMult = 1.40f;
        gems.push_back({ 5, "Increased Duration Support", GemAttribute::Int, 20, increasedDuration });

        SupportModifiers concentratedEffect;
        concentratedEffect.damageMult = 1.40f;
        concentratedEffect.rangeMult = 0.70f;
        gems.push_back({ 6, "Concentrated Effect Support", GemAttribute::Int, 25, concentratedEffect });

        SupportModifiers elementalFocus;
        elementalFocus.damageMult = 1.30f;
        elementalFocus.elementalOnly = true;
        gems.push_back({ 7, "Elemental Focus Support", GemAttribute::Int, 25, elementalFocus });

        return gems;
    }
};
