#pragma once
#include <string>

struct CharacterStatsComponent {
    // Basic Info
    std::string name = "Unknown";
    int level = 1;
    float currentHP = 100.0f;
    float maxHP = 100.0f;
    float currentMP = 200.0f;
    float maxMP = 200.0f;

    // Primary Attributes (Passive Tree—\’è ŠÔ‚È‚³‚»[)
    int str = 0; // Strength:MaxHP‚È‚Ç‚É‰e‹¿
    int dex = 0; // Dexterity:AtkSpd‚È‚Ç‚É‰e‹¿
    int intelligence = 0; // Intelligence:MaxMP‚È‚Ç‚É‰e‹¿

    // Combat Stats
    float atk = 10.0f;
    float def = 0.0f;
    float atkSpd = 1.0f;
    float critRate = 0.05f;
    float critDamage = 1.50f;

    // Movement Stats
    float moveSpeed = 200.0f;
    // ‚ ‚Æ‚ÅƒWƒƒƒ“ƒv—Í’Ç‰Á‚·‚é

    // Resistances (Max 75%)
    float fireRes = 0.0f;
    float iceRes = 0.0f;
    float lightningRes = 0.0f;
};