#pragma once
#include <vector>

struct PoisonStack {
    float damagePerSecond = 0.0f;
    float remaining = 0.0f;
};

struct StatusEffectsComponent {
    float igniteDps = 0.0f;
    float igniteRemaining = 0.0f;

    float bleedDps = 0.0f;
    float bleedRemaining = 0.0f;

    std::vector<PoisonStack> poisonStacks;

    float chillRemaining = 0.0f;
    float chillSlowPercent = 0.0f;

    float freezeRemaining = 0.0f;

    float shockRemaining = 0.0f;
    float shockIncreasedDamageTaken = 0.0f;

    float stunRemaining = 0.0f;

    bool IsHardLocked() const { return freezeRemaining > 0.0f || stunRemaining > 0.0f; }
};
