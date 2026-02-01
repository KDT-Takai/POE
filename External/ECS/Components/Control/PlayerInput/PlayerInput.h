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

    std::array<bool, MAX_SKILL_SLOTS> skillInputs = { false, false, false, false, false };

    sf::Vector2f mouseWorldPos = { 0.0f, 0.0f };
};