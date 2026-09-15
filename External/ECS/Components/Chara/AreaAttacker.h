#pragma once
#include <SFML/Graphics.hpp>

// Marks a monster as a melee AoE archetype: closes to triggerRange like a normal
// monster, then roots itself and telegraphs (see EnemyAreaAttackSystem) before
// dealing damage to the player if they are still within radius when the windup
// completes. Distinct from RangedAttackerComponent, which kites instead of closing.
struct AreaAttackerComponent {
    float triggerRange = 90.0f;
    float telegraphTime = 0.8f;
    float cooldownTime = 3.2f;
    float radius = 100.0f;
    float damagePercent = 90.0f; // % of atk, same convention as RangedAttackerComponent

    float currentCooldown = 0.0f;
    float currentTelegraph = -1.0f; // >= 0 while telegraphing, -1 when idle/cooling down
    sf::Color baseColor = sf::Color::Green; // restored on CircleComponent once the telegraph ends
};
