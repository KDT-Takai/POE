#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include <SFML/System/Vector2.hpp>
#include "HazardGround.h"

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
    // プレイヤーが実際に歩いて可視範囲に入ったタイルの記録(0/1、tilesと同じ添字)。
    // ミニマップ/Tab全体マップ(MinimapSystem)はこれが立っているタイルだけを描画する
    // (PoE2同様、未探索の場所は真っ暗のまま)。
    std::vector<uint8_t> visited;
    // 燃焼床/雷の床/氷の床/混沌ダメの床(HazardGroundKind、External/ECS/Components/World/
    // HazardGround.h)。0=なし、それ以外は`static_cast<uint8_t>(kind)+1`。タイルの属性
    // として持たせる(タイルと同じ添字、円形エンティティではない)ことで、他のタイル種別
    // (Dirt/Stone等)と同じ「マス目に紐づく地形情報」として扱える。
    std::vector<uint8_t> hazardTiles;

    // マップサイズをリセットするヘルパー
    void Resize(int w, int h) {
        width = w;
        height = h;
        tiles.assign(width * height, static_cast<uint8_t>(TileType::Stone)); // デフォルトは壁(Stone)で埋める
        visited.assign(width * height, 0);
        hazardTiles.assign(width * height, 0);
    }

    bool IsVisited(int tx, int ty) const {
        if (tx < 0 || tx >= width || ty < 0 || ty >= height) return false;
        return visited[ty * width + tx] != 0;
    }

    // タイル全体を既訪問にする(タウンのミニマップ/全体マップは探索済みかどうかに
    // 関わらず常に全体表示にする、ユーザー指示対応。ZoneBuilder::Buildがタウン生成時
    // にのみ呼ぶ)。
    void RevealAll() {
        std::fill(visited.begin(), visited.end(), static_cast<uint8_t>(1));
    }

    // 中心(cx,cy)から半径radiusTiles(タイル単位、円形)を既訪問としてマークする。
    void RevealCircle(int cx, int cy, int radiusTiles) {
        int r2 = radiusTiles * radiusTiles;
        int minY = (std::max)(0, cy - radiusTiles);
        int maxY = (std::min)(height - 1, cy + radiusTiles);
        int minX = (std::max)(0, cx - radiusTiles);
        int maxX = (std::min)(width - 1, cx + radiusTiles);
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                int dx = x - cx, dy = y - cy;
                if (dx * dx + dy * dy <= r2) visited[y * width + x] = 1;
            }
        }
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

    HazardGroundKind GetHazard(int tx, int ty) const {
        if (tx < 0 || tx >= width || ty < 0 || ty >= height) return HazardGroundKind::None;
        uint8_t raw = hazardTiles[ty * width + tx];
        return raw == 0 ? HazardGroundKind::None : static_cast<HazardGroundKind>(raw - 1);
    }

    void SetHazard(int tx, int ty, HazardGroundKind kind) {
        if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
            hazardTiles[ty * width + tx] = kind == HazardGroundKind::None ? 0 : static_cast<uint8_t>(kind) + 1;
        }
    }

    // 座標変換系はそのまま...
    sf::Vector2i WorldToTile(float worldX, float worldY) const {
        return sf::Vector2i(static_cast<int>(worldX / tileSize), static_cast<int>(worldY / tileSize));
    }
};