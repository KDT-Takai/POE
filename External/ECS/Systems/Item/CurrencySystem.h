#pragma once
#include "Components/Item/Currency.h"
#include "Components/Item/Equipment.h"
#include "ItemFactory.h"
#include "EquipmentSystem.h"
#include <random>
#include <string>

class CurrencySystem {
public:
    static std::string ApplyCurrency(EquipmentComponent& equipment, CharacterStatsComponent& live,
        CurrencyType type, int itemLevel, std::mt19937& rng) {

        if (type == CurrencyType::Regret) {
            live.regretOrbs++;
            return CurrencyLabel(type) + " (" + std::to_string(live.regretOrbs) + " held)";
        }

        ItemRarity requiredRarity = ItemRarity::Normal;
        switch (type) {
        case CurrencyType::Transmutation: requiredRarity = ItemRarity::Normal; break;
        case CurrencyType::Regal: requiredRarity = ItemRarity::Magic; break;
        case CurrencyType::Chaos: requiredRarity = ItemRarity::Rare; break;
        }

        int targetIdx = -1;
        for (size_t i = 0; i < equipment.slots.size(); ++i) {
            if (equipment.slots[i].has_value() && equipment.slots[i]->rarity == requiredRarity) {
                targetIdx = static_cast<int>(i);
                break;
            }
        }

        if (targetIdx < 0) {
            return CurrencyLabel(type) + ": no eligible item, discarded";
        }

        ItemComponent& item = *equipment.slots[targetIdx];
        switch (type) {
        case CurrencyType::Transmutation:
            item.rarity = ItemRarity::Magic;
            item.affixes.push_back(ItemFactory::RollRandomAffix(itemLevel, rng));
            break;
        case CurrencyType::Regal:
            item.rarity = ItemRarity::Rare;
            item.affixes.push_back(ItemFactory::RollRandomAffix(itemLevel, rng));
            break;
        case CurrencyType::Chaos: {
            EquipSlot slot = item.slot;
            std::string keptName = item.baseName;
            item = ItemFactory::GenerateItem(slot, ItemRarity::Rare, itemLevel, rng);
            item.baseName = keptName;
            break;
        }
        }

        EquipmentSystem::RecalculateStats(live, equipment);
        return CurrencyLabel(type) + " -> " + item.baseName;
    }

private:
    static std::string CurrencyLabel(CurrencyType t) {
        switch (t) {
        case CurrencyType::Transmutation: return "Orb of Transmutation";
        case CurrencyType::Regal: return "Regal Orb";
        case CurrencyType::Chaos: return "Chaos Orb";
        case CurrencyType::Regret: return "Orb of Regret";
        }
        return "Currency";
    }
};
