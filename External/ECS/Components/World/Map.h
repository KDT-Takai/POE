#pragma once
#include <vector>
#include <cstdint>
#include <SFML/System/Vector2.hpp>

enum class TileType : uint8_t {
    Air = 0,        // 何もない（今回は使わないかも）
    Dirt = 1,       // 床 (Floor)
    Stone = 2,      // 壁 (Wall)
    Bedrock = 3,    // 岩盤 (境界線など)
    Wood = 4,       // スタート地点の目印
    Grass = 5       // ゴール地点の目印
};

struct MapComponent {
    int width = 0;
    int height = 0;
    float tileSize = 64.0f;

    std::vector<uint8_t> tiles;

    // マップサイズをリセットするヘルパー
    void Resize(int w, int h) {
        width = w;
        height = h;
        tiles.assign(width * height, static_cast<uint8_t>(TileType::Stone)); // デフォルトは壁(Stone)で埋める
    }

    TileType GetTile(int tx, int ty) const {
        if (tx < 0 || tx >= width || ty < 0 || ty >= height) {
            return TileType::Bedrock;
        }
        return static_cast<TileType>(tiles[ty * width + tx]);
    }

    void SetTile(int tx, int ty, TileType type) {
        if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
            tiles[ty * width + tx] = static_cast<uint8_t>(type);
        }
    }

    // 座標変換系はそのまま...
    sf::Vector2i WorldToTile(float worldX, float worldY) const {
        return sf::Vector2i(static_cast<int>(worldX / tileSize), static_cast<int>(worldY / tileSize));
    }
};