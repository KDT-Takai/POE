#pragma once
#include <algorithm>
#include <SFML/System/Vector2.hpp>
#include "../../Registry/Registry.h"
#include "../../Components/World/Map.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Components.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Combat/StatusEffects.h"
#include "../Combat/CombatMath.h"
#include "System/Campaign/CampaignManager.h"

// 燃焼床/雷の床/氷の床/混沌ダメの床(MapComponent::hazardTiles)の常時判定。タイル属性として
// 持たせているため(ZoneBuilder::PlaceHazardGrounds参照)、判定は「キャラクターが今いる
// タイルにハザードが乗っているか」だけで済む。効果の強さはエンドゲームマップのTier
// (CampaignManager::GetEndgameMapTier、既存のZoneBuilder/ApplyTierResistanceと同じ
// 1.0+0.15*(tier-1)の式)から都度算出する。プレイヤー/敵/ミニオン問わず陣営を区別しない
// (環境ギミックのため)。
//
// Shocked/Chilledは既存のCombatMath::AccumulateShock/AccumulateFreeze(PoE2の
// Elemental Ailment Threshold参考、耐性で軽減された後の値だけを蓄積するため耐性が
// 高いほど発症が遅れる)に乗せるだけで、実際の被ダメージ増加/移動速度低下自体は既存の
// CombatMath::ApplyDamage/StatusEffectSystemがそのまま処理する(通常の被弾によるShock/
// Chillと同じ土台、二重実装を避けている)。
class HazardGroundSystem {
public:
    void Update(Registry& registry, float dt) {
        auto mapView = registry.View<MapComponent>();
        if (mapView.empty()) return;
        auto& map = registry.GetComponent<MapComponent>(mapView[0]);
        if (map.width <= 0) return;

        float tierScale = 1.0f + 0.15f * static_cast<float>(CampaignManager::Instance().GetEndgameMapTier() - 1);

        for (auto target : registry.View<CharacterStatsComponent, TransformComponent, CircleComponent>()) {
            auto& stats = registry.GetComponent<CharacterStatsComponent>(target);
            if (stats.currentHP <= 0.0f) continue;

            auto& trans = registry.GetComponent<TransformComponent>(target);
            auto& circle = registry.GetComponent<CircleComponent>(target);
            sf::Vector2f center = trans.position + sf::Vector2f(circle.radius, circle.radius);
            sf::Vector2i tile = map.WorldToTile(center.x, center.y);
            HazardGroundKind kind = map.GetHazard(tile.x, tile.y);
            if (kind == HazardGroundKind::None) continue;

            switch (kind) {
            case HazardGroundKind::Burning: {
                float dealt = CombatMath::ApplyDamageOverTime(stats, 9.0f * tierScale * dt, DamageElement::Fire);
                if (dealt > 0.0f && registry.HasComponent<StatusEffectsComponent>(target)) {
                    // 継続的に燃焼床へ立っていると、床から離れた後も残るIgnite自体を
                    // 発症しうる(耐性で軽減された後のdealtをそのまま閾値へ積む)。
                    CombatMath::AccumulateIgnite(registry.GetComponent<StatusEffectsComponent>(target), dealt, stats.maxHP, false);
                }
                break;
            }
            case HazardGroundKind::Caustic:
                CombatMath::ApplyDamageOverTime(stats, 6.0f * tierScale * dt, DamageElement::Chaos);
                break;
            case HazardGroundKind::Shocked:
                if (registry.HasComponent<StatusEffectsComponent>(target)) {
                    float postRes = 26.0f * tierScale * (1.0f - CombatMath::GetResistance(stats, DamageElement::Lightning));
                    CombatMath::AccumulateShock(registry.GetComponent<StatusEffectsComponent>(target), (std::max)(0.0f, postRes) * dt, stats.maxHP, false);
                }
                break;
            case HazardGroundKind::Chilled:
                if (registry.HasComponent<StatusEffectsComponent>(target)) {
                    auto& status = registry.GetComponent<StatusEffectsComponent>(target);
                    float coldRes = CombatMath::GetResistance(stats, DamageElement::Cold);
                    // Chillのスロウ自体は乗っている間ずっと即時付与(耐性で効果量だけ減衰)。
                    status.chillRemaining = (std::max)(status.chillRemaining, 0.3f);
                    status.chillSlowPercent = (std::max)(status.chillSlowPercent, 0.3f * (1.0f - coldRes));
                    // Freeze(完全行動不能)は耐性で遅らせられる閾値蓄積モデルに乗せる。
                    float postRes = 22.0f * tierScale * (1.0f - coldRes);
                    CombatMath::AccumulateFreeze(status, (std::max)(0.0f, postRes) * dt, stats.maxHP, false);
                }
                break;
            default:
                break;
            }
        }
    }
};
