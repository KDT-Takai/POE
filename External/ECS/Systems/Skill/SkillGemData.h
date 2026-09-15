#pragma once
#include "Components/Stats/SkillData/Skill.h"
#include <vector>

struct GemDefinition {
    int id;
    SkillData skill;
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
        gems.push_back({ 0, spark });

        SkillData slam;
        slam.name = "Thunder Slam";
        slam.behaviorType = SkillBehaviorType::GroundSlam;
        slam.cooldownTime = 5.0f;
        slam.mpCost = 35;
        slam.damage = 120.0f; // % of atk
        slam.element = DamageElement::Physical;
        slam.isValid = true;
        gems.push_back({ 1, slam });

        SkillData warp;
        warp.name = "Lightning Warp";
        warp.behaviorType = SkillBehaviorType::LightningWarp;
        warp.range = 350.0f;
        warp.damage = 80.0f;
        warp.cooldownTime = 8.0f;
        warp.mpCost = 35;
        warp.element = DamageElement::Lightning;
        warp.isValid = true;
        gems.push_back({ 2, warp });

        SkillData ball;
        ball.name = "Lightning Ball";
        ball.behaviorType = SkillBehaviorType::LightningBall;
        ball.damage = 40.0f; // % of atk, per bullet (fires 12)
        ball.cooldownTime = 12.0f;
        ball.mpCost = 40;
        ball.element = DamageElement::Lightning;
        ball.isValid = true;
        gems.push_back({ 3, ball });

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
        gems.push_back({ 4, warcry });

        SkillData nova;
        nova.name = "Nova";
        nova.behaviorType = SkillBehaviorType::AreaEffect;
        nova.cooldownTime = 6.0f;
        nova.mpCost = 30;
        nova.damage = 90.0f; // % of atk, matches LightningWarp's convention
        nova.range = 180.0f; // blast radius-ish size
        nova.element = DamageElement::Cold;
        nova.isValid = true;
        gems.push_back({ 5, nova });

        SkillData cleave;
        cleave.name = "Cleave";
        cleave.behaviorType = SkillBehaviorType::Melee;
        cleave.cooldownTime = 0.6f;
        cleave.mpCost = 8;
        cleave.damage = 110.0f; // % of atk
        cleave.range = 70.0f;
        cleave.element = DamageElement::Physical;
        cleave.isValid = true;
        gems.push_back({ 6, cleave });

        SkillData frostBolt;
        frostBolt.name = "Frost Bolt";
        frostBolt.behaviorType = SkillBehaviorType::Projectile;
        frostBolt.cooldownTime = 0.5f;
        frostBolt.mpCost = 6;
        frostBolt.damage = 140.0f; // % of atk, single-target so higher than AoE skills
        frostBolt.element = DamageElement::Cold;
        frostBolt.isValid = true;
        gems.push_back({ 7, frostBolt });

        return gems;
    }
};
