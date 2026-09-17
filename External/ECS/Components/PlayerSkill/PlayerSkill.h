#pragma once
#include "../Stats/SkillData/Skill.h"
#include <array>
#include <optional>

struct PlayerSkill {
    // 装備中のスキルリスト (live/derived -- rebuilt from equippedItems[i] via
    // SkillGemScaling::BuildEquippedSkillData whenever the item or its sockets change)
    std::array<SkillData, 5> skills;

    // The actual bag item (ItemComponent, category==SkillGem) backing each skill slot --
    // std::nullopt means empty. This is the "source of truth" moved out of
    // InventoryComponent::items when equipped (see SkillGemSystem), and moved back in when
    // unequipped; skills[i] above is just its derived live stats.
    std::array<std::optional<ItemComponent>, 5> equippedItems;

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