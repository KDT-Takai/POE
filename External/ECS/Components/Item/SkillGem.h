#pragma once
#include <vector>

struct SkillGemPickupComponent {
    int gemId = 0;
};

struct SkillGemInventoryComponent {
    std::vector<int> unlockedGemIds;
};
