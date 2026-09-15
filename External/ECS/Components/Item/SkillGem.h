#pragma once
#include <vector>
#include <array>

struct SkillGemPickupComponent {
    int gemId = 0;
};

struct SkillGemInventoryComponent {
    std::vector<int> unlockedGemIds;
};

// Two Spirit slots, separate from the 5 activated skill slots in PlayerSkill.
// -1 = empty. See SpiritAuraSystem for assignment/effect application.
struct SpiritGemLoadoutComponent {
    std::array<int, 2> auraGemIds = { -1, -1 };
};
