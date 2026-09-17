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
#include "../../ECS/Components/Chara/RangedAttacker.h"
#include "../../ECS/Components/Chara/AreaAttacker.h"
#include "../../ECS/Components/Chara/Summoner.h"
#include "../../ECS/Components/Chara/Charger.h"
#include "TownNpc.h"

struct ZoneBuildResult {
    sf::Vector2f playerSpawn{ 100.f, 100.f };
    bool hasPortal = false;
    sf::Vector2f portalPos{ 0.f, 0.f };
    std::vector<TownNpcSpawn> townNpcs;
};

class ZoneBuilder {
public:
    static ZoneBuildResult Build(Registry& registry, const ZoneDefinition& zone, int endgameMapTier, bool isEndgame,
        const std::vector<WaystoneMod>& mapMods = {}) {
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

            result.townNpcs = PlaceTownNpcs(map, startPos, goalPos);
            for (const auto& npc : result.townNpcs) {
                SpawnNpcMarker(registry, npc.pos, npc.kind);
            }

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

        // Endgame maps must complete requires clearing multiple Rare enemies plus the
        // boss (see GameScene's all-enemies-dead check), so guarantee a handful of Rares
        // instead of leaving it to the normal 4% per-monster roll.
        int guaranteedRares = isEndgame ? (std::min)(3, zone.enemyCount) : 0;

        int trashCount = zone.enemyCount;
        for (int i = 0; i < trashCount && i < static_cast<int>(freeSlots.size()); ++i) {
            SpawnTrash(registry, freeSlots[i], zone, tierScale, endgameMapTier, i < guaranteedRares, mapMods);
        }

        if (zone.isBossZone) {
            SpawnBoss(registry, goalPos, zone, tierScale, endgameMapTier, mapMods);
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

    static void SpawnTrash(Registry& registry, sf::Vector2f pos, const ZoneDefinition& zone, float tierScale,
        int endgameMapTier, bool forceRare, const std::vector<WaystoneMod>& mapMods) {
        auto enemy = EntitySpawner::CreateEnemy(registry, pos);

        auto& stats = enemy.GetComponent<CharacterStatsComponent>();
        stats.name = zone.enemyTypeName;
        stats.maxHP *= zone.enemyHpMult * tierScale;
        stats.atk *= zone.enemyAtkMult * tierScale;
        stats.contactDamageType = zone.enemyContactType;
        ApplyTierResistance(stats, endgameMapTier);
        ApplyMapMods(stats, mapMods);

        auto& circle = enemy.GetComponent<CircleComponent>();
        circle.color = zone.enemyColor;

        static std::random_device rd;
        static std::mt19937 rng(rd());
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);
        float r = forceRare ? 0.0f : roll(rng);

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

        // レア/ユニークとは独立して、一定確率で行動パターンの異なるアーケタイプにする
        // (接触ダメージのみだった既存モンスターに行動パターンの幅を持たせる。
        //  遠距離/範囲攻撃/召喚/突進は互いに排他)
        float archetypeRoll = roll(rng);
        if (archetypeRoll < 0.15f) {
            stats.name = "叩き潰す" + stats.name;
            circle.radius *= 1.1f;

            AreaAttackerComponent area;
            area.triggerRange = 90.0f;
            area.telegraphTime = (std::max)(0.5f, 0.9f - tierScale * 0.05f);
            area.cooldownTime = 3.2f;
            area.radius = 100.0f;
            area.damagePercent = 90.0f;
            area.baseColor = circle.color;
            enemy.AddComponent(area);
        } else if (archetypeRoll < 0.35f) {
            stats.name = "射手の" + stats.name;
            circle.radius *= 0.85f;

            RangedAttackerComponent ranged;
            ranged.preferredRange = 220.0f;
            ranged.attackRange = 340.0f;
            ranged.cooldownTime = (std::max)(0.8f, 2.4f - tierScale * 0.2f);
            ranged.projectileSpeed = 260.0f;
            ranged.damagePercent = 65.0f;
            enemy.AddComponent(ranged);
        } else if (archetypeRoll < 0.45f) {
            stats.name = "召喚術士の" + stats.name;

            SummonerComponent summoner;
            summoner.triggerRange = 260.0f;
            summoner.summonInterval = (std::max)(3.0f, 5.5f - tierScale * 0.3f);
            summoner.maxActiveSummons = 2;
            summoner.statScale = 0.5f;
            enemy.AddComponent(summoner);
        } else if (archetypeRoll < 0.60f) {
            stats.name = "突進する" + stats.name;

            ChargerComponent charger;
            charger.triggerRange = 220.0f;
            charger.telegraphTime = (std::max)(0.25f, 0.5f - tierScale * 0.03f);
            charger.chargeDuration = 0.4f;
            charger.chargeSpeedMultiplier = 3.5f;
            charger.cooldownTime = (std::max)(1.2f, 2.0f - tierScale * 0.1f);
            charger.baseColor = circle.color;
            enemy.AddComponent(charger);
        }

        stats.currentHP = stats.maxHP;
        enemy.AddComponent(TagComponent{ stats.name });
    }

    static void SpawnBoss(Registry& registry, sf::Vector2f pos, const ZoneDefinition& zone, float tierScale, int endgameMapTier,
        const std::vector<WaystoneMod>& mapMods) {
        auto boss = EntitySpawner::CreateEnemy(registry, pos);

        auto& stats = boss.GetComponent<CharacterStatsComponent>();
        stats.name = zone.bossName;
        stats.maxHP *= zone.bossHpMult * tierScale;
        stats.atk *= zone.bossAtkMult * tierScale;
        stats.rarity = MonsterRarity::Unique;
        stats.contactDamageType = zone.enemyContactType;
        ApplyTierResistance(stats, endgameMapTier);
        ApplyMapMods(stats, mapMods);
        stats.currentHP = stats.maxHP;

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

    // Endgame Waystones raise monster resistances by tier (in addition to the existing
    // HP/attack tierScale), matching PoE2's map-tier scaling. A no-op outside endgame
    // since endgameMapTier stays 1 for the whole campaign until the first map is opened.
    static void ApplyTierResistance(CharacterStatsComponent& stats, int endgameMapTier) {
        float bonus = (std::min)(0.75f, 0.03f * static_cast<float>(endgameMapTier - 1));
        stats.fireRes = (std::min)(0.9f, stats.fireRes + bonus);
        stats.iceRes = (std::min)(0.9f, stats.iceRes + bonus);
        stats.lightningRes = (std::min)(0.9f, stats.lightningRes + bonus);
        stats.chaosRes = (std::min)(0.9f, stats.chaosRes + bonus);
    }

    // Monster-affecting mods rolled onto the Waystone that opened this map (see Item.h /
    // ItemFactory::WaystoneModPool). The player-affecting mod (reduced player elemental
    // resistance) is applied separately in GameScene, not here -- this only ever touches
    // monster stats. Item find mods (rarity/quantity) are applied in CollisionSystem at
    // drop time instead, since they don't describe a monster stat at all.
    static void ApplyMapMods(CharacterStatsComponent& stats, const std::vector<WaystoneMod>& mods) {
        for (const auto& mod : mods) {
            switch (mod.stat) {
            case WaystoneModStat::MonsterIncreasedLife:
                stats.maxHP *= (1.0f + mod.value / 100.0f);
                break;
            case WaystoneModStat::MonsterIncreasedDamage:
                stats.atk *= (1.0f + mod.value / 100.0f);
                break;
            case WaystoneModStat::MonsterIncreasedElementalResistance: {
                float bonus = mod.value / 100.0f;
                stats.fireRes = (std::min)(0.9f, stats.fireRes + bonus);
                stats.iceRes = (std::min)(0.9f, stats.iceRes + bonus);
                stats.lightningRes = (std::min)(0.9f, stats.lightningRes + bonus);
                break;
            }
            default:
                break; // player/item-find mods handled elsewhere, see comment above
            }
        }
    }

    static void SpawnPortal(Registry& registry, sf::Vector2f pos) {
        auto portal = registry.CreateEntityObject();
        portal.AddComponent(TransformComponent{ pos, {1.f, 1.f}, 0.f });
        portal.AddComponent(CircleComponent{ 24.0f, sf::Color(255, 210, 60), true });
        portal.AddComponent(TagComponent{ "Portal" });
        portal.AddComponent(InteractableComponent{ InteractType::Portal, true, false, false, -1 });
    }

    static void SpawnNpcMarker(Registry& registry, sf::Vector2f pos, TownNpcKind kind) {
        auto npc = registry.CreateEntityObject();
        npc.AddComponent(TransformComponent{ pos, {1.f, 1.f}, 0.f });
        switch (kind) {
        case TownNpcKind::ItemVendor:
            npc.AddComponent(CircleComponent{ 20.0f, sf::Color(80, 200, 255), true });
            npc.AddComponent(TagComponent{ "Vendor" });
            break;
        case TownNpcKind::WaystoneVendor:
            npc.AddComponent(CircleComponent{ 20.0f, sf::Color(200, 150, 255), true });
            npc.AddComponent(TagComponent{ "Waystone Vendor" });
            break;
        case TownNpcKind::Stash:
            npc.AddComponent(CircleComponent{ 20.0f, sf::Color(150, 110, 60), true });
            npc.AddComponent(TagComponent{ "Stash" });
            break;
        }
    }

    // Picks well-separated walkable spots for the town's NPCs (item Vendor, Waystone
    // Vendor, Stash), avoiding the player's spawn point and the portal so nothing spawns
    // on top of either. Falls back to the map center for any NPC that couldn't find a
    // spread-out spot (e.g. a very small/cramped town layout).
    static std::vector<TownNpcSpawn> PlaceTownNpcs(const MapComponent& map, sf::Vector2f avoidA, sf::Vector2f avoidB) {
        std::vector<sf::Vector2f> candidates = MapGenerator::GetWalkablePositions(map);
        float minAvoidDist = map.tileSize * 2.0f;
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](const sf::Vector2f& p) {
            return Distance(p, avoidA) < minAvoidDist || Distance(p, avoidB) < minAvoidDist;
            }), candidates.end());

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(candidates.begin(), candidates.end(), g);

        sf::Vector2f fallback(static_cast<float>(map.width / 2) * map.tileSize, static_cast<float>(map.height / 2) * map.tileSize);
        std::vector<TownNpcKind> kinds = { TownNpcKind::ItemVendor, TownNpcKind::WaystoneVendor, TownNpcKind::Stash };
        std::vector<TownNpcSpawn> result;
        float minSeparation = map.tileSize * 2.5f;

        for (TownNpcKind kind : kinds) {
            sf::Vector2f chosen = fallback;
            bool found = false;
            for (const auto& candidate : candidates) {
                bool farEnough = true;
                for (const auto& placed : result) {
                    if (Distance(candidate, placed.pos) < minSeparation) { farEnough = false; break; }
                }
                if (farEnough) { chosen = candidate; found = true; break; }
            }
            if (!found) chosen = fallback;
            result.push_back({ kind, chosen });
        }
        return result;
    }
};
