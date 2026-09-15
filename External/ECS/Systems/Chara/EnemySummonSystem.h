#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Chara/Summoner.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "Game/GameScene/Entity/EntitySpawner.h"
#include <algorithm>
#include <cmath>
#include <random>

// Periodically spawns a weaker add near a monster with a SummonerComponent
// while the player is within range, up to maxActiveSummons alive at once.
class EnemySummonSystem {
public:
    void Update(Registry& registry, float dt, sf::Vector2f playerPos) {
        for (auto entity : registry.View<SummonerComponent, CharacterStatsComponent, TransformComponent>()) {
            if (registry.HasComponent<PlayerInputComponent>(entity)) continue;

            auto& summoner = registry.GetComponent<SummonerComponent>(entity);
            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);
            if (stats.currentHP <= 0.0f) continue;

            PruneDeadSummons(registry, summoner.activeSummonIds);

            if (summoner.currentCooldown > 0.0f) {
                summoner.currentCooldown -= dt;
                continue;
            }

            if (static_cast<int>(summoner.activeSummonIds.size()) >= summoner.maxActiveSummons) continue;

            auto& trans = registry.GetComponent<TransformComponent>(entity);
            sf::Vector2f diff = playerPos - trans.position;
            float distanceSq = diff.x * diff.x + diff.y * diff.y;
            if (distanceSq > summoner.triggerRange * summoner.triggerRange) continue;

            sf::Vector2f spawnPos = trans.position + RandomOffset();
            auto add = EntitySpawner::CreateEnemy(registry, spawnPos);

            auto& addStats = add.GetComponent<CharacterStatsComponent>();
            addStats.maxHP *= summoner.statScale;
            addStats.currentHP = addStats.maxHP;
            addStats.atk *= summoner.statScale;
            addStats.name = "召喚された" + stats.name;
            add.AddComponent(TagComponent{ addStats.name });

            summoner.activeSummonIds.push_back(add.GetID());
            summoner.currentCooldown = summoner.summonInterval;
        }
    }

private:
    void PruneDeadSummons(Registry& registry, std::vector<unsigned int>& ids) {
        ids.erase(std::remove_if(ids.begin(), ids.end(), [&](unsigned int id) {
            if (!registry.IsValid(id)) return true;
            if (!registry.HasComponent<CharacterStatsComponent>(id)) return true;
            return registry.GetComponent<CharacterStatsComponent>(id).currentHP <= 0.0f;
            }), ids.end());
    }

    sf::Vector2f RandomOffset() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
        std::uniform_real_distribution<float> radiusDist(30.0f, 60.0f);
        float angle = angleDist(gen);
        float radius = radiusDist(gen);
        return { std::cos(angle) * radius, std::sin(angle) * radius };
    }
};
