#pragma once
#include <ECS.h>
#include <System/Campaign/CampaignManager.h>
#include <random>
#include <algorithm>
#include <cmath>
#include "../MapGenerator/MapGenerator.h"
#include "../Entity/EntitySpawner.h"
#include "../../ECS/Components/Tags/Boss/Boss.h"
#include "../../ECS/Components/Tags/Boss/BossPhase.h"
#include "../../ECS/Components/Interaction/Interaction.h"

struct ZoneBuildResult {
    sf::Vector2f playerSpawn{ 100.f, 100.f };
    bool hasPortal = false;
    sf::Vector2f portalPos{ 0.f, 0.f };
};

class ZoneBuilder {
public:
    static ZoneBuildResult Build(Registry& registry, const ZoneDefinition& zone, int endgameMapTier) {
        ZoneBuildResult result;

        auto worldObj = (zone.kind == ZoneKind::Town)
            ? MapGenerator::CreateTownWorld(registry, zone.mapWidth, zone.mapHeight)
            : MapGenerator::CreateProceduralWorld(registry, zone.mapWidth, zone.mapHeight);
        if (!worldObj) return result;

        auto& map = worldObj.GetComponent<MapComponent>();

        sf::Vector2f startPos = FindTile(map, TileType::Wood);
        sf::Vector2f goalPos = FindTile(map, TileType::Grass);
        result.playerSpawn = startPos;

        if (zone.kind == ZoneKind::Town) {
            SpawnPortal(registry, goalPos);
            result.hasPortal = true;
            result.portalPos = goalPos;
            return result;
        }

        std::vector<sf::Vector2f> freeSlots = MapGenerator::GetWalkablePositions(map);
        freeSlots.erase(std::remove_if(freeSlots.begin(), freeSlots.end(), [&](const sf::Vector2f& p) {
            return Distance(p, startPos) < map.tileSize * 3.0f;
            }), freeSlots.end());

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(freeSlots.begin(), freeSlots.end(), g);

        float tierScale = 1.0f + 0.15f * static_cast<float>(endgameMapTier - 1);

        int trashCount = zone.enemyCount;
        for (int i = 0; i < trashCount && i < static_cast<int>(freeSlots.size()); ++i) {
            SpawnTrash(registry, freeSlots[i], zone, tierScale);
        }

        if (zone.isBossZone) {
            SpawnBoss(registry, goalPos, zone, tierScale);
        }

        return result;
    }

private:
    static float Distance(const sf::Vector2f& a, const sf::Vector2f& b) {
        float dx = a.x - b.x, dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    static sf::Vector2f FindTile(const MapComponent& map, TileType type) {
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                if (map.GetTile(x, y) == type) {
                    return sf::Vector2f(x * map.tileSize, y * map.tileSize);
                }
            }
        }
        return sf::Vector2f(map.tileSize, map.tileSize);
    }

    static void SpawnTrash(Registry& registry, sf::Vector2f pos, const ZoneDefinition& zone, float tierScale) {
        auto enemy = EntitySpawner::CreateEnemy(registry, pos);

        auto& stats = enemy.GetComponent<CharacterStatsComponent>();
        stats.name = zone.enemyTypeName;
        stats.maxHP *= zone.enemyHpMult * tierScale;
        stats.atk *= zone.enemyAtkMult * tierScale;
        stats.contactDamageType = zone.enemyContactType;

        auto& circle = enemy.GetComponent<CircleComponent>();
        circle.color = zone.enemyColor;

        static std::random_device rd;
        static std::mt19937 rng(rd());
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);
        float r = roll(rng);

        if (r < 0.04f) {
            stats.rarity = MonsterRarity::Rare;
            stats.maxHP *= 2.5f;
            stats.atk *= 1.6f;
            stats.name = "変異した" + zone.enemyTypeName;
            circle.radius *= 1.4f;
            circle.color = sf::Color(255, 200, 40);

            std::uniform_int_distribution<int> elemRoll(0, 3);
            switch (elemRoll(rng)) {
            case 0: stats.contactDamageType = DamageElement::Fire; break;
            case 1: stats.contactDamageType = DamageElement::Cold; break;
            case 2: stats.contactDamageType = DamageElement::Lightning; break;
            default: stats.contactDamageType = DamageElement::Physical; break;
            }
        } else if (r < 0.22f) {
            stats.rarity = MonsterRarity::Magic;
            stats.maxHP *= 1.4f;
            stats.atk *= 1.2f;
            stats.name = zone.enemyTypeName + "・強化個体";
            circle.color = sf::Color(
                (std::min)(255, zone.enemyColor.r + 60),
                (std::min)(255, zone.enemyColor.g + 60),
                255);
        }

        stats.currentHP = stats.maxHP;
        enemy.AddComponent(TagComponent{ stats.name });
    }

    static void SpawnBoss(Registry& registry, sf::Vector2f pos, const ZoneDefinition& zone, float tierScale) {
        auto boss = EntitySpawner::CreateEnemy(registry, pos);

        auto& stats = boss.GetComponent<CharacterStatsComponent>();
        stats.name = zone.bossName;
        stats.maxHP *= zone.bossHpMult * tierScale;
        stats.currentHP = stats.maxHP;
        stats.atk *= zone.bossAtkMult * tierScale;
        stats.rarity = MonsterRarity::Unique;
        stats.contactDamageType = zone.enemyContactType;

        auto& circle = boss.GetComponent<CircleComponent>();
        circle.color = zone.bossColor;
        circle.radius = 36.0f;

        auto& collider = boss.GetComponent<BoxColliderComponent>();
        collider.width = 48.0f;
        collider.height = 48.0f;

        boss.AddComponent(TagComponent{ zone.bossName });
        boss.AddComponent(BossTag{});
        boss.AddComponent(BossPhaseComponent{});
    }

    static void SpawnPortal(Registry& registry, sf::Vector2f pos) {
        auto portal = registry.CreateEntityObject();
        portal.AddComponent(TransformComponent{ pos, {1.f, 1.f}, 0.f });
        portal.AddComponent(CircleComponent{ 24.0f, sf::Color(255, 210, 60), true });
        portal.AddComponent(TagComponent{ "Portal" });
        portal.AddComponent(InteractableComponent{ InteractType::Portal, true, false, false, -1 });
    }
};
