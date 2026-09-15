#pragma once
#include "Components/Item/Currency.h"
#include "Components/Item/Equipment.h"
#include "ItemFactory.h"
#include "EquipmentSystem.h"
#include <random>
#include <string>
#include <algorithm>

class CurrencySystem {
public:
    static std::string ApplyCurrency(EquipmentComponent& equipment, CharacterStatsComponent& live,
        CurrencyType type, int itemLevel, std::mt19937& rng) {

        if (type == CurrencyType::Regret) {
            live.regretOrbs++;
            return CurrencyLabel(type) + " (" + std::to_string(live.regretOrbs) + " held)";
        }

        int targetIdx = FindEligibleSlot(equipment, type);
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
        case CurrencyType::Alchemy: {
            EquipSlot slot = item.slot;
            std::string keptName = item.baseName;
            item = ItemFactory::GenerateItem(slot, ItemRarity::Rare, itemLevel, rng);
            item.baseName = keptName;
            break;
        }
        case CurrencyType::Chance: {
            EquipSlot slot = item.slot;
            std::string keptName = item.baseName;
            ItemRarity rolled = ItemFactory::RollRarity(rng);
            item = ItemFactory::GenerateItem(slot, rolled, itemLevel, rng);
            item.baseName = keptName;
            break;
        }
        case CurrencyType::Augmentation: {
            // Magic items always start with exactly one affix (see ItemFactory::GenerateItem);
            // add the missing side (prefix if the existing one is a suffix, or vice versa).
            bool hasPrefix = std::any_of(item.affixes.begin(), item.affixes.end(), [](const ItemAffix& a) { return a.isPrefix; });
            item.affixes.push_back(ItemFactory::RollAffixOfType(!hasPrefix, itemLevel, rng));
            break;
        }
        case CurrencyType::Annulment: {
            std::uniform_int_distribution<int> pick(0, static_cast<int>(item.affixes.size()) - 1);
            item.affixes.erase(item.affixes.begin() + pick(rng));
            break;
        }
        case CurrencyType::Scouring:
            item.rarity = ItemRarity::Normal;
            item.affixes.clear();
            break;
        default:
            break;
        }

        EquipmentSystem::RecalculateStats(live, equipment);
        return CurrencyLabel(type) + " -> " + item.baseName;
    }

private:
    // Finds the first equipped item eligible for this currency's effect. Returns -1 if none.
    static int FindEligibleSlot(EquipmentComponent& equipment, CurrencyType type) {
        for (size_t i = 0; i < equipment.slots.size(); ++i) {
            if (!equipment.slots[i].has_value()) continue;
            const ItemComponent& item = *equipment.slots[i];

            bool eligible = false;
            switch (type) {
            case CurrencyType::Transmutation:
            case CurrencyType::Alchemy:
            case CurrencyType::Chance:
                eligible = (item.rarity == ItemRarity::Normal);
                break;
            case CurrencyType::Regal:
                eligible = (item.rarity == ItemRarity::Magic);
                break;
            case CurrencyType::Augmentation:
                eligible = (item.rarity == ItemRarity::Magic) && (item.affixes.size() < 2);
                break;
            case CurrencyType::Chaos:
                eligible = (item.rarity == ItemRarity::Rare);
                break;
            case CurrencyType::Annulment:
                eligible = (item.rarity == ItemRarity::Magic || item.rarity == ItemRarity::Rare) && !item.affixes.empty();
                break;
            case CurrencyType::Scouring:
                eligible = (item.rarity != ItemRarity::Normal);
                break;
            default:
                break;
            }

            if (eligible) return static_cast<int>(i);
        }
        return -1;
    }

public:
    static std::string CurrencyLabel(CurrencyType t) {
        switch (t) {
        case CurrencyType::Transmutation: return "Orb of Transmutation";
        case CurrencyType::Regal: return "Regal Orb";
        case CurrencyType::Chaos: return "Chaos Orb";
        case CurrencyType::Regret: return "Orb of Regret";
        case CurrencyType::Alchemy: return "Orb of Alchemy";
        case CurrencyType::Augmentation: return "Orb of Augmentation";
        case CurrencyType::Annulment: return "Orb of Annulment";
        case CurrencyType::Chance: return "Orb of Chance";
        case CurrencyType::Scouring: return "Orb of Scouring";
        default: break;
        }
        return "Currency";
    }
};
