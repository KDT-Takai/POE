#pragma once
#include <string>
#include <vector>

enum class AffixStat {
    FlatLife, FlatMana, FlatES,
    IncreasedAttackDamage,
    FireRes, ColdRes, LightningRes, ChaosRes,
    FlatArmour, FlatEvasion, FlatAccuracy,
    CritChance, CritMultiplier, MoveSpeed
};

struct ItemAffix {
    AffixStat stat = AffixStat::FlatLife;
    float value = 0.0f;
    int tier = 1;
    bool isPrefix = true;
    std::string label = "";
};

enum class EquipSlot : size_t { Weapon = 0, BodyArmour, Helmet, Gloves, Boots, Ring1, Ring2, Amulet, Belt, Count };

enum class ItemRarity { Normal, Magic, Rare, Unique };

struct ItemComponent {
    std::string baseName = "Item";
    EquipSlot slot = EquipSlot::Weapon;
    ItemRarity rarity = ItemRarity::Normal;
    int itemLevel = 1;
    float baseValue = 0.0f; // weapon: flat attack; armour pieces: flat armour
    std::vector<ItemAffix> affixes;

    // Top-left cell this item occupies in the owning InventoryComponent's grid
    // (see ItemUIHelpers::ItemGridSize for the WxH footprint by slot). -1 means
    // "not currently placed in a bag" (equipped, or a vendor's transient stock/
    // buyback list, neither of which persist a grid position).
    int gridCol = -1;
    int gridRow = -1;

    float PowerScore() const {
        float score = baseValue;
        for (const auto& a : affixes) score += a.value;
        return score;
    }
};
