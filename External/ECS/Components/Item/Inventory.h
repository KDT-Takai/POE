#pragma once
#include "Item.h"
#include <vector>

struct InventoryComponent {
    static constexpr size_t kCapacity = 24;
    std::vector<ItemComponent> items;
};
