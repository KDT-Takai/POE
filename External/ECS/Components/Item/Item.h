#pragma once
#include <string>
#include <vector>

enum class AffixStat {
    FlatLife, FlatMana, FlatES,
    IncreasedAttackDamage,
    FireRes, ColdRes, LightningRes, ChaosRes,
    FlatArmour, FlatEvasion, FlatAccuracy,
    CritChance, CritMultiplier, MoveSpeed,
    BlockChance,
    // Grants MaxSpirit like any other flat roll -- Spirit gems' auraEffect already used
    // this same ApplyAffix/RemoveAffix pipeline, but MaxSpirit itself had no source other
    // than EntitySpawner's fixed starting value until now (see SpiritAuraSystem::
    // ReevaluateReservations for what happens when equipment granting this is removed).
    FlatSpirit
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

// Gear (equippable, uses `slot`/`affixes`) vs Waystone (map-opening item, uses
// `waystoneTier`/`waystoneMods` instead -- `slot`/`affixes` are unused and left at their
// defaults). Kept as one ItemComponent type rather than a separate component so Waystones
// can live in the same InventoryComponent/StashComponent grids and flow through the same
// pickup/sell/buyback code paths as gear, per the "held exactly like a normal item" request.
enum class ItemCategory { Gear, Waystone };

constexpr int kMaxWaystoneTier = 15;

// Map modifiers rolled onto a Waystone when it's generated (see ItemFactory::GenerateWaystone).
// Unlike ItemAffix (always a player-stat buff applied via EquipmentSystem::ApplyAffix),
// these describe how the MAP itself is changed when that Waystone is consumed to open one
// (monster stat multipliers applied in ZoneBuilder, or a temporary player debuff applied
// only while inside that map) -- a deliberately separate, smaller type since the two only
// share a passing resemblance (stat + value + label).
enum class WaystoneModStat {
    MonsterIncreasedLife,
    MonsterIncreasedDamage,
    MonsterIncreasedElementalResistance,
    PlayerReducedElementalResistance,
    IncreasedItemRarity,
    IncreasedItemQuantity,
};

struct WaystoneMod {
    WaystoneModStat stat = WaystoneModStat::MonsterIncreasedLife;
    float value = 0.0f;
    std::string label = "";
};

struct ItemComponent {
    std::string baseName = "Item";
    ItemCategory category = ItemCategory::Gear;
    EquipSlot slot = EquipSlot::Weapon; // Gear only
    ItemRarity rarity = ItemRarity::Normal;
    int itemLevel = 1;
    float baseValue = 0.0f; // weapon: flat attack; armour pieces: flat armour
    std::vector<ItemAffix> affixes; // Gear only

    int waystoneTier = 0; // Waystone only, 1-15
    std::vector<WaystoneMod> waystoneMods; // Waystone only

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
