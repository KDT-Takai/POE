#pragma once
#include <ECS.h>
// プレイヤー用
#include "../ECS/Components/Tags/Player/Player.h"
#include "../ECS/Components/Physics/Transform/Transform.h"
#include "../ECS/Components/Physics/Velocity/Velocity.h"
#include "../ECS/Components/Physics/Gravity/Gravity.h"
#include "../ECS/Components/Physics/Facing/Facing.h"
#include "../ECS/Components/Physics/BoxCollider/BoxCollider.h"
#include "../ECS/Components/Control/PlayerInput/PlayerInput.h"
#include "../ECS/Components/Control/State/State.h"
#include "../ECS/Components/Stats/CharacterStats/CharacterStats.h"
#include "../ECS/Components/Interaction/Interaction.h"

class EntitySpawner {
public:
    static EntityObject CreatePlayer(Registry& registry, float startX, float startY) {
        // エンティティ生成（ラッパーオブジェクトを取得）
        auto entity = registry.CreateEntityObject();

        // タグ設定
        entity.AddComponent(PlayerTag{});

        // エディタで名前が見えるようにする（ECSのTagComponentを使用）
        // 提供コード内の struct TagComponent { string name; }; を使用
        entity.AddComponent(TagComponent{ "Player" });

        // 物理・座標設定
        entity.AddComponent(TransformComponent{
            sf::Vector2f(startX, startY),
            sf::Vector2f(1.f, 1.f),
            0.f
            });

        entity.AddComponent(VelocityComponent{ sf::Vector2f(0.f, 0.f) });
        entity.AddComponent(GravityComponent{ 980.0f });

        // 当たり判定
        entity.AddComponent(BoxColliderComponent{ 32.f, 64.f, 0.f, 0.f, false, false });

        // 制御・状態設定
        entity.AddComponent(PlayerInputComponent{});
        entity.AddComponent(StateComponent{ ActorState::Idle, 0.0f });

        //ステータス設定
        CharacterStatsComponent stats;
        stats.name = "Hero";
        stats.moveSpeed = 400.0f;
        stats.maxHP = 100.0f;
        stats.currentHP = stats.maxHP;
        entity.AddComponent(stats);

        // 描画設定（仮の見た目として緑色の円を設定）
        // RenderSystemがCircleComponentを描画できるため
        CircleComponent circleVis;
        circleVis.radius = 16.0f; // 幅32px相当
        circleVis.color = sf::Color::Green;
        circleVis.isVisible = true;
        entity.AddComponent(circleVis);

        // もしスプライトを使うならこちら
        /*
        SpriteComponent spriteVis;
        spriteVis.textureName = "PlayerIdle"; // ResourceManagerにロード済みである必要あり
        spriteVis.color = sf::Color::White;
        spriteVis.layer = 1; // 手前に表示
        entity.AddComponent(spriteVis);
        */

        return entity;
    }
    // 敵を作成
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
    // 精霊碑作成
    static Entity CreateMonument(Registry& registry, sf::Vector2f position, int spiritID) {
        Entity entity = registry.CreateEntity();

        TransformComponent trans;
        trans.position = position;
        registry.AddComponent(entity, trans);

        InteractableComponent interact;
        interact.type = InteractType::SpiritMonument;
        interact.targetID = spiritID;
        registry.AddComponent(entity, interact);

        return entity;
    }
};