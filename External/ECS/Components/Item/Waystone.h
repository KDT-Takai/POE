#pragma once
#include <array>

// PoE2-style endgame currency: a Waystone opens one instance of an endgame map at its
// tier (1-15). Unlike crafting Currency (Currency.h), which applies instantly on
// pickup, Waystones are held and spent deliberately at the endgame hub's map device.
struct WaystoneInventoryComponent {
    static constexpr int kMaxTier = 15;
    std::array<int, kMaxTier> counts{}; // counts[0] = tier 1 ... counts[kMaxTier-1] = tier 15
};

struct WaystonePickupComponent {
    int tier = 1;
};
