#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>
#include <utility>
#include "Components/Item/Item.h" // kMaxWaystoneTier
#include "Components/Progression/Atlas.h"

// Pure functions over the Atlas grid (see AtlasComponent/AtlasSystem). Nodes are not
// stored anywhere -- (col, row) is procedurally mapped to a tier/screen position, and
// only which coordinates are *completed* is persisted. This is what makes the grid
// "infinite": there is no upper bound on col/row to ever run out of.
class AtlasData {
public:
    static constexpr float kSpacing = 80.0f;

    // col/row are biased into a positive range before packing so negative coordinates
    // (the grid extends in all 4 directions from the start node) don't collide.
    static constexpr int64_t kBias = 1'000'000;
    static constexpr int64_t kMul = 2'000'001; // > 2*kBias, keeps col/row from overlapping

    static int64_t PackKey(int col, int row) {
        return (static_cast<int64_t>(col) + kBias) * kMul + (static_cast<int64_t>(row) + kBias);
    }

    static void UnpackKey(int64_t key, int& col, int& row) {
        int64_t c = key / kMul;
        int64_t r = key % kMul;
        col = static_cast<int>(c - kBias);
        row = static_cast<int>(r - kBias);
    }

    // Tier grows with Chebyshev distance from the start node, capped at the game's
    // existing 15-tier ceiling (see Item.h) -- the grid keeps expanding forever, but
    // the actual map difficulty/reward saturates the same way real PoE2's Waystones do.
    static int TierForCoord(int col, int row) {
        int dist = (std::max)(std::abs(col), std::abs(row));
        return std::clamp(1 + dist, 1, kMaxWaystoneTier);
    }

    // Diamond/rotated-lattice layout (matches how PoE2's Atlas actually reads visually,
    // rather than a plain square grid).
    static void NodePos(int col, int row, float& x, float& y) {
        x = static_cast<float>(col - row) * kSpacing * 0.5f;
        y = static_cast<float>(col + row) * kSpacing * 0.5f;
    }

    static std::array<std::pair<int, int>, 4> Neighbors(int col, int row) {
        return { { {col + 1, row}, {col - 1, row}, {col, row + 1}, {col, row - 1} } };
    }

    static bool IsCompleted(const AtlasComponent& atlas, int col, int row) {
        if (col == 0 && row == 0) return true; // start node, always cleared
        int64_t key = PackKey(col, row);
        for (int64_t k : atlas.completedNodeKeys) {
            if (k == key) return true;
        }
        return false;
    }

    // A node stays clickable forever once unlocked (matches real PoE2: revealed Atlas
    // nodes can always be re-run, they don't lock again after one clear) -- this only
    // gates whether it's reachable AT ALL yet.
    static bool IsUnlocked(const AtlasComponent& atlas, int col, int row) {
        if (IsCompleted(atlas, col, row)) return true;
        for (const auto& [nc, nr] : Neighbors(col, row)) {
            if (IsCompleted(atlas, nc, nr)) return true;
        }
        return false;
    }

    static void MarkCompleted(AtlasComponent& atlas, int col, int row) {
        if (IsCompleted(atlas, col, row)) return;
        atlas.completedNodeKeys.push_back(PackKey(col, row));
    }

    // Furthest completed node's distance from the start -- AtlasSystem grows the visible
    // window as this grows, giving the "endlessly expanding" feel without ever having to
    // materialize more than a small window of nodes at once.
    static int FurthestCompletedDistance(const AtlasComponent& atlas) {
        int furthest = 0;
        for (int64_t key : atlas.completedNodeKeys) {
            int col, row;
            UnpackKey(key, col, row);
            furthest = (std::max)(furthest, (std::max)(std::abs(col), std::abs(row)));
        }
        return furthest;
    }
};
