#pragma once
#include <array>
#include <optional>
#include "Item.h"
#include "Components/Stats/CharacterStats/CharacterStats.h"

struct EquipmentComponent {
    std::array<std::optional<ItemComponent>, static_cast<size_t>(EquipSlot::Count)> slots;
    CharacterStatsComponent baseStats;
};
