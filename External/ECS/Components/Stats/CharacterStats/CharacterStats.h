#pragma once
#include <string>

struct CharacterStatsComponent {
    // Basic Info
    std::string name = "Unknown";
    int level = 1;
    float currentHP = 100.0f;
    float maxHP = 100.0f;
	float healthRegen = 3.0f;
    float currentMP = 200.0f;
    float maxMP = 200.0f;
    float manaRegen = 20.0f;

    // Primary Attributes (Passive Tree予定 時間なさそー)
    int str = 10; // Strength:MaxHPなどに影響
    int dex = 10; // Dexterity:AtkSpdなどに影響
    int intelligence = 10; // Intelligence:MaxMPなどに影響

    // Combat Stats
    float atk = 10.0f;
    float def = 0.0f;
    float atkSpd = 1.0f;
    float critRate = 0.05f;
    float critDamage = 1.50f;

    // Movement Stats
    float moveSpeed = 50.0f;

    float rollSpeed = 800.0f;       // 回避中の移動速度 (通常移動の約4倍)
    float rollDuration = 0.35f;     // 回避の持続時間 (秒)
    float rollCooldownMax = 2.0f;   // クールダウンの最大時間 (秒)
    float rollCooldownTimer = 0.0f; // 現在のクールダウン残り時間 (0なら使用可能)

    // Resistances (Max 75%)
    float fireRes = 0.0f;
    float iceRes = 0.0f;
    float lightningRes = 0.0f;
};