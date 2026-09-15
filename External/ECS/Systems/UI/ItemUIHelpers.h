#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Components/Item/Item.h"

namespace ItemUIHelpers {
    inline std::string SlotName(EquipSlot slot) {
        switch (slot) {
        case EquipSlot::Weapon: return "武器";
        case EquipSlot::BodyArmour: return "胴防具";
        case EquipSlot::Helmet: return "兜";
        case EquipSlot::Gloves: return "手袋";
        case EquipSlot::Boots: return "靴";
        case EquipSlot::Ring1: return "指輪1";
        case EquipSlot::Ring2: return "指輪2";
        case EquipSlot::Amulet: return "首飾り";
        case EquipSlot::Belt: return "ベルト";
        default: return "スロット";
        }
    }

    inline std::string RarityName(ItemRarity rarity) {
        switch (rarity) {
        case ItemRarity::Normal: return "ノーマル";
        case ItemRarity::Magic: return "マジック";
        case ItemRarity::Rare: return "レア";
        case ItemRarity::Unique: return "ユニーク";
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
