#pragma once
#include <SFML/System/Vector2.hpp>

// What clicking a town NPC opens (see ZoneBuilder for spawning, GameScene for the
// click/proximity handling and which UI system each kind maps to).
enum class TownNpcKind { ItemVendor, WaystoneVendor, Stash };

struct TownNpcSpawn {
    TownNpcKind kind;
    sf::Vector2f pos;
};
