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
#include "../../ECS/Components/Chara/MapSlot.h"
#include "../../ECS/Components/World/HazardGround.h"
#include "TownNpc.h"

struct ZoneBuildResult {
    sf::Vector2f playerSpawn{ 100.f, 100.f };
    bool hasPortal = false;
    sf::Vector2f portalPos{ 0.f, 0.f }; // マップデバイス自体の中心(近接判定用)
    // クリック可能な個々のポータルの中心座標。進行中のマップアタempトが無ければ従来通り
    // 1個(Atlasを開く)、あればGetMapDeathsRemaining()個を円状に並べる(「デバイスを
    // 中心として６つのポータルを円状に出す」という指示)。
    std::vector<sf::Vector2f> portalMarkers;
    std::vector<TownNpcSpawn> townNpcs;
};

class ZoneBuilder {
public:
    // mapSeed: 0なら毎回ランダムな迷路・敵構成(初回生成時の従来動作)、非0なら同じ値の
    // うちは毎回同じ迷路・同じ敵構成になる(CampaignManager::GetMapSeed、同一マップ
    // アタempト中は再入場のたびに変わらないようにするため)。トラッシュ/Rare/ボスは
    // それぞれスポーン枠番号(MapSlotComponent)を持ち、CampaignManager::
    // IsEnemySlotDeadで個体ごとに死亡済みかを見て、死亡済みの枠だけスポーンをスキップ
    // する("雑魚敵も復活しないようにして…同じ個体で"という指示対応、生存中の個体は
    // 同じシードのおかげで毎回同じ座標・種族・レアリティ・アーケタイプで出現する)。
    static ZoneBuildResult Build(Registry& registry, const ZoneDefinition& zone, int endgameMapTier, bool isEndgame,
        const std::vector<WaystoneMod>& mapMods = {}, bool hasActiveMapAttempt = false, int mapDeathsRemaining = 0,
        unsigned int mapSeed = 0) {
        ZoneBuildResult result;

        auto worldObj = (zone.kind == ZoneKind::Town)
            ? MapGenerator::CreateTownWorld(registry, zone.mapWidth, zone.mapHeight, zone.tileSize)
            : MapGenerator::CreateProceduralWorld(registry, zone.mapWidth, zone.mapHeight, zone.tileSize, mapSeed);
        if (!worldObj) return result;

        auto& map = worldObj.GetComponent<MapComponent>();

        // 同じマップアタempト中に帰還用ポータルで離脱していれば、その時点の探索済み
        // タイルを復元する(タウンは常にRevealAllするため対象外、サイズが違えば
        // 別マップなので復元しない)。("ミニマップがリセットされてる"というバグ
        // 報告対応)。
        if (zone.kind != ZoneKind::Town) {
            const auto& savedVisited = CampaignManager::Instance().GetMapVisitedTiles();
            if (savedVisited.size() == map.visited.size()) {
                map.visited = savedVisited;
            }
        }

        sf::Vector2f startPos = FindTile(map, TileType::Wood);
        sf::Vector2f goalPos = FindTile(map, TileType::Grass);
        result.playerSpawn = startPos;

        if (zone.kind == ZoneKind::Town) {
            // タウンのミニマップ/Tab全体マップは探索済みかどうかに関わらず常に全体を
            // 表示する("町でのミニマップは全部常に見えている状態"という指示対応)。
            map.RevealAll();
            result.hasPortal = true;
            // goalPos is the portal entity's top-left TransformComponent anchor (see
            // RenderSystem, which always draws a CircleComponent centered at
            // position + radius) -- offset by the same radius here so the click-range
            // ring/proximity check GameScene does against portalPos actually lines up
            // with where the portal is drawn, instead of sitting at its top-left corner.
            result.portalPos = goalPos + sf::Vector2f(kPortalMarkerRadius, kPortalMarkerRadius);

            // 進行中のマップアタempトがあれば、残りポータル数ぶんの入場口をデバイス周りに
            // 円状に並べる(どれをクリックしても同じマップへ無料で再入場)。無ければ従来
            // 通り単体のポータルを出し、これはAtlasを開く(Waystoneでティアを選ぶ)ための
            // もの。「マップデバイスのポータルが消えるタイミングは次のマップをウェイス
            // トーンで開くときに更新される」の通り、ここでの再計算はゾーン再構築(=次の
            // マップ開始/再入場)のたびにしか起きない。
            if (hasActiveMapAttempt && mapDeathsRemaining > 0) {
                result.portalMarkers = SpawnReturnPortalRing(registry, result.portalPos, mapDeathsRemaining);
            } else {
                SpawnPortal(registry, goalPos);
                result.portalMarkers = { result.portalPos };
            }

            result.townNpcs = PlaceTownNpcs(registry, map, startPos, goalPos);

            return result;
        }

        // Bosses fight in a dedicated carved-out room (see MapGenerator::CarveBossRoom)
        // instead of wherever the random walk happened to end -- goalPos becomes that
        // room's center, which is also this zone's only other use of goalPos (SpawnBoss
        // below).
        if (zone.isBossZone) {
            goalPos = MapGenerator::CarveBossRoom(map, startPos, kBossRoomHalfSize);
        }

        std::vector<sf::Vector2f> freeSlots = MapGenerator::GetWalkablePositions(map);
        float bossRoomExclusion = (static_cast<float>(kBossRoomHalfSize) + 1.5f) * map.tileSize;
        freeSlots.erase(std::remove_if(freeSlots.begin(), freeSlots.end(), [&](const sf::Vector2f& p) {
            bool nearStart = Distance(p, startPos) < map.tileSize * 3.0f;
            bool inBossRoom = zone.isBossZone && Distance(p, goalPos) < bossRoomExclusion;
            return nearStart || inBossRoom;
            }), freeSlots.end());

        // マップシードと同じ値を使い回すことで、freeSlotsのシャッフル順=各スポーン枠が
        // 対応する座標を再構築のたびに安定させる(0=非決定的の時だけ従来通りランダム)。
        std::mt19937 g(mapSeed != 0 ? mapSeed : std::random_device{}());
        std::shuffle(freeSlots.begin(), freeSlots.end(), g);

        float tierScale = 1.0f + 0.15f * static_cast<float>(endgameMapTier - 1);

        auto& campaign = CampaignManager::Instance();

        // Endgame maps must complete requires clearing multiple Rare enemies plus the
        // boss (see GameScene's all-enemies-dead check), so guarantee a handful of Rares
        // instead of leaving it to the normal 4% per-monster roll.
        int trashCount = zone.enemyCount;
        int guaranteedRares = isEndgame ? (std::min)(3, trashCount) : 0;

        for (int i = 0; i < trashCount && i < static_cast<int>(freeSlots.size()); ++i) {
            if (campaign.IsEnemySlotDead(i)) continue; // この個体は既にこのアタempト中に倒された
            // スロット番号とマップシードから決定論的なシードを算出する(同じ枠は
            // 再構築のたびに同じ種族/レアリティ/アーケタイプ/元素になる)。
            unsigned int slotSeed = mapSeed != 0 ? (mapSeed ^ (static_cast<unsigned int>(i) * 2654435761u)) : 0;
            SpawnTrash(registry, freeSlots[i], zone, tierScale, endgameMapTier, i < guaranteedRares, mapMods, i, slotSeed);
        }

        if (zone.isBossZone && !campaign.IsEnemySlotDead(kBossSlotIndex)) {
            SpawnBoss(registry, goalPos, zone, tierScale, endgameMapTier, mapMods);
        }

        unsigned int hazardSeed = mapSeed != 0 ? (mapSeed ^ 0x9E3779B9u) : 0;
        PlaceHazardGrounds(map, freeSlots, startPos, hazardSeed);

        return result;
    }

private:
    // RenderSystem draws every CircleComponent centered at transform.position + radius
    // (position is the entity's top-left anchor, not its visual center -- see
    // RenderSystem::Render). SpawnPortal/SpawnNpcMarker below spawn their entities with
    // that same top-left convention, so ZoneBuildResult::portalPos/TownNpcSpawn::pos must
    // add this same radius offset to actually line up with where the circle is drawn --
    // otherwise the click-range ring GameScene draws (and the click/proximity checks
    // themselves) end up centered on the shape's top-left corner instead of its visible
    // center, which is what caused the reported "talk range doesn't line up with the NPC"
    // misalignment.
    static constexpr float kPortalMarkerRadius = 24.0f;
    static constexpr float kTownNpcMarkerRadius = 20.0f;
    // Boss room half-size in tiles (room is a (2*N+1) square) -- see MapGenerator::CarveBossRoom.
    static constexpr int kBossRoomHalfSize = 6;
    // MapSlotComponentの予約番号(トラッシュ/Rareは0..trashCount-1を使うため、ボスは
    // 衝突しない-1固定)。
    static constexpr int kBossSlotIndex = -1;

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

