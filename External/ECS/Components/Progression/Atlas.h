#pragma once
#include <vector>
#include <cstdint>

// PoE2-style Atlas: an unbounded grid of map nodes the player reveals outward from the
// start node (0,0). Only completed node coordinates are stored (packed via
// AtlasData::PackKey) -- tier, screen position, and unlock state are all derived on
// demand from AtlasData, so this component never needs migrating as the grid
// conceptually grows.
struct AtlasComponent {
    std::vector<int64_t> completedNodeKeys;
};
