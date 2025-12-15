#pragma once
#include <ECS.h>

class EntitySpawner {
public:
    // --------------------------------------------------------
    // 敵を作成する関数
    // --------------------------------------------------------
    static EntityObject CreateEnemy(Registry& registry, sf::Vector2f position) {
        // EntityObjectを使って生成
        EntityObject enemy = registry.CreateEntityObject();

        enemy.AddComponent<TagComponent>({ "Enemy" });

        // Transform (位置は引数で受け取るようにすると柔軟)
        enemy.AddComponent<TransformComponent>({ position, {1.f, 1.f}, 0.0f });

        // Visual
        enemy.AddComponent<CircleComponent>({ 25.0f, sf::Color::Red, true });

        // 将来的にステータスなどもここで追加
        // enemy.AddComponent<StatsComponent>({ 100, 10 }); // HP:100, Atk:10

        return enemy; // 作ったエンティティを返す（エディタで選択するため）
    }
};