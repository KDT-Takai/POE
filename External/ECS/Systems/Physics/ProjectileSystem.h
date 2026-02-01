#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Physics/Projectile/Projectile.h"

class ProjectileSystem {
public:
    void Update(Registry& registry, float dt) {
        // ProjectileComponentを持つすべてのエンティティを取得
        auto view = registry.View<ProjectileComponent>();

        // 削除予定のエンティティリスト
        std::vector<unsigned int> entitiesToDestroy;

        for (auto entity : view) {
            auto& proj = registry.GetComponent<ProjectileComponent>(entity);

            // 寿命を減らす
            proj.duration -= dt;

            // 寿命が尽きたら削除リストへ
            if (proj.duration <= 0.0f) {
                entitiesToDestroy.push_back(entity);
            }
        }

        // 実際に削除実行
        for (auto entity : entitiesToDestroy) {
            registry.DestroyEntity(entity);
        }
    }
};