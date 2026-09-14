#pragma once
#include "ECS.h"
#include "../../ECS/Components/World/Map.h"
#include <random>
#include <algorithm>
#include <spdlog/spdlog.h>

class MapGenerator {
public:
    static EntityObject CreateProceduralWorld(Registry& registry, int w = 100, int h = 100) {
        auto entity = registry.CreateEntityObject();

        //entity.AddComponent<MapComponent>();
        entity.AddComponent(MapComponent{});
        auto& map = entity.GetComponent<MapComponent>();

        int steps = (w * h) / 2;

        GenerateRandomWalk(map, w, h, steps);

        return entity;
    }

    static EntityObject CreateTownWorld(Registry& registry, int w = 30, int h = 20) {
        auto entity = registry.CreateEntityObject();
        entity.AddComponent(MapComponent{});
        auto& map = entity.GetComponent<MapComponent>();
        map.Resize(w, h);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                bool border = (x == 0 || y == 0 || x == w - 1 || y == h - 1);
                map.SetTile(x, y, border ? TileType::Stone : TileType::Dirt);
            }
        }

        int midY = h / 2;
        map.SetTile(2, midY, TileType::Wood);
        map.SetTile(w - 3, midY, TileType::Grass);

        spdlog::info("Map Generated: Town Room ({}x{})", w, h);
        return entity;
    }

    static void GenerateRandomWalk(MapComponent& map, int width, int height, int steps) {
        // ������
        map.Resize(width, height);

        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> dirDist(0, 3);

        int x = width / 2;
        int y = height / 2;
        map.SetTile(x, y, TileType::Dirt);

        for (int i = 0; i < steps; ++i) {
            int dir = dirDist(mt);

            if (dir == 0 && x > 1) x--;             // Left
            else if (dir == 1 && x < width - 2) x++; // Right
            else if (dir == 2 && y > 1) y--;        // Up
            else if (dir == 3 && y < height - 2) y++; // Down

            map.SetTile(x, y, TileType::Dirt); // ���ɂ���
        }

        SetStartGoal(map);

        spdlog::info("Map Generated: RandomWalk ({}x{})", width, height);
    }
    // �G�l�~�[�����p�Ɉʒu���肷���
    static std::vector<sf::Vector2f> GetWalkablePositions(const MapComponent& map) {
        std::vector<sf::Vector2f> positions;
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                TileType tile = map.GetTile(x, y);

                if (tile == TileType::Dirt || tile == TileType::Wood || tile == TileType::Grass) {
                    float posX = x * map.tileSize;
                    float posY = y * map.tileSize;
                    positions.push_back({ posX, posY });
                }
            }
        }
        return positions;
    }
private:
    static void SetStartGoal(MapComponent& map) {
        // �X�^�[�g�n�_�T��
        bool startSet = false;
        for (int y = 0; y < map.height && !startSet; ++y) {
            for (int x = 0; x < map.width; ++x) {
                if (map.GetTile(x, y) == TileType::Dirt) {
                    map.SetTile(x, y, TileType::Wood); // Start
                    startSet = true;
                    break;
                }
            }
        }

        // �S�[���n�_�T��
        bool goalSet = false;
        for (int y = map.height - 1; y >= 0 && !goalSet; --y) {
            for (int x = map.width - 1; x >= 0; --x) {
                if (map.GetTile(x, y) == TileType::Dirt) {
                    map.SetTile(x, y, TileType::Grass); // Goal
                    goalSet = true;
                    break;
                }
            }
        }
    }
};