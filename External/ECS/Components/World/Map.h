#pragma once
#include <vector>
#include <cstdint>
#include <SFML/System/Vector2.hpp>

// タイルの種類
enum class TileType : uint8_t {
    Air = 0,        // 空気
    Dirt = 1,       // 土
    Stone = 2,      // 石
    Bedrock = 3,    // 岩盤
    Wood = 4,
    Grass = 5
};

struct MapComponent {
    // マップの大きさ
    int width = 0;
    int height = 0;

    // ピクセルサイズ
    float tileSize = 32.0f;

    std::vector<uint8_t> tiles;
    
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

    sf::Vector2i WorldToTile(float worldX, float worldY) const {
        return sf::Vector2i(
            static_cast<int>(worldX / tileSize),
            static_cast<int>(worldY / tileSize)
        );
    }
};