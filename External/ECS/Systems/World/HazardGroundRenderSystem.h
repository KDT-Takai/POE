#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <algorithm>
#include "../../Registry/Registry.h"
#include "../../Components/World/Map.h"
#include "System/Time/Time.h"

// 燃焼床/雷の床/氷の床/混沌ダメの床(MapComponent::hazardTiles)の見た目。GameScene::Render
// がMapRenderSystemの直後・キャラクター描画(RenderSystem)の前に呼ぶことで、床デカールの
// 上にキャラクターが重なって見えるようにする。MapRenderSystemと同じく画面内のタイルだけ
// を1本のsf::VertexArrayにまとめて1回のdraw callで描く(タイル毎の個別draw callは
// 既存コードが「処理落ちした」と明記している理由と同じ)。ワールド座標系(カメラ追従)の
// sf::Viewが設定されている間に呼ぶこと。
class HazardGroundRenderSystem {
public:
    void Render(Registry& registry, sf::RenderTarget& target) {
        auto mapView = registry.View<MapComponent>();
        if (mapView.empty()) return;
        auto& map = registry.GetComponent<MapComponent>(mapView[0]);
        if (map.width <= 0) return;

        sf::View currentView = target.getView();
        sf::Vector2f center = currentView.getCenter();
        sf::Vector2f size = currentView.getSize();
        float viewLeft = center.x - size.x / 2.0f;
        float viewTop = center.y - size.y / 2.0f;
        float viewRight = center.x + size.x / 2.0f;
        float viewBottom = center.y + size.y / 2.0f;

        int startX = (std::max)(0, static_cast<int>(viewLeft / map.tileSize) - 1);
        int endX = (std::min)(map.width, static_cast<int>(viewRight / map.tileSize) + 1);
        int startY = (std::max)(0, static_cast<int>(viewTop / map.tileSize) - 1);
        int endY = (std::min)(map.height, static_cast<int>(viewBottom / map.tileSize) + 1);

        float t = static_cast<float>(Time::Instance().GetTotalTime());
        float ts = map.tileSize;

        sf::VertexArray quads(sf::PrimitiveType::Triangles);
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                HazardGroundKind kind = map.GetHazard(x, y);
                if (kind == HazardGroundKind::None) continue;

                sf::Color color = ColorFor(kind);
                // タイル座標を位相のシードに使い、隣接タイルが同時にパチパチしないように
                // 明滅をずらす(HazardGroundComponent::pulsePhaseの代わり、タイルには
                // 個体差データを持たせていないため)。
                float phase = static_cast<float>(x) * 0.7f + static_cast<float>(y) * 1.3f;
                float pulse = 0.5f + 0.5f * std::sin(t * 2.4f + phase);
                color.a = static_cast<std::uint8_t>(100.0f + pulse * 70.0f);

                float px = static_cast<float>(x) * ts;
                float py = static_cast<float>(y) * ts;
                sf::Vector2f p0(px, py);
                sf::Vector2f p1(px + ts, py);
                sf::Vector2f p2(px + ts, py + ts);
                sf::Vector2f p3(px, py + ts);
                quads.append(sf::Vertex{ p0, color });
                quads.append(sf::Vertex{ p1, color });
                quads.append(sf::Vertex{ p2, color });
                quads.append(sf::Vertex{ p0, color });
                quads.append(sf::Vertex{ p2, color });
                quads.append(sf::Vertex{ p3, color });
            }
        }
        if (quads.getVertexCount() > 0) target.draw(quads);
    }

private:
    static sf::Color ColorFor(HazardGroundKind kind) {
        switch (kind) {
        case HazardGroundKind::Burning: return sf::Color(255, 90, 20);
        case HazardGroundKind::Shocked: return sf::Color(255, 230, 40);
        case HazardGroundKind::Chilled: return sf::Color(120, 220, 255);
        case HazardGroundKind::Caustic: return sf::Color(150, 40, 200);
        default: return sf::Color::Transparent;
        }
    }
};
