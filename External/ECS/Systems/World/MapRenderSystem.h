#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <array>
#include "../../Registry/Registry.h"
#include "../../Components/World/Map.h"
#include "TileTextureFactory.h"

class MapRenderSystem {
public:
    void Render(Registry& registry, sf::RenderTarget& target) {
        // MapComponentを持つエンティティを探す（通常は1つだけ）
        auto view = registry.View<MapComponent>();
        if (view.empty()) return;

        // 最初の1つを取得（WorldEntity）
        const auto& map = registry.GetComponent<MapComponent>(view[0]);

        // 現在のカメラの表示範囲を取得
        sf::View currentView = target.getView();
        sf::Vector2f center = currentView.getCenter();
        sf::Vector2f size = currentView.getSize();

        // 画面の左上と右下のワールド座標を計算
        float viewLeft = center.x - size.x / 2.0f;
        float viewTop = center.y - size.y / 2.0f;
        float viewRight = center.x + size.x / 2.0f;
        float viewBottom = center.y + size.y / 2.0f;

        int startX = std::max(0, static_cast<int>(viewLeft / map.tileSize) - 1);
        int endX = std::min(map.width, static_cast<int>(viewRight / map.tileSize) + 1);
        int startY = std::max(0, static_cast<int>(viewTop / map.tileSize) - 1);
        int endY = std::min(map.height, static_cast<int>(viewBottom / map.tileSize) + 1);

        int tileSizeInt = static_cast<int>(map.tileSize);

        // One vertex batch per TileType instead of a separate draw call per visible tile
        // ("処理落ちした" -- at the current tile density this used to be several hundred+
        // individual sf::RectangleShape draws every frame). Same procedurally-textured
        // look (TileTextureFactory), just assembled as 2 triangles per tile appended into
        // a shared sf::VertexArray so the whole visible map costs at most 6 draw calls
        // (one per distinct TileType actually on screen) instead of one per tile.
        std::array<sf::VertexArray, 6> batches;
        for (auto& va : batches) va.setPrimitiveType(sf::PrimitiveType::Triangles);

        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                TileType type = map.GetTile(x, y);
                sf::IntRect uv = TileTextureFactory::TextureRectFor(x, y, tileSizeInt);

                float px = static_cast<float>(x) * map.tileSize;
                float py = static_cast<float>(y) * map.tileSize;
                float ts = map.tileSize;

                sf::Vector2f p0(px, py);
                sf::Vector2f p1(px + ts, py);
                sf::Vector2f p2(px + ts, py + ts);
                sf::Vector2f p3(px, py + ts);

                sf::Vector2f uv0(static_cast<float>(uv.position.x), static_cast<float>(uv.position.y));
                sf::Vector2f uv1(static_cast<float>(uv.position.x + uv.size.x), static_cast<float>(uv.position.y));
                sf::Vector2f uv2(static_cast<float>(uv.position.x + uv.size.x), static_cast<float>(uv.position.y + uv.size.y));
                sf::Vector2f uv3(static_cast<float>(uv.position.x), static_cast<float>(uv.position.y + uv.size.y));

                sf::VertexArray& va = batches[static_cast<size_t>(type)];
                va.append(sf::Vertex{ p0, sf::Color::White, uv0 });
                va.append(sf::Vertex{ p1, sf::Color::White, uv1 });
                va.append(sf::Vertex{ p2, sf::Color::White, uv2 });
                va.append(sf::Vertex{ p0, sf::Color::White, uv0 });
                va.append(sf::Vertex{ p2, sf::Color::White, uv2 });
                va.append(sf::Vertex{ p3, sf::Color::White, uv3 });
            }
        }

        for (size_t t = 0; t < batches.size(); ++t) {
            if (batches[t].getVertexCount() == 0) continue;
            sf::RenderStates states(&TileTextureFactory::Get(static_cast<TileType>(t)));
            target.draw(batches[t], states);
        }
    }
};