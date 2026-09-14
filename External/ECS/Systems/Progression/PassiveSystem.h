#pragma once
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "Components/Item/Item.h"
#include "Components/Item/Equipment.h"
#include <random>
#include <string>
#include <vector>
#include <algorithm>

class PassiveSystem {
public:
    static std::string ApplyRandomPassive(EquipmentComponent& equipment, std::mt19937& rng) {
        auto pool = Pool();
        std::uniform_int_distribution<int> pick(0, static_cast<int>(pool.size()) - 1);
        const auto& opt = pool[pick(rng)];

        ApplyToBase(equipment.baseStats, opt);

        int displayValue = static_cast<int>(opt.value);
        return "Passive: +" + std::to_string(displayValue) + (opt.isPercent ? "% " : " ") + opt.label;
    }

private:
    struct PassiveOption {
        AffixStat stat;
        float value;
        std::string label;
        bool isPercent;
    };

    static std::vector<PassiveOption> Pool() {
        return {
            { AffixStat::FlatLife, 10.0f, "Life", false },
            { AffixStat::FlatMana, 10.0f, "Mana", false },
            { AffixStat::FlatES, 6.0f, "Energy Shield", false },
            { AffixStat::FireRes, 5.0f, "Fire Resistance", true },
            { AffixStat::ColdRes, 5.0f, "Cold Resistance", true },
            { AffixStat::LightningRes, 5.0f, "Lightning Resistance", true },
            { AffixStat::FlatArmour, 8.0f, "Armour", false },
            { AffixStat::FlatEvasion, 8.0f, "Evasion", false },
            { AffixStat::CritChance, 1.0f, "Critical Chance", true },
            { AffixStat::MoveSpeed, 3.0f, "Movement Speed", true },
            { AffixStat::IncreasedAttackDamage, 5.0f, "Attack Damage", true },
        };
    }

    static void ApplyToBase(CharacterStatsComponent& base, const PassiveOption& opt) {
        switch (opt.stat) {
        case AffixStat::FlatLife: base.maxHP += opt.value; break;
        case AffixStat::FlatMana: base.maxMP += opt.value; break;
        case AffixStat::FlatES: base.maxES += opt.value; break;
        case AffixStat::FireRes: base.fireRes = (std::min)(0.75f, base.fireRes + opt.value / 100.0f); break;
        case AffixStat::ColdRes: base.iceRes = (std::min)(0.75f, base.iceRes + opt.value / 100.0f); break;
        case AffixStat::LightningRes: base.lightningRes = (std::min)(0.75f, base.lightningRes + opt.value / 100.0f); break;
        case AffixStat::FlatArmour: base.armour += opt.value; break;
        case AffixStat::FlatEvasion: base.evasion += opt.value; break;
        case AffixStat::CritChance: base.critRate += opt.value / 100.0f; break;
        case AffixStat::MoveSpeed: base.moveSpeed *= (1.0f + opt.value / 100.0f); break;
        case AffixStat::IncreasedAttackDamage: base.atk *= (1.0f + opt.value / 100.0f); break;
        default: break;
        }
    }
};
