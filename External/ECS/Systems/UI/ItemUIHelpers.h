#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "Components/Item/Item.h"

namespace ItemUIHelpers {
    // Grid footprint in bag cells, referenced against real PoE2 inventory sizing
    // (one-handed weapons/body armour take up multiple cells, rings/amulets are 1x1).
    inline sf::Vector2i ItemGridSize(EquipSlot slot) {
        switch (slot) {
        case EquipSlot::Weapon: return { 1, 3 };
        case EquipSlot::BodyArmour: return { 2, 3 };
        case EquipSlot::Helmet: return { 2, 2 };
        case EquipSlot::Gloves: return { 2, 2 };
        case EquipSlot::Boots: return { 2, 2 };
        case EquipSlot::Belt: return { 2, 1 };
        case EquipSlot::Ring1:
        case EquipSlot::Ring2: return { 1, 1 };
        case EquipSlot::Amulet: return { 1, 1 };
        default: return { 1, 1 };
        }
    }

    // Short label for a cramped grid cell. A whole short word, not a substr() of
    // SlotName()/RarityName() -- those are UTF-8 multibyte strings, so byte-slicing
    // them (e.g. .substr(0, 2)) cuts a character in half and renders as tofu boxes.
    inline std::string ShortSlotCode(EquipSlot slot) {
        switch (slot) {
        case EquipSlot::Weapon: return "武器";
        case EquipSlot::BodyArmour: return "胴";
        case EquipSlot::Helmet: return "兜";
        case EquipSlot::Gloves: return "手袋";
        case EquipSlot::Boots: return "靴";
        case EquipSlot::Ring1:
        case EquipSlot::Ring2: return "指輪";
        case EquipSlot::Amulet: return "首飾";
        case EquipSlot::Belt: return "帯";
        default: return "?";
        }
    }

    constexpr int kBagGridCols = 6;
    constexpr int kBagGridRows = 4;

    // True if the WxH region at (col,row) doesn't run off the grid or overlap any
    // already-placed item (skipping ignoreIndex, e.g. the item currently being dragged).
    inline bool BagRegionFree(const std::vector<ItemComponent>& items, int col, int row, int w, int h, int ignoreIndex = -1) {
        if (col < 0 || row < 0 || col + w > kBagGridCols || row + h > kBagGridRows) return false;
        for (size_t i = 0; i < items.size(); ++i) {
            if (static_cast<int>(i) == ignoreIndex) continue;
            const ItemComponent& other = items[i];
            if (other.gridCol < 0 || other.gridRow < 0) continue;
            sf::Vector2i sz = ItemGridSize(other.slot);
            bool overlapX = col < other.gridCol + sz.x && col + w > other.gridCol;
            bool overlapY = row < other.gridRow + sz.y && row + h > other.gridRow;
            if (overlapX && overlapY) return false;
        }
        return true;
    }

    // First (row-major) free spot for a w x h item. Returns false if the bag has no
    // room, e.g. an "inventory full" condition now depends on shape, not just count.
    inline bool FindBagFreeSpace(const std::vector<ItemComponent>& items, int w, int h, int& outCol, int& outRow) {
        for (int row = 0; row <= kBagGridRows - h; ++row) {
            for (int col = 0; col <= kBagGridCols - w; ++col) {
                if (BagRegionFree(items, col, row, w, h)) {
                    outCol = col;
                    outRow = row;
                    return true;
                }
            }
        }
        return false;
    }

    // Assigns bag grid positions to any item that doesn't have one yet (gridCol < 0):
    // freshly loaded saves, or an item added by a code path that forgot to place it.
    // Shared by InventorySystem and VendorSystem since both render the same bag.
    inline void NormalizeBagPlacement(std::vector<ItemComponent>& items) {
        for (auto& item : items) {
            if (item.gridCol >= 0 && item.gridRow >= 0) continue;
            sf::Vector2i sz = ItemGridSize(item.slot);
            int col, row;
            if (FindBagFreeSpace(items, sz.x, sz.y, col, row)) {
                item.gridCol = col;
                item.gridRow = row;
            }
        }
    }

    // Simple left-to-right, top-to-bottom shelf packing for a transient list that has
    // no persisted placement (vendor stock/buyback) -- just lays items out in order.
    inline std::vector<sf::Vector2i> ShelfPack(const std::vector<sf::Vector2i>& sizes, int gridCols) {
        std::vector<sf::Vector2i> positions(sizes.size());
        int cursorCol = 0, cursorRow = 0, rowHeight = 0;
        for (size_t i = 0; i < sizes.size(); ++i) {
            int w = sizes[i].x, h = sizes[i].y;
            if (cursorCol + w > gridCols) {
                cursorCol = 0;
                cursorRow += rowHeight;
                rowHeight = 0;
            }
            positions[i] = { cursorCol, cursorRow };
            cursorCol += w;
            rowHeight = (std::max)(rowHeight, h);
        }
        return positions;
    }
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

