#pragma once
#include "../Stats/SkillData/Skill.h"
#include <array>

struct PlayerSkill {
    // 装備中のスキルリスト
    std::array<SkillData, 5> skills;

    // 現在詠唱中かどうか
    int castingSkillIndex = -1;
    float castTimer = 0.0f;

    PlayerSkill() {
        for (auto& skill : skills) {
            skill.name = "Empty";
            skill.behaviorType = SkillBehaviorType::Melee;
        }
    }
};