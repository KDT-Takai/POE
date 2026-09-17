#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "Components/Item/Item.h"
#include "../Skill/SkillGemData.h"
#include "../Skill/SupportGemData.h"

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

    // Category-aware entry point -- prefer this one everywhere an actual ItemComponent
    // (not just a bare EquipSlot) is at hand, so Waystones (which don't have a meaningful
    // `slot`) always get their own compact footprint instead of whatever `slot` happens to
    // be left at.
    inline sf::Vector2i ItemGridSize(const ItemComponent& item) {
        if (item.category == ItemCategory::Waystone || item.category == ItemCategory::SkillGem) return { 1, 1 };
        return ItemGridSize(item.slot);
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
    // cols/rows default to the player's bag size; StashSystem passes its own (larger)
    // grid dimensions to reuse this same placement logic for a differently-sized grid.
    inline bool BagRegionFree(const std::vector<ItemComponent>& items, int col, int row, int w, int h, int ignoreIndex = -1,
        int cols = kBagGridCols, int rows = kBagGridRows) {
        if (col < 0 || row < 0 || col + w > cols || row + h > rows) return false;
        for (size_t i = 0; i < items.size(); ++i) {
            if (static_cast<int>(i) == ignoreIndex) continue;
            const ItemComponent& other = items[i];
            if (other.gridCol < 0 || other.gridRow < 0) continue;
            sf::Vector2i sz = ItemGridSize(other);
            bool overlapX = col < other.gridCol + sz.x && col + w > other.gridCol;
            bool overlapY = row < other.gridRow + sz.y && row + h > other.gridRow;
            if (overlapX && overlapY) return false;
        }
        return true;
    }

    // First (row-major) free spot for a w x h item. Returns false if the grid has no
    // room, e.g. an "inventory full" condition now depends on shape, not just count.
    inline bool FindBagFreeSpace(const std::vector<ItemComponent>& items, int w, int h, int& outCol, int& outRow,
        int cols = kBagGridCols, int rows = kBagGridRows) {
        for (int row = 0; row <= rows - h; ++row) {
            for (int col = 0; col <= cols - w; ++col) {
                if (BagRegionFree(items, col, row, w, h, -1, cols, rows)) {
                    outCol = col;
                    outRow = row;
                    return true;
                }
            }
        }
        return false;
    }

    // Assigns grid positions to any item that doesn't have one yet (gridCol < 0):
    // freshly loaded saves, or an item added by a code path that forgot to place it.
    // Shared by InventorySystem/VendorSystem (bag) and StashSystem (stash grid).
    inline void NormalizeBagPlacement(std::vector<ItemComponent>& items, int cols = kBagGridCols, int rows = kBagGridRows) {
        for (auto& item : items) {
            if (item.gridCol >= 0 && item.gridRow >= 0) continue;
            sf::Vector2i sz = ItemGridSize(item);
            int col, row;
            if (FindBagFreeSpace(items, sz.x, sz.y, col, row, cols, rows)) {
                item.gridCol = col;
                item.gridRow = row;
            }
        }
    }

    // Moves items[index] to (targetCol,targetRow) within the SAME container: a no-op if
    // it's already there, moves it if the target region is free, swaps with whatever
    // occupies it if that's a single same-size item, otherwise leaves it untouched. This
    // is what lets the player freely rearrange a grid instead of every placement being
    // forced to the first free slot (see AI/DECISIONS.md "自由配置").
    inline void TryMoveWithinGrid(std::vector<ItemComponent>& items, int index, int targetCol, int targetRow,
        int cols = kBagGridCols, int rows = kBagGridRows) {
        if (index < 0 || index >= static_cast<int>(items.size())) return;
        ItemComponent& dragItem = items[index];
        sf::Vector2i sz = ItemGridSize(dragItem);

        if (targetCol == dragItem.gridCol && targetRow == dragItem.gridRow) return;

        if (BagRegionFree(items, targetCol, targetRow, sz.x, sz.y, index, cols, rows)) {
            dragItem.gridCol = targetCol;
            dragItem.gridRow = targetRow;
            return;
        }

        for (size_t i = 0; i < items.size(); ++i) {
            if (static_cast<int>(i) == index) continue;
            ItemComponent& other = items[i];
            if (other.gridCol != targetCol || other.gridRow != targetRow) continue;
            sf::Vector2i otherSz = ItemGridSize(other);
            if (otherSz.x == sz.x && otherSz.y == sz.y) {
                std::swap(dragItem.gridCol, other.gridCol);
                std::swap(dragItem.gridRow, other.gridRow);
            }
            return;
        }
    }

    // Moves `from[index]` into `to` at the exact (targetCol,targetRow) the player dropped
    // it on -- if that region is free, or swaps positions with whatever single same-size
    // item occupies it there. Returns false (leaving both containers untouched) if
    // neither applies, e.g. dropped on a differently-shaped item -- callers should show a
    // message rather than silently falling back to "first free slot" like this used to.
    inline bool TryMoveBetweenGrids(std::vector<ItemComponent>& from, std::vector<ItemComponent>& to, int index,
        int targetCol, int targetRow, int toCols = kBagGridCols, int toRows = kBagGridRows) {
        if (index < 0 || index >= static_cast<int>(from.size())) return false;
        ItemComponent dragItem = from[index];
        sf::Vector2i sz = ItemGridSize(dragItem);

        if (BagRegionFree(to, targetCol, targetRow, sz.x, sz.y, -1, toCols, toRows)) {
            dragItem.gridCol = targetCol;
            dragItem.gridRow = targetRow;
            to.push_back(dragItem);
            from.erase(from.begin() + index);
            return true;
        }

        for (size_t i = 0; i < to.size(); ++i) {
            ItemComponent& other = to[i];
            if (other.gridCol != targetCol || other.gridRow != targetRow) continue;
            sf::Vector2i otherSz = ItemGridSize(other);
            if (otherSz.x != sz.x || otherSz.y != sz.y) return false;

            ItemComponent movedIn = dragItem;
            movedIn.gridCol = other.gridCol;
            movedIn.gridRow = other.gridRow;
            ItemComponent movedOut = other;
            movedOut.gridCol = dragItem.gridCol;
            movedOut.gridRow = dragItem.gridRow;

            to.erase(to.begin() + i);
            from.erase(from.begin() + index);
            to.push_back(movedIn);
            from.push_back(movedOut);
            return true;
        }
        return false;
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
        case AffixStat::BlockChance:
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
        case AffixStat::BlockChance: return "ブロック率";
        case AffixStat::FlatSpirit: return "スピリット";
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

    inline std::string WaystoneModStatLabel(WaystoneModStat stat) {
        switch (stat) {
        case WaystoneModStat::MonsterIncreasedLife: return "Monsters have increased Life";
        case WaystoneModStat::MonsterIncreasedDamage: return "Monsters deal increased Damage";
        case WaystoneModStat::MonsterIncreasedElementalResistance: return "Monsters have increased Elemental Resistance";
        case WaystoneModStat::PlayerReducedElementalResistance: return "Players have reduced Elemental Resistances";
        case WaystoneModStat::IncreasedItemRarity: return "Increased Rarity of Items found in this Area";
        case WaystoneModStat::IncreasedItemQuantity: return "Increased Quantity of Items found in this Area";
        default: return "?";
        }
    }

    // "+25% Monsters deal increased Damage" style line, matching FormatAffixLine's
    // sign-then-label convention for Waystone map mods instead of player-stat affixes.
    inline std::string FormatWaystoneModLine(const WaystoneMod& mod) {
        long rounded = std::lround(mod.value);
        std::string sign = (rounded >= 0) ? "+" : "";
        return sign + std::to_string(rounded) + "% " + WaystoneModStatLabel(mod.stat);
    }

    // Compact grid-cell label: gear shows "Lv{itemLevel}", Waystones show "T{tier}"
    // instead (itemLevel is repurposed on a Waystone purely for its SellValue formula,
    // not a meaningful "level" to show the player).
    inline std::string CompactLevelLabel(const ItemComponent& item) {
        if (item.category == ItemCategory::Waystone) return "T" + std::to_string(item.waystoneTier);
        if (item.category == ItemCategory::SkillGem) {
            if (!item.skillGemIdentified) return "?";
            if (item.skillGemIsSupport) return "Su";
            return "Lv" + std::to_string(item.skillGemLevel);
        }
        return "Lv" + std::to_string(item.itemLevel);
    }

    inline const char* GemPickupKindLabel(GemPickupKind kind) {
        switch (kind) {
        case GemPickupKind::Support: return "Support";
        case GemPickupKind::Spirit: return "Spirit";
        default: return "Skill";
        }
    }

    // A SkillGem-category item's display name: uncut gems show "Uncut {Kind} Gem",
    // identified ones show the actual skill/support name from its catalog -- so a bag
    // cell/tooltip never has to special-case where the name comes from.
    inline std::string SkillGemDisplayName(const ItemComponent& item) {
        if (!item.skillGemIdentified) {
            return std::string("Uncut ") + GemPickupKindLabel(item.skillGemUncutKind) + " Gem";
        }
        if (item.skillGemIsSupport) {
            const SupportGemDefinition* def = SupportGemData::Find(item.skillGemId);
            return def ? def->name : "Support Gem";
        }
        const GemDefinition* def = SkillGemData::Find(item.skillGemId);
        return def ? def->skill.name : "Skill Gem";
    }

    // Tooltip header line: gear shows "Slot - Rarity - iLvl N - WxH", Waystones show
    // their tier instead, SkillGems show their kind/level (slot/rarity/size are all
    // meaningless for either).
    inline std::string ItemTypeLine(const ItemComponent& item) {
        if (item.category == ItemCategory::Waystone) {
            return "Waystone - Tier " + std::to_string(item.waystoneTier);
        }
        if (item.category == ItemCategory::SkillGem) {
            if (!item.skillGemIdentified) {
                return std::string("Uncut ") + GemPickupKindLabel(item.skillGemUncutKind) + " Gem - Lv" + std::to_string(item.skillGemLevel)
                    + " (right-click to identify)";
            }
            if (item.skillGemIsSupport) return "Support Gem";
            return "Skill Gem - Lv" + std::to_string(item.skillGemLevel);
        }
        sf::Vector2i sz = ItemGridSize(item);
        return SlotName(item.slot) + " - " + RarityName(item.rarity) + " - iLvl " + std::to_string(item.itemLevel)
            + " - " + std::to_string(sz.x) + "x" + std::to_string(sz.y);
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
        if (item.category != ItemCategory::Gear) return 0;
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
