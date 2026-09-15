#pragma once
#include <vector>

// Marks a monster as a summoner archetype: while the player is within
// triggerRange, periodically spawns a weaker add near itself, up to
// maxActiveSummons alive at once (see EnemySummonSystem). Movement/aggro
// otherwise follows the default melee chase in EnemyAISystem.
struct SummonerComponent {
    float triggerRange = 260.0f;
    float summonInterval = 5.0f;
    float currentCooldown = 0.0f;
    int maxActiveSummons = 2;
    float statScale = 0.5f; // summoned add's HP/atk relative to itself
    std::vector<unsigned int> activeSummonIds;
};
