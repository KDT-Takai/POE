#pragma once
#include "../../Registry/Registry.h"
#include "Game/GameScene/Character/Enemy/EnemyFactory.h"
#include "../../ECS/Components/Control/PlayerInput/PlayerInput.h"
#include <random>
#include <cmath>

class EnemySpawnSystem {
public:
    // 設定
    float spawnTimer = 0.0f;
    float spawnInterval = 2.0f; // 2秒ごとに湧く
    int maxEnemies = 20;        // 最大20体まで
    float spawnRadiusMin = 300.0f; // プレイヤーから300px以上離す
    float spawnRadiusMax = 600.0f; // 600px以内

    void Update(Registry& registry, float dt, sf::Vector2f playerPos) {
        int enemyCount = 0;
        auto view = registry.View<CharacterStatsComponent>();
        for (auto entity : view) {
            if (!registry.HasComponent<PlayerInputComponent>(entity)) {
                enemyCount++;
            }
        }

        // 最大数なら湧かない
        if (enemyCount >= maxEnemies) return;

        // タイマー更新
        spawnTimer -= dt;
        if (spawnTimer <= 0.0f) {
            spawnTimer = spawnInterval;
            SpawnOneEnemy(registry, playerPos);
        }
    }

private:
    void SpawnOneEnemy(Registry& registry, sf::Vector2f centerPos) {
        // ランダムな位置を計算
        float angle = GetRandom(0.0f, 6.28318f);
        float dist = GetRandom(spawnRadiusMin, spawnRadiusMax);

        sf::Vector2f offset(std::cos(angle) * dist, std::sin(angle) * dist);
        sf::Vector2f spawnPos = centerPos + offset;

        EnemyFactory::CreateEnemy(registry, spawnPos);
    }

    float GetRandom(float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    }
};