#pragma once
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "Components/Item/Equipment.h"
#include <string>
#include <cmath>

class LevelSystem {
public:
    static int CalcXPToNextLevel(int level) {
        return static_cast<int>(100.0f * std::pow(static_cast<float>(level), 1.5f));
    }

    // level/XP counters live on the live stats component; permanent stat growth is
    // applied to equipment.baseStats so it survives future EquipmentSystem::RecalculateStats calls.
    static bool GrantXP(CharacterStatsComponent& live, EquipmentComponent& equipment, int amount, std::string& outMessage) {
        live.currentXP += amount;
        bool leveledUp = false;
        while (live.currentXP >= live.xpToNextLevel) {
            live.currentXP -= live.xpToNextLevel;
            live.level++;
            live.xpToNextLevel = CalcXPToNextLevel(live.level);
            live.passivePoints++;

            equipment.baseStats.maxHP += 8.0f;
            equipment.baseStats.maxMP += 6.0f;
            equipment.baseStats.atk += 1.5f;

            leveledUp = true;
        }
        if (leveledUp) {
            outMessage = "LEVEL UP! Lv." + std::to_string(live.level) + " (+1 Passive Point - P to spend)";
        }
        return leveledUp;
    }

    static int CalcKillXP(const CharacterStatsComponent& enemy) {
        int base = static_cast<int>(enemy.maxHP * 0.3f) + 5;
        switch (enemy.rarity) {
        case MonsterRarity::Magic: base = static_cast<int>(base * 2.0f); break;
        case MonsterRarity::Rare: base = static_cast<int>(base * 4.0f); break;
        case MonsterRarity::Unique: base = static_cast<int>(base * 10.0f); break;
        default: break;
        }
        return base;
    }
};