    // Whether an affix's value is a percentage (show "%") vs. a flat number (show plain).
    // Matches EquipmentSystem::ApplyAffix's handling of each AffixStat 1:1.
    inline bool IsPercentAffix(AffixStat stat) {
        switch (stat) {
        case AffixStat::IncreasedAttackDamage:
        case AffixStat::FireRes:
        case AffixStat::ColdRes:
        case AffixStat::LightningRes:
        case AffixStat::ChaosRes:
        case AffixStat::CritChance:
        case AffixStat::CritMultiplier:
        case AffixStat::MoveSpeed:
            return true;
        default:
            return false;
        }
    }

    // Canonical Japanese name for a stat, derived purely from AffixStat rather than
    // trusting ItemAffix::label -- CampaignManager's save format never serializes
    // `label` (only stat/value/tier/isPrefix), so any equipped/bagged item that has
    // gone through a save/load round-trip has an empty label. Deriving it here fixes
    // display for both fresh and already-saved items without touching the save format.
    inline std::string AffixLabel(AffixStat stat) {
        switch (stat) {
        case AffixStat::FlatLife: return "生命力";
        case AffixStat::FlatMana: return "マナ";
        case AffixStat::FlatES: return "エナジーシールド";
        case AffixStat::IncreasedAttackDamage: return "攻撃ダメージ増加";
        case AffixStat::FireRes: return "火耐性";
        case AffixStat::ColdRes: return "冷気耐性";
        case AffixStat::LightningRes: return "電気耐性";
        case AffixStat::ChaosRes: return "カオス耐性";
        case AffixStat::FlatArmour: return "アーマー";
        case AffixStat::FlatEvasion: return "回避力";
        case AffixStat::FlatAccuracy: return "命中率";
        case AffixStat::CritChance: return "クリティカル率";
        case AffixStat::CritMultiplier: return "クリティカルダメージ";
        case AffixStat::MoveSpeed: return "移動速度";
        default: return "?";
        }
    }

    // "火耐性 +10%" / "マナ +10" style line, matching real PoE2's stat-name-first mod
    // display (label first, then the signed value, rounded to a whole number since
    // PoE2 doesn't show decimals on these). Shared so Inventory/Vendor/CharacterSheet
    // render mods identically.
    inline std::string FormatAffixLine(const ItemAffix& affix) {
        long rounded = std::lround(affix.value);
        std::string sign = (rounded >= 0) ? "+" : "";
        std::string unit = IsPercentAffix(affix.stat) ? "%" : "";
        return AffixLabel(affix.stat) + " " + sign + std::to_string(rounded) + unit;
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

    // Cosmetic-only socket count (PoE2-style: only weapons/armour pieces have sockets,
    // jewelry never does). There's no rune/gem-in-socket mechanic implemented yet -- this
    // is purely a visual indicator of the item derived from its rarity, not a system.
    inline int SocketCount(const ItemComponent& item) {
        switch (item.slot) {
        case EquipSlot::Weapon:
        case EquipSlot::BodyArmour:
        case EquipSlot::Helmet:
        case EquipSlot::Gloves:
        case EquipSlot::Boots:
            break;
        default:
            return 0;
        }
        switch (item.rarity) {
        case ItemRarity::Normal: return 0;
        case ItemRarity::Magic: return 1;
        case ItemRarity::Rare: return 2;
        case ItemRarity::Unique: return 2;
        default: return 0;
        }
    }

    // Small empty circles along a cell's bottom edge representing SocketCount(). Shapes
    // only (no text), so this doesn't need a font and can be shared as a free function.
    inline void DrawSocketPips(sf::RenderTarget& target, float x, float y, int count) {
        constexpr float kPipRadius = 3.0f;
        constexpr float kPipGap = 3.0f;
        for (int i = 0; i < count; ++i) {
            sf::CircleShape pip(kPipRadius);
            pip.setPosition({ x + static_cast<float>(i) * (kPipRadius * 2.0f + kPipGap), y });
            pip.setFillColor(sf::Color(40, 40, 45));
            pip.setOutlineColor(sf::Color(210, 210, 220));
            pip.setOutlineThickness(1.0f);
            target.draw(pip);
        }
    }
}
