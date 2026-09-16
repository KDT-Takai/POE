#pragma once
#include <SFML/Graphics.hpp>

// Marks a monster as a lunge/charge archetype: chases normally until within
// triggerRange, then roots and telegraphs (see EnemyChargeSystem) before bursting
// toward the player's position at chargeSpeedMultiplier for chargeDuration. Contact
// damage during the charge is handled by the normal player-enemy collision path
// (CollisionSystem) like any other trash monster -- no separate damage system needed,
// unlike RangedAttackerComponent/AreaAttackerComponent which fire their own projectile/AoE.
struct ChargerComponent {
    float triggerRange = 200.0f;
    float telegraphTime = 0.5f;
    float chargeDuration = 0.4f;
    float chargeSpeedMultiplier = 3.5f;
    float cooldownTime = 2.0f;

    float currentTelegraph = -1.0f; // >= 0 while telegraphing (rooted)
    float currentCharge = -1.0f;    // >= 0 while actively charging
    float currentCooldown = 0.0f;
    sf::Vector2f chargeDirection{ 0.0f, 0.0f }; // frozen at telegraph end, so the charge is a committed line
    sf::Color baseColor = sf::Color::Red; // restored on CircleComponent once the telegraph ends
};