    // slotIndex: このマップアタempト内でのスポーン枠番号(MapSlotComponent、死亡記録・
    // 再湧き防止に使う)。seed: 0なら非決定的(std::random_device)、非0ならこの値だけで
    // 種族/レアリティ/アーケタイプ/元素が決まる(ZoneBuilder::Build参照、同じ枠は毎回
    // 同じ個体になる)。
    static void SpawnTrash(Registry& registry, sf::Vector2f pos, const ZoneDefinition& zone, float tierScale,
        int endgameMapTier, bool forceRare, const std::vector<WaystoneMod>& mapMods, int slotIndex, unsigned int seed = 0) {
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

        std::mt19937 rng(seed != 0 ? seed : std::random_device{}());
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

        // 元素攻撃モンスター("敵の攻撃に元素攻撃がいない"というフィードバック対応)。
        // Rareは既に上のブロックでランダムな元素を持ちうるため、まだPhysicalのままの
        // 個体にだけ追加で元素を付与する。遠距離(Ranged)は投射物のdamageTypeへそのまま
        // 乗る(EnemyRangedAttackSystem::proj.damageType = stats.contactDamageType参照)
        // ため主眼として高確率、近接アーキタイプにも低確率で付与する(「近距離もいて
        // いいよ」との指示通り、接触/範囲攻撃も既にstats.contactDamageTypeをそのまま
        // 使っているため追加実装は不要、CollisionSystem/EnemyAreaAttackSystem参照)。
        if (stats.contactDamageType == DamageElement::Physical) {
            bool isRanged = archetypeRoll >= 0.15f && archetypeRoll < 0.35f;
            float elemChance = isRanged ? 0.6f : 0.25f;
            if (roll(rng) < elemChance) {
                std::uniform_int_distribution<int> elemPick(0, 2);
                std::string prefix;
                switch (elemPick(rng)) {
                case 0: stats.contactDamageType = DamageElement::Fire; circle.color = sf::Color(255, 120, 40); prefix = "業火の"; break;
                case 1: stats.contactDamageType = DamageElement::Cold; circle.color = sf::Color(120, 210, 255); prefix = "凍える"; break;
                default: stats.contactDamageType = DamageElement::Lightning; circle.color = sf::Color(255, 235, 90); prefix = "帯電した"; break;
                }
                stats.name = prefix + stats.name;
            }
        }

        // 生存中に離脱した個体はそのHPから再開する(無ければ満タン、「同じ個体で」
        // という指示対応)。
        float savedHp;
        stats.currentHP = CampaignManager::Instance().GetEnemySlotHp(slotIndex, savedHp)
            ? std::clamp(savedHp, 0.0f, stats.maxHP) : stats.maxHP;
        enemy.AddComponent(TagComponent{ stats.name });
        enemy.AddComponent(MapSlotComponent{ slotIndex });
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
        // 生存中に離脱していたらそのHPから再開する(無ければ満タン、「同じ個体で」
        // という指示対応)。
        float savedHp;
        stats.currentHP = CampaignManager::Instance().GetEnemySlotHp(kBossSlotIndex, savedHp)
            ? std::clamp(savedHp, 0.0f, stats.maxHP) : stats.maxHP;
        boss.AddComponent(MapSlotComponent{ kBossSlotIndex });

        auto& circle = boss.GetComponent<CircleComponent>();
        circle.color = zone.bossColor;
        circle.radius = 36.0f; // 直径72px

        // CreateEnemy's default collider offset (10px/side) was tuned for its 40px visual
        // -- left un-recalculated here, the boss's much bigger 72px circle would visually
        // "めり込む"(sink) into walls by up to 22px on one side. Recompute for the new size
        // with the same 3px-gap policy used everywhere else (see CreatePlayer/CreateEnemy).
        auto& collider = boss.GetComponent<BoxColliderComponent>();
        collider.width = 66.0f;
        collider.height = 66.0f;
        collider.offsetX = 3.0f;
        collider.offsetY = 3.0f;

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

    // PoE2の燃焼床/雷の床/氷の床/混沌ダメの床(HazardGroundKind)を、freeSlots(既に
    // 開始地点付近/ボス部屋を除外済み、SpawnTrashが消費した後の同じ候補リスト)から
    // ランダムに選んだタイルを起点に、そこから数タイル分のランダムウォークで
    // MapComponent::hazardTilesへ焼き込む(円形エンティティではなく、Dirt/Stone等の他の
    // タイル種別と同じ「マスに紐づく地形情報」として持たせる、ユーザー指示の「床はタイル
    // で」に対応)。Dirt(床)以外のタイル(壁等)には書き込まない。効果の強さ自体は
    // タイルにデータを持たせず、HazardGroundSystemがそのときのTierから都度算出する。
    // 開始地点からさらにtileSize*4分の余裕を取り、湧いた直後に即被弾する事故を防ぐ。
    // タウンには配置しない(呼び出し元のBuild()がTownでは早期returnするため、ここに
    // 到達する時点で必ずCombatゾーン)。
    // seed: 0なら非決定的、非0なら同じ値のうちは毎回同じ配置になる(ZoneBuilder::Build
    // のmapSeedから派生、マップ全体の再現性を敵構成と揃えるための任意対応)。
    static void PlaceHazardGrounds(MapComponent& map, const std::vector<sf::Vector2f>& freeSlots, sf::Vector2f startPos, unsigned int seed = 0) {
        if (freeSlots.empty()) return;

        int patchCount = std::clamp((map.width * map.height) / 1400, 3, 12);

        std::mt19937 rng(seed != 0 ? seed : std::random_device{}());
        std::uniform_int_distribution<size_t> slotDist(0, freeSlots.size() - 1);
        std::uniform_int_distribution<int> kindDist(1, 4); // HazardGroundKind::Burning..Caustic
        std::uniform_int_distribution<int> patchSizeDist(5, 14);
        std::uniform_int_distribution<int> dirDist(0, 3);

        for (int i = 0; i < patchCount; ++i) {
            sf::Vector2f seedWorld = freeSlots[slotDist(rng)];
            if (Distance(seedWorld, startPos) < map.tileSize * 4.0f) continue;

            sf::Vector2i tile = map.WorldToTile(seedWorld.x + map.tileSize / 2.0f, seedWorld.y + map.tileSize / 2.0f);
            HazardGroundKind kind = static_cast<HazardGroundKind>(kindDist(rng));
            int patchSize = patchSizeDist(rng);

            for (int step = 0; step < patchSize; ++step) {
                if (map.GetTile(tile.x, tile.y) == TileType::Dirt) {
                    map.SetHazard(tile.x, tile.y, kind);
                }
                switch (dirDist(rng)) {
                case 0: tile.x++; break;
                case 1: tile.x--; break;
                case 2: tile.y++; break;
                default: tile.y--; break;
                }
            }
        }
    }

    static void SpawnPortal(Registry& registry, sf::Vector2f pos) {
        auto portal = registry.CreateEntityObject();
        portal.AddComponent(TransformComponent{ pos, {1.f, 1.f}, 0.f });
        portal.AddComponent(CircleComponent{ kPortalMarkerRadius, sf::Color(255, 210, 60), true });
        portal.AddComponent(TagComponent{ "Portal" });
        portal.AddComponent(InteractableComponent{ InteractType::Portal, true, false, false, -1 });
    }

    // 進行中のマップアタempトの残りポータル数ぶん、デバイス中心(center、既に描画中心へ
    // オフセット済み)を囲む円状に小さめのポータルを配置する(「デバイスを中心として
    // ６つのポータルを円状に出す」)。どれをクリックしても同じ再入場処理になるため
    // (GameScene側は位置のリストだけを見て最近傍をクリック判定する)、個々のポータルに
    // 役割の違いは持たせない。返り値は各ポータルの描画中心(GameSceneのクリック/リング
    // 判定にそのまま使える)。
    static std::vector<sf::Vector2f> SpawnReturnPortalRing(Registry& registry, sf::Vector2f center, int count) {
        std::vector<sf::Vector2f> markers;
        constexpr float kRingRadius = 70.0f;
        constexpr float kReturnPortalRadius = 16.0f;
        for (int i = 0; i < count; ++i) {
            float angleDeg = -90.0f + (360.0f / static_cast<float>(count)) * static_cast<float>(i);
            float angleRad = angleDeg * 3.14159265f / 180.0f;
            sf::Vector2f markerCenter = center + sf::Vector2f(std::cos(angleRad), std::sin(angleRad)) * kRingRadius;

            auto portal = registry.CreateEntityObject();
            portal.AddComponent(TransformComponent{ markerCenter - sf::Vector2f(kReturnPortalRadius, kReturnPortalRadius), {1.f, 1.f}, 0.f });
            portal.AddComponent(CircleComponent{ kReturnPortalRadius, sf::Color(255, 210, 60), true });
            portal.AddComponent(TagComponent{ "Portal" });
            portal.AddComponent(InteractableComponent{ InteractType::Portal, true, false, false, -1 });
            markers.push_back(markerCenter);
        }
        return markers;
    }

    static void SpawnNpcMarker(Registry& registry, sf::Vector2f pos, TownNpcKind kind) {
        auto npc = registry.CreateEntityObject();
        npc.AddComponent(TransformComponent{ pos, {1.f, 1.f}, 0.f });
        switch (kind) {
        case TownNpcKind::ItemVendor:
            npc.AddComponent(CircleComponent{ kTownNpcMarkerRadius, sf::Color(80, 200, 255), true });
            npc.AddComponent(TagComponent{ "Vendor" });
            break;
        case TownNpcKind::WaystoneVendor:
            npc.AddComponent(CircleComponent{ kTownNpcMarkerRadius, sf::Color(200, 150, 255), true });
            npc.AddComponent(TagComponent{ "Waystone Vendor" });
            break;
        case TownNpcKind::Stash:
            npc.AddComponent(CircleComponent{ kTownNpcMarkerRadius, sf::Color(150, 110, 60), true });
            npc.AddComponent(TagComponent{ "Stash" });
            break;
        }
    }

    // Picks well-separated walkable spots for the town's NPCs (item Vendor, Waystone
    // Vendor, Stash), avoiding the player's spawn point and the portal so nothing spawns
    // on top of either. Falls back to the map center for any NPC that couldn't find a
    // spread-out spot (e.g. a very small/cramped town layout). Spawns each marker here
    // (rather than in a separate loop back in Build()) so it has both the raw top-left
    // tile position (for the entity's TransformComponent) and can return the
    // radius-adjusted center in the same place, instead of the two getting out of sync.
    static std::vector<TownNpcSpawn> PlaceTownNpcs(Registry& registry, const MapComponent& map, sf::Vector2f avoidA, sf::Vector2f avoidB) {
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
        std::vector<sf::Vector2f> chosenTopLefts; // separation check only, kept in the same (top-left) space as `candidates`
        std::vector<TownNpcSpawn> result;
        float minSeparation = map.tileSize * 2.5f;
        sf::Vector2f centerOffset(kTownNpcMarkerRadius, kTownNpcMarkerRadius);

        for (TownNpcKind kind : kinds) {
            sf::Vector2f chosen = fallback;
            bool found = false;
            for (const auto& candidate : candidates) {
                bool farEnough = true;
                for (const auto& placedTopLeft : chosenTopLefts) {
                    if (Distance(candidate, placedTopLeft) < minSeparation) { farEnough = false; break; }
                }
                if (farEnough) { chosen = candidate; found = true; break; }
            }
            if (!found) chosen = fallback;
            chosenTopLefts.push_back(chosen);
            SpawnNpcMarker(registry, chosen, kind);
            result.push_back({ kind, chosen + centerOffset });
        }
        return result;
    }
};
