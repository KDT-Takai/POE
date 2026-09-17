#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
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

        sf::RectangleShape rect(sf::Vector2f(map.tileSize, map.tileSize));
        rect.setFillColor(sf::Color::White); // texture shows through undimmed
        int tileSizeInt = static_cast<int>(map.tileSize);

        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                TileType type = map.GetTile(x, y);

                // 座標設定
                rect.setPosition(sf::Vector2{ x * map.tileSize, y * map.tileSize });

                // procedurally-painted texture per tile type (see TileTextureFactory --
                // no hand-authored tile art exists yet). A different window of the same
                // noise atlas is sampled per cell so same-type tiles don't visibly repeat.
                rect.setTexture(&TileTextureFactory::Get(type));
                rect.setTextureRect(TileTextureFactory::TextureRectFor(x, y, tileSizeInt));

                target.draw(rect);
            }
        }
    }
};