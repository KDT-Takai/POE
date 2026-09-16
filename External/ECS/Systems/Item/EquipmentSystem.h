#pragma once
#include "Components/Item/Equipment.h"
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include <algorithm>

class EquipmentSystem {
public:
    static void RecalculateStats(CharacterStatsComponent& live, EquipmentComponent& equipment) {
        float savedCurrentHP = live.currentHP;
        float savedCurrentMP = live.currentMP;
        float savedCurrentES = live.currentES;
        int savedLevel = live.level;
        int savedXP = live.currentXP;
        int savedXPToNext = live.xpToNextLevel;
        int savedGold = live.gold;
        int savedPassivePoints = live.passivePoints;
        int savedRegretOrbs = live.regretOrbs;

        live = equipment.baseStats;

        live.level = savedLevel;
        live.currentXP = savedXP;
        live.xpToNextLevel = savedXPToNext;
        live.gold = savedGold;
        live.passivePoints = savedPassivePoints;
        live.regretOrbs = savedRegretOrbs;

        for (auto& slotItem : equipment.slots) {
            if (!slotItem.has_value()) continue;
            const ItemComponent& item = *slotItem;

            if (item.slot == EquipSlot::Weapon) {
                live.atk += item.baseValue;
            } else {
                live.armour += item.baseValue;
            }

            for (const auto& affix : item.affixes) {
                ApplyAffix(live, affix);
            }
        }

        // Increased/Reduced-style modifiers are summed above (increasedAttackDamage/
        // increasedMoveSpeed) rather than compounded per-source; apply the combined
        // percentage as a single multiplier here (PoE-style stacking, see AI/DECISIONS.md).
        live.atk *= (1.0f + live.increasedAttackDamage / 100.0f);
        live.moveSpeed *= (1.0f + live.increasedMoveSpeed / 100.0f);

        live.maxHP = (std::max)(1.0f, live.maxHP);
        live.currentHP = (std::min)(savedCurrentHP, live.maxHP);
        live.currentMP = (std::min)(savedCurrentMP, live.maxMP);
        live.currentES = (std::min)(savedCurrentES, live.maxES);
    }

    // Returns true if the item was equipped (replacing/filling that slot); false if discarded as weaker.
    static bool TryEquip(CharacterStatsComponent& live, EquipmentComponent& equipment, const ItemComponent& item) {
        size_t idx = static_cast<size_t>(item.slot);
        auto& current = equipment.slots[idx];

        if (current.has_value() && current->PowerScore() >= item.PowerScore()) {
            return false;
        }

        current = item;
        RecalculateStats(live, equipment);
        return true;
    }

    static void ApplyAffix(CharacterStatsComponent& live, const ItemAffix& affix) {
        switch (affix.stat) {
        case AffixStat::FlatLife: live.maxHP += affix.value; break;
        case AffixStat::FlatMana: live.maxMP += affix.value; break;
        case AffixStat::FlatES: live.maxES += affix.value; break;
        case AffixStat::IncreasedAttackDamage: live.increasedAttackDamage += affix.value; break;
        case AffixStat::FireRes: live.fireRes = (std::min)(0.75f, live.fireRes + affix.value / 100.0f); break;
        case AffixStat::ColdRes: live.iceRes = (std::min)(0.75f, live.iceRes + affix.value / 100.0f); break;
        case AffixStat::LightningRes: live.lightningRes = (std::min)(0.75f, live.lightningRes + affix.value / 100.0f); break;
        case AffixStat::ChaosRes: live.chaosRes = (std::min)(0.75f, live.chaosRes + affix.value / 100.0f); break;
        case AffixStat::FlatArmour: live.armour += affix.value; break;
        case AffixStat::FlatEvasion: live.evasion += affix.value; break;
        case AffixStat::FlatAccuracy: live.accuracy += affix.value; break;
        case AffixStat::CritChance: live.critRate += affix.value / 100.0f; break;
        case AffixStat::CritMultiplier: live.critDamage += affix.value / 100.0f; break;
        case AffixStat::MoveSpeed: live.increasedMoveSpeed += affix.value; break;
        case AffixStat::BlockChance: live.blockChance = (std::min)(0.75f, live.blockChance + affix.value / 100.0f); break;
        case AffixStat::FlatSpirit: live.maxSpirit += affix.value; break;
        }
    }

    // Exact inverse of ApplyAffix; must be called with the same affix used to apply.
    static void RemoveAffix(CharacterStatsComponent& live, const ItemAffix& affix) {
        switch (affix.stat) {
        case AffixStat::FlatLife: live.maxHP -= affix.value; break;
        case AffixStat::FlatMana: live.maxMP -= affix.value; break;
        case AffixStat::FlatES: live.maxES -= affix.value; break;
        case AffixStat::IncreasedAttackDamage: live.increasedAttackDamage -= affix.value; break;
        // No floor at 0 here: baseline resistance can legitimately be negative (see the
        // per-act campaign penalty in CampaignManager), and flooring would silently wipe
        // that penalty out whenever a resistance-granting passive/aura is removed.
        case AffixStat::FireRes: live.fireRes -= affix.value / 100.0f; break;
        case AffixStat::ColdRes: live.iceRes -= affix.value / 100.0f; break;
        case AffixStat::LightningRes: live.lightningRes -= affix.value / 100.0f; break;
        case AffixStat::ChaosRes: live.chaosRes -= affix.value / 100.0f; break;
        case AffixStat::FlatArmour: live.armour -= affix.value; break;
        case AffixStat::FlatEvasion: live.evasion -= affix.value; break;
        case AffixStat::FlatAccuracy: live.accuracy -= affix.value; break;
        case AffixStat::CritChance: live.critRate -= affix.value / 100.0f; break;
        case AffixStat::CritMultiplier: live.critDamage -= affix.value / 100.0f; break;
        case AffixStat::MoveSpeed: live.increasedMoveSpeed -= affix.value; break;
        case AffixStat::BlockChance: live.blockChance -= affix.value / 100.0f; break;
        case AffixStat::FlatSpirit: live.maxSpirit -= affix.value; break;
        }
    }
};
