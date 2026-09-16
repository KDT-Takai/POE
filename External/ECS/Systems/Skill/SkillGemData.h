#pragma once
#include "Components/Stats/SkillData/Skill.h"
#include <vector>

// Which CharacterStatsComponent attribute a gem's level-scaled requirement is checked
// against (see SkillGemScaling::RequiredStat). Derived mechanically from behaviorType,
// not hand-tuned per gem: Str = melee/physical-ish, Dex = mobility/single-target agility,
// Int = spells/auras. Shared with SupportGemData.
enum class GemAttribute { Str, Dex, Int };

struct GemDefinition {
    int id;
    SkillData skill;
    GemAttribute primaryAttribute;
    int baseRequirement; // requirement at gem level 1; see SkillGemScaling::RequiredStat for the level curve
};

// Static catalog of every skill gem that can be found/equipped in any of the
// 5 player skill slots. Equipping is free and reversible (see SkillGemSystem);
// only *finding* a gem is a one-time unlock (SkillGemInventoryComponent).
class SkillGemData {
public:
    static const std::vector<GemDefinition>& Gems() {
        static std::vector<GemDefinition> gems = BuildGems();
        return gems;
    }

    static const GemDefinition* Find(int id) {
        for (const auto& gem : Gems()) {
            if (gem.id == id) return &gem;
        }
        return nullptr;
    }

private:
    static std::vector<GemDefinition> BuildGems() {
        std::vector<GemDefinition> gems;

        SkillData spark;
        spark.name = "Spark";
        spark.behaviorType = SkillBehaviorType::Spark;
        spark.cooldownTime = 0.3f;
        spark.mpCost = 4;
        spark.damage = 25.0f; // % of atk, per pellet (fires 7)
        spark.duration = 3.5f;
        spark.element = DamageElement::Lightning;
        spark.isValid = true;
        gems.push_back({ 0, spark, GemAttribute::Int, 8 });

        SkillData slam;
        slam.name = "Thunder Slam";
        slam.behaviorType = SkillBehaviorType::GroundSlam;
        slam.cooldownTime = 5.0f;
        slam.mpCost = 35;
        slam.damage = 120.0f; // % of atk
        slam.element = DamageElement::Physical;
        slam.isValid = true;
        gems.push_back({ 1, slam, GemAttribute::Str, 8 });

        SkillData warp;
        warp.name = "Lightning Warp";
        warp.behaviorType = SkillBehaviorType::LightningWarp;
        warp.range = 350.0f;
        warp.damage = 80.0f;
        warp.cooldownTime = 8.0f;
        warp.mpCost = 35;
        warp.element = DamageElement::Lightning;
        warp.isValid = true;
        gems.push_back({ 2, warp, GemAttribute::Int, 8 });

        SkillData ball;
        ball.name = "Lightning Ball";
        ball.behaviorType = SkillBehaviorType::LightningBall;
        ball.damage = 40.0f; // % of atk, per bullet (fires 12)
        ball.cooldownTime = 12.0f;
        ball.mpCost = 40;
        ball.element = DamageElement::Lightning;
        ball.isValid = true;
        gems.push_back({ 3, ball, GemAttribute::Int, 8 });

        SkillData warcry;
        warcry.name = "War Cry";
        warcry.behaviorType = SkillBehaviorType::Buff;
        warcry.cooldownTime = 18.0f;
        warcry.mpCost = 20;
        warcry.duration = 6.0f;
        warcry.buffAtk = 0.3f;   // +30% atk
        warcry.buffSpeed = 0.25f; // +25% move speed
        warcry.element = DamageElement::Physical;
        warcry.isValid = true;
        gems.push_back({ 4, warcry, GemAttribute::Str, 8 });

        SkillData nova;
        nova.name = "Nova";
        nova.behaviorType = SkillBehaviorType::AreaEffect;
        nova.cooldownTime = 6.0f;
        nova.mpCost = 30;
        nova.damage = 90.0f; // % of atk, matches LightningWarp's convention
        nova.range = 180.0f; // blast radius-ish size
        nova.element = DamageElement::Cold;
        nova.isValid = true;
        gems.push_back({ 5, nova, GemAttribute::Int, 8 });

        SkillData cleave;
        cleave.name = "Cleave";
        cleave.behaviorType = SkillBehaviorType::Melee;
        cleave.cooldownTime = 0.6f;
        cleave.mpCost = 8;
        cleave.damage = 110.0f; // % of atk
        cleave.range = 70.0f;
        cleave.element = DamageElement::Physical;
        cleave.isValid = true;
        gems.push_back({ 6, cleave, GemAttribute::Str, 8 });

        SkillData frostBolt;
        frostBolt.name = "Frost Bolt";
        frostBolt.behaviorType = SkillBehaviorType::Projectile;
        frostBolt.cooldownTime = 0.5f;
        frostBolt.mpCost = 6;
        frostBolt.damage = 140.0f; // % of atk, single-target so higher than AoE skills
        frostBolt.element = DamageElement::Cold;
        frostBolt.isValid = true;
        gems.push_back({ 7, frostBolt, GemAttribute::Dex, 8 });

        // Aura gems: not activated skills. Equipping one in a Spirit slot (see
        // SkillGemSystem) reserves spiritCost from maxSpirit for as long as it stays
        // equipped, and its auraEffect is applied directly to equipment.baseStats
        // (removed the same way on unequip; see SpiritAuraSystem).
        SkillData determination;
        determination.name = "Determination";
        determination.behaviorType = SkillBehaviorType::Aura;
        determination.spiritCost = 50.0f;
        determination.auraEffect = { AffixStat::FlatArmour, 60.0f, 1, true, "Armour" };
        determination.isValid = true;
        gems.push_back({ 8, determination, GemAttribute::Int, 8 });

        SkillData discipline;
        discipline.name = "Discipline";
        discipline.behaviorType = SkillBehaviorType::Aura;
        discipline.spiritCost = 40.0f;
        discipline.auraEffect = { AffixStat::FlatES, 40.0f, 1, true, "Energy Shield" };
        discipline.isValid = true;
        gems.push_back({ 9, discipline, GemAttribute::Int, 8 });

        // Fire/Chaos were the only two DamageElement values with no active skill gem
        // dealing that type (Physical/Lightning/Cold were already covered above), even
        // though monsters and item resistances both use all 4 elements.
        SkillData fireball;
        fireball.name = "Fireball";
        fireball.behaviorType = SkillBehaviorType::AreaEffect;
        fireball.cooldownTime = 5.0f;
        fireball.mpCost = 28;
        fireball.damage = 100.0f; // % of atk, matches Nova's convention
        fireball.range = 160.0f;
        fireball.element = DamageElement::Fire;
        fireball.isValid = true;
        gems.push_back({ 10, fireball, GemAttribute::Int, 8 });

        SkillData chaosBolt;
        chaosBolt.name = "Chaos Bolt";
        chaosBolt.behaviorType = SkillBehaviorType::Projectile;
        chaosBolt.cooldownTime = 0.5f;
        chaosBolt.mpCost = 6;
        chaosBolt.damage = 130.0f; // % of atk, matches Frost Bolt's single-target convention
        chaosBolt.element = DamageElement::Chaos;
        chaosBolt.isValid = true;
        gems.push_back({ 11, chaosBolt, GemAttribute::Dex, 8 });

        // SkillBehaviorType::Dash was already fully implemented (SkillSystem::ActivateSkill,
        // UISystem's cyan icon color) but no gem used it -- an alternate, independently-
        // cooldowned dash to slot alongside the base Roll action, matching PoE's "Flame Dash".
        // Deals no damage, so it's intentionally excluded from SkillGemSystem's element display.
        SkillData flameDash;
        flameDash.name = "Flame Dash";
        flameDash.behaviorType = SkillBehaviorType::Dash;
        flameDash.cooldownTime = 3.5f;
        flameDash.mpCost = 15;
        flameDash.duration = 0.3f;
        flameDash.isValid = true;
        gems.push_back({ 12, flameDash, GemAttribute::Dex, 8 });

        // Third defensive Aura, parallel to Determination(Armour)/Discipline(ES): rounds
        // out the trio of PoE2's classic defense-layer auras with Evasion.
        SkillData grace;
        grace.name = "Grace";
        grace.behaviorType = SkillBehaviorType::Aura;
        grace.spiritCost = 35.0f;
        grace.auraEffect = { AffixStat::FlatEvasion, 50.0f, 1, true, "Evasion" };
        grace.isValid = true;
        gems.push_back({ 13, grace, GemAttribute::Int, 8 });

        // Second Fire/Chaos skill each, so both attribute-2 elements reach the same
        // 2-gem count Cold already has, and to add Melee/AreaEffect elemental variety
        // (the only Melee gem so far, Cleave, was Physical-only).
        SkillData immolate;
        immolate.name = "Immolate";
        immolate.behaviorType = SkillBehaviorType::Melee;
        immolate.cooldownTime = 0.8f;
        immolate.mpCost = 10;
        immolate.damage = 100.0f; // % of atk
        immolate.range = 65.0f;
        immolate.element = DamageElement::Fire;
        immolate.isValid = true;
        gems.push_back({ 14, immolate, GemAttribute::Str, 8 });

        SkillData soulRend;
        soulRend.name = "Soul Rend";
        soulRend.behaviorType = SkillBehaviorType::AreaEffect;
        soulRend.cooldownTime = 7.0f;
        soulRend.mpCost = 32;
        soulRend.damage = 95.0f; // % of atk, matches Nova/Fireball convention
        soulRend.range = 170.0f;
        soulRend.element = DamageElement::Chaos;
        soulRend.isValid = true;
        gems.push_back({ 15, soulRend, GemAttribute::Int, 8 });

        return gems;
    }
};
