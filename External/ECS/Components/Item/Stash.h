#pragma once
#include "Item.h"
#include <vector>

// Persistent item storage, separate from the carried InventoryComponent, accessible via
// the Stash NPC in town (see StashSystem/ZoneBuilder). Bigger than the bag since its only
// purpose is holding overflow loot, not being carried into combat. Reuses ItemComponent's
// gridCol/gridRow exactly like InventoryComponent does, just against this grid's own
// (larger) dimensions instead of ItemUIHelpers::kBagGridCols/Rows.
struct StashComponent {
    static constexpr int kCols = 10;
    static constexpr int kRows = 8;
    static constexpr size_t kCapacity = static_cast<size_t>(kCols) * static_cast<size_t>(kRows);
    std::vector<ItemComponent> items;
};
