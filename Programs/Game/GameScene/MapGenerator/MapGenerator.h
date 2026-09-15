#pragma once
#include "ECS.h"
#include "../../ECS/Components/World/Map.h"
#include <random>
#include <algorithm>
#include <vector>
#include <spdlog/spdlog.h>

class MapGenerator {
public:
    static EntityObject CreateProceduralWorld(Registry& registry, int w = 100, int h = 100) {
        auto entity = registry.CreateEntityObject();

        entity.AddComponent(MapComponent{});
        auto& map = entity.GetComponent<MapComponent>();

        GenerateRoomsAndCorridors(map, w, h);

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

    // Carves a handful of rectangular rooms (non-overlapping, with a margin) connected
    // in sequence by L-shaped corridors, which guarantees every room is reachable from
    // the first. Falls back to the old single-corridor random walk if the map is too
    // small to fit any room (so a playable start/goal path always exists).
    static void GenerateRoomsAndCorridors(MapComponent& map, int width, int height) {
        map.Resize(width, height); // defaults to all Stone (walls)

        std::random_device rd;
        std::mt19937 mt(rd());

        struct Room {
            int x, y, w, h;
            sf::Vector2i Center() const { return { x + w / 2, y + h / 2 }; }
        };
        std::vector<Room> rooms;

        std::uniform_int_distribution<int> roomSizeDist(5, 9);
        int targetRoomCount = std::clamp((width * height) / 400, 4, 14);
        int attempts = targetRoomCount * 10;

        for (int a = 0; a < attempts && static_cast<int>(rooms.size()) < targetRoomCount; ++a) {
            int rw = roomSizeDist(mt);
            int rh = roomSizeDist(mt);
            int maxX = width - rw - 2;
            int maxY = height - rh - 2;
            if (maxX <= 2 || maxY <= 2) break; // map too small for this room size

            std::uniform_int_distribution<int> xDist(2, maxX);
            std::uniform_int_distribution<int> yDist(2, maxY);
            int rx = xDist(mt);
            int ry = yDist(mt);

            bool overlaps = false;
            for (const auto& other : rooms) {
                if (rx < other.x + other.w + 2 && rx + rw + 2 > other.x &&
                    ry < other.y + other.h + 2 && ry + rh + 2 > other.y) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps) continue;

            for (int y = ry; y < ry + rh; ++y) {
                for (int x = rx; x < rx + rw; ++x) {
                    map.SetTile(x, y, TileType::Dirt);
                }
            }
            rooms.push_back({ rx, ry, rw, rh });
        }

        if (rooms.empty()) {
            // Map too small to fit even one room; keep the old generator as a fallback
            // so there is always some walkable area with a start/goal.
            GenerateRandomWalk(map, width, height, (width * height) / 2);
            return;
        }

        for (size_t i = 1; i < rooms.size(); ++i) {
            CarveCorridor(map, rooms[i - 1].Center(), rooms[i].Center(), mt);
        }

        sf::Vector2i startCenter = rooms.front().Center();
        sf::Vector2i goalCenter = rooms.back().Center();
        map.SetTile(startCenter.x, startCenter.y, TileType::Wood);
        map.SetTile(goalCenter.x, goalCenter.y, TileType::Grass);

        spdlog::info("Map Generated: Rooms&Corridors ({}x{}, {} rooms)", width, height, static_cast<int>(rooms.size()));
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
    // Carves an L-shaped 1-tile-wide corridor between two points (random bend order),
    // guaranteeing the two rooms it connects become mutually reachable.
    static void CarveCorridor(MapComponent& map, sf::Vector2i from, sf::Vector2i to, std::mt19937& mt) {
        std::uniform_int_distribution<int> coin(0, 1);
        bool horizontalFirst = coin(mt) == 0;

        int x = from.x, y = from.y;
        auto carve = [&](int cx, int cy) { map.SetTile(cx, cy, TileType::Dirt); };

        if (horizontalFirst) {
            while (x != to.x) { carve(x, y); x += (to.x > x) ? 1 : -1; }
            while (y != to.y) { carve(x, y); y += (to.y > y) ? 1 : -1; }
        } else {
            while (y != to.y) { carve(x, y); y += (to.y > y) ? 1 : -1; }
            while (x != to.x) { carve(x, y); x += (to.x > x) ? 1 : -1; }
        }
        carve(to.x, to.y);
    }

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
