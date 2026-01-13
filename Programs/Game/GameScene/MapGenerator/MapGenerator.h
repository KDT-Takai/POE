#pragma once
#include <ECS.h>
#include "../ECS/Components/World/Map.h"
#include <random>
#include <cmath>

class MapGenerator {
public:
    // テスト用
    static Entity CreateTestWorld(Registry& registry) {
        Entity entity = registry.CreateEntity();

        MapComponent map;
        map.width = 100;
        map.height = 30;
        map.tileSize = 32.0f;
        map.tiles.resize(map.width * map.height, (uint8_t)TileType::Air);

        // 地面
        for (int x = 0; x < map.width; ++x) {
            for (int y = map.height - 3; y < map.height; ++y) {
                map.SetTile(x, y, TileType::Dirt);
            }
        }

        // 石
        for (int y = map.height - 6; y < map.height - 3; ++y) {
            map.SetTile(20, y, TileType::Stone);
        }

        // 穴
        for (int x = 40; x < 45; ++x) {
            map.SetTile(x, map.height - 3, TileType::Air);
            map.SetTile(x, map.height - 1, TileType::Bedrock);
        }

        registry.AddComponent(entity, map);
        return entity;
    }
    // 試作品１号ちゃん
    static Entity CreateProceduralWorld(Registry& registry) {
        Entity entity = registry.CreateEntity();
        MapComponent map;
        map.width = 400;  // 幅400ブロック
        map.height = 100; // 高さ100ブロック
        map.tileSize = 32.0f;
        map.tiles.resize(map.width * map.height, (uint8_t)TileType::Air);

        // 乱数
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 100);

        std::vector<int> groundHeights(map.width);

        int currentHeight = map.height / 2;

        for (int x = 0; x < map.width; ++x) {
            int change = (dis(gen) % 3) - 1;
            currentHeight += change;

            if (currentHeight < 10) currentHeight = 10;
            if (currentHeight > map.height - 10) currentHeight = map.height - 10;

            groundHeights[x] = currentHeight;
        }

        for (int x = 0; x < map.width; ++x) {
            int surfaceY = groundHeights[x];
            for (int y = 0; y < map.height; ++y) {
                if (y < surfaceY) {
                    continue;
                }
                if (y == surfaceY) {
                    map.SetTile(x, y, TileType::Dirt); // 草ブロック的な扱い
                }
                else if (y > surfaceY && y < surfaceY + 5) {
                    map.SetTile(x, y, TileType::Dirt);
                }
                else {
                    if (dis(gen) < 10) {
                        map.SetTile(x, y, TileType::Dirt);
                    }
                    else {
                        map.SetTile(x, y, TileType::Stone);
                    }
                }
                if (y > surfaceY + 10) {
                    if (dis(gen) < 2) {
                        map.SetTile(x, y, TileType::Bedrock);
                    }
                }
            }
            map.SetTile(x, map.height - 1, TileType::Bedrock);
        }
        registry.AddComponent(entity, map);
        return entity;
    }
};