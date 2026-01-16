#pragma once
#include <array>

// スキルスロットの数
constexpr int MAX_SKILL_SLOTS = 5;

struct PlayerInputComponent {
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool roll = false;
    bool attack = false;

    // スキルは最大５個まで対応させる
	// スキルスロットの最大数
    std::array<bool, MAX_SKILL_SLOTS> skillInputs = { false, false, false, false, false };
};