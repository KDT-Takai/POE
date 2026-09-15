#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Components/Item/Item.h"

namespace ItemUIHelpers {
    inline std::string SlotName(EquipSlot slot) {
        switch (slot) {
        case EquipSlot::Weapon: return "Weapon";
        case EquipSlot::BodyArmour: return "Body Armour";
        case EquipSlot::Helmet: return "Helmet";
        case EquipSlot::Gloves: return "Gloves";
        case EquipSlot::Boots: return "Boots";
        case EquipSlot::Ring1: return "Ring 1";
        case EquipSlot::Ring2: return "Ring 2";
        case EquipSlot::Amulet: return "Amulet";
        case EquipSlot::Belt: return "Belt";
        default: return "Slot";
        }
    }

    inline std::string RarityName(ItemRarity rarity) {
        switch (rarity) {
        case ItemRarity::Normal: return "Normal";
        case ItemRarity::Magic: return "Magic";
        case ItemRarity::Rare: return "Rare";
        case ItemRarity::Unique: return "Unique";
        default: return "?";
        }
    }

    inline sf::Color RarityColor(ItemRarity rarity) {
        switch (rarity) {
        case ItemRarity::Normal: return sf::Color(220, 220, 220);
        case ItemRarity::Magic: return sf::Color(120, 150, 255);
        case ItemRarity::Rare: return sf::Color(255, 210, 60);
        case ItemRarity::Unique: return sf::Color(200, 130, 40);
        default: return sf::Color::White;
        }
    }
}
