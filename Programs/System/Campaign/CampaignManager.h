#pragma once
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <unordered_map>
#include <algorithm>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "Components/Item/Equipment.h"
#include "Components/Item/Inventory.h"
#include "Components/Item/Stash.h"
#include "../Singleton/Singleton.h"

enum class ZoneKind { Town, Combat };

struct ZoneDefinition {
    std::string id;
    std::string displayName;
    ZoneKind kind = ZoneKind::Combat;
    bool isBossZone = false;

    int mapWidth = 60;
    int mapHeight = 60;
    float tileSize = 64.0f; // world px per tile (see MapComponent) -- per-zone so a
    // single zone (e.g. the endgame map) can use finer/smaller tiles without affecting
    // the town's already-tuned scale.

    int enemyCount = 20;
    float enemyHpMult = 1.0f;
    float enemyAtkMult = 1.0f;
    std::string enemyTypeName = "Enemy";
    sf::Color enemyColor = sf::Color::Red;
    DamageElement enemyContactType = DamageElement::Physical;

    std::string bossName = "Boss";
    float bossHpMult = 8.0f;
    float bossAtkMult = 3.0f;
    sf::Color bossColor = sf::Color(160, 0, 200);
};

struct ActDefinition {
    std::string id;
    std::string displayName;
    bool isEndgame = false;
    std::vector<ZoneDefinition> zones;
};

class CampaignManager : public Singleton<CampaignManager> {
    friend class Singleton<CampaignManager>;
    CampaignManager();

    std::vector<ActDefinition> m_acts;
    int m_actIndex = 0;
    int m_zoneIndex = 0;
    int m_endgameMapTier = 1;

    // Current map "attempt" state (see spec: opening a map grants up to 6 portals --
    // i.e. up to 6 deaths recoverable without losing the map -- and the mods rolled on
    // the Waystone that opened it). Not persisted to disk: an in-progress map attempt
    // doesn't survive a full save/quit/reload (matches this project's existing "a zone's
    // live entity state isn't persisted" behavior, only the character/inventory/etc is).
    bool m_mapAttemptActive = false;
    int m_mapDeathsRemaining = 0;
    int m_mapDeathsMax = 0;
    std::vector<WaystoneMod> m_activeMapMods;
    // このマップアタempトの迷路生成シード(MapGenerator::GenerateRandomWalk)。
    // OpenEndgameMapで一度だけ振り直し、以後同じアタempト中(再入場含む)は使い回す
    // ことで「同じマップのポータルに入ったら別のマップになっている」を防ぐ。他の
    // アタempt状態と同じく非永続(保存/再読込を跨いで残す設計にはしていない)。
    unsigned int m_mapSeed = 0;
    // このマップアタempト中に死亡したスポーン枠番号の集合(MapSlotComponent参照、
    // トラッシュ/Rareは0..trashCount-1、ボスは-1固定)。ZoneBuilder::Buildは
    // 再構築(町へ戻って再入場等)のたびに、この集合に含まれる枠だけスポーンをスキップ
    // する。マップ生成自体をマップシードで決定論的にしてあるため、同じ枠には常に同じ
    // 個体(同じ座標・種族・レアリティ・アーケタイプ)が対応し、「倒した個体はもう
    // 出てこない、生きていた個体はまた同じ場所に出る」を実現する
    // ("雑魚敵も復活しないようにして…同じ個体で"という指示対応)。
    std::vector<int> m_deadEnemySlots;
    // 生存中に離脱した個体のcurrentHPを枠番号ごとに記録する(死亡時はNotifyEnemySlotDead
    // が削除する)。再構築時にここへ記録があれば、そのHPから再開する(無ければ満タン)。
    std::unordered_map<int, float> m_enemySlotHp;
    // 帰還用ポータルでタウンへ戻る時点の、マップ内MapComponent::visited(探索済み
    // タイル)のスナップショット。同じマップアタempト中の再入場ではこれを復元する
    // ("ミニマップがリセットされてる"というバグ報告対応)。タウンはZoneBuilder::Build
    // が常にRevealAllするため対象外。
    std::vector<uint8_t> m_mapVisitedTiles;

    // 同じゾーン種別内でのシーン再構築(例: Atlasでマップを開いた直後、タウンを円状
    // ポータル入りで作り直す)の直後だけ、プレイヤーをゾーン固定のスポーン地点では
    // なく直前の座標へ置くための一時的な引き継ぎ値("町でポータルを出すと位置がリ
    // セットされる"というフィードバック対応)。ゾーンをまたぐ遷移(タウン⇔マップ)は
    // 座標系が別物なので使わない。
    bool m_pendingSpawnOverrideValid = false;
    sf::Vector2f m_pendingSpawnOverride{ 0.f, 0.f };

    // 帰還用ポータル(GameScene::ReturnToHubViaPortal)でタウンへ戻った時点のマップ内
    // 座標。同じマップアタempト中に町側のポータルから再入場すると、ゾーン固定の
    // スポーン地点ではなくここへ置かれる("ポータルで町に戻った際に再度ポータルに
    // 入ると先ほど帰還用ポータルで出た位置からにする"という指示対応)。使うたびに
    // 上書きされる(消費されない、次にOpenEndgameMapが呼ばれる=新しいマップを開く
    // まで有効)。死亡による帰還はこれを更新しない(意図的に別経路のまま)。
    bool m_mapReturnPositionValid = false;
    sf::Vector2f m_mapReturnPosition{ 0.f, 0.f };

    // Which Atlas node (see AtlasSystem/AtlasData) the current map attempt was opened
    // from, if any -- set right before entering the map, consumed on a successful clear
    // (marks the node completed) or cleared without completing if the attempt ends in
    // failure (all 6 portals used). Not persisted, same reasoning as the map attempt
    // fields above.
    bool m_pendingAtlasNodeValid = false;
    int m_pendingAtlasCol = 0;
    int m_pendingAtlasRow = 0;

    bool m_hasSavedPlayer = false;
    CharacterStatsComponent m_savedStats;
    EquipmentComponent m_savedEquipment;
    std::vector<ItemComponent> m_savedInventory;
    std::array<std::vector<ItemComponent>, StashComponent::kTabCount> m_savedStash;
    std::vector<int> m_savedPassiveTree;
    // Skill/Spirit gems are equipped items now (see PlayerSkill::equippedItems /
    // SpiritGemLoadoutComponent::items, AI/DECISIONS.md "スキルジェムもアイテム欄に置く")
    // rather than a separate owned-gems list -- unequipped gems (identified or still
    // Uncut) simply round-trip as part of m_savedInventory/m_savedStash like any other item.
    std::array<std::optional<ItemComponent>, 5> m_savedSkillLoadout;
    std::array<std::optional<ItemComponent>, 5> m_savedAuraLoadout;
    std::array<bool, 5> m_savedAuraActive = { false, false, false, false, false };
    std::vector<std::pair<int, int>> m_savedAtlasNodes;
    bool m_isHardcore = false;

    void BuildActs();

public:
    bool IsHardcore() const { return m_isHardcore; }
    void SetHardcore(bool hardcore) { m_isHardcore = hardcore; }
    const ActDefinition& CurrentAct() const { return m_acts[m_actIndex]; }
    const ZoneDefinition& CurrentZone() const { return CurrentAct().zones[m_zoneIndex]; }
    int GetEndgameMapTier() const { return m_endgameMapTier; }

    // Spends a held Waystone item (tier + rolled mods, see ItemCategory::Waystone) to
    // open the endgame map zone at that tier, granting a fresh portal attempt (see
    // MaxMapDeathsForTier for how many). Returns false for an out-of-range tier.
    bool OpenEndgameMap(int tier, const std::vector<WaystoneMod>& mods);

    // Higher-tier Waystones grant fewer portals ("ウェイストーンのTierが上がると上限が
    // 減っていく" -- higher-risk maps get a smaller safety net). Capped at a minimum of 2
    // so a map is never a guaranteed one-death loss.
    static int MaxMapDeathsForTier(int tier) {
        if (tier <= 3) return 6;
        if (tier <= 6) return 5;
        if (tier <= 9) return 4;
        if (tier <= 12) return 3;
        return 2;
    }

    std::string GetProgressLabel() const;

    // 現在ゾーンをクリアして次へ進める(タウン↔マップの往復、常にエンドゲームループ)
    void CompleteCurrentZoneAndAdvance();

    // True while a map attempt (opened via OpenEndgameMap) still has portals left --
    // i.e. the hub's map device should let the player step back into THIS map for free
    // instead of requiring another Waystone (see GameScene::TryOpenEndgameMapFromHub).
    bool HasActiveMapAttempt() const { return m_mapAttemptActive; }
    int GetMapDeathsRemaining() const { return m_mapDeathsRemaining; }
    int GetMapDeathsMax() const { return m_mapDeathsMax; }
    unsigned int GetMapSeed() const { return m_mapSeed; }

    // スポーン枠(MapSlotComponent::slotIndex)が死亡したことを記録する(重複無視)。
    // 同時にそのHPスナップショットも消す(死んだ個体のHPは意味を持たないため)。
    void NotifyEnemySlotDead(int slotIndex) {
        if (std::find(m_deadEnemySlots.begin(), m_deadEnemySlots.end(), slotIndex) == m_deadEnemySlots.end()) {
            m_deadEnemySlots.push_back(slotIndex);
        }
        m_enemySlotHp.erase(slotIndex);
    }
    bool IsEnemySlotDead(int slotIndex) const {
        return std::find(m_deadEnemySlots.begin(), m_deadEnemySlots.end(), slotIndex) != m_deadEnemySlots.end();
    }
    // 生存中に離脱する個体のHPスナップショット(GameScene::ReturnToHubViaPortal等)。
    void SetEnemySlotHp(int slotIndex, float hp) { m_enemySlotHp[slotIndex] = hp; }
    bool GetEnemySlotHp(int slotIndex, float& outHp) const {
        auto it = m_enemySlotHp.find(slotIndex);
        if (it == m_enemySlotHp.end()) return false;
        outHp = it->second;
        return true;
    }
    const std::vector<WaystoneMod>& GetActiveMapMods() const { return m_activeMapMods; }

    // Call when the player dies inside a map (not the hub). Consumes one of the 6
    // portals; once none remain, the attempt closes (HasActiveMapAttempt() becomes
    // false) and the next map requires a fresh Waystone.
    void ConsumeMapDeath();

    // Atlas node tied to the currently-open map attempt (see AtlasSystem::TryOpenSelected/
    // GameScene's zone-cleared handling). GetPendingAtlasNode returns false (and leaves
    // col/row untouched) if no map was opened from an Atlas node this attempt.
    void SetPendingAtlasNode(int col, int row) { m_pendingAtlasNodeValid = true; m_pendingAtlasCol = col; m_pendingAtlasRow = row; }
    bool GetPendingAtlasNode(int& col, int& row) const {
        if (!m_pendingAtlasNodeValid) return false;
        col = m_pendingAtlasCol;
        row = m_pendingAtlasRow;
        return true;
    }
    void ClearPendingAtlasNode() { m_pendingAtlasNodeValid = false; }

    // 同一ゾーン内でのシーン再構築時だけプレイヤーのスポーン地点を上書きする
    // (ConsumePendingSpawnOverrideを呼んだ側が一度きりで消費する)。
    void SetPendingSpawnOverride(sf::Vector2f pos) { m_pendingSpawnOverrideValid = true; m_pendingSpawnOverride = pos; }
    bool ConsumePendingSpawnOverride(sf::Vector2f& outPos) {
        if (!m_pendingSpawnOverrideValid) return false;
        outPos = m_pendingSpawnOverride;
        m_pendingSpawnOverrideValid = false;
        return true;
    }

    // 探索済みタイルのスナップショット(上書きのみ、消費されない)。
    void SetMapVisitedTiles(std::vector<uint8_t> tiles) { m_mapVisitedTiles = std::move(tiles); }
    const std::vector<uint8_t>& GetMapVisitedTiles() const { return m_mapVisitedTiles; }

    // 帰還用ポータルでタウンへ戻った座標(消費されない、上書きのみ)。
    void SetMapReturnPosition(sf::Vector2f pos) { m_mapReturnPositionValid = true; m_mapReturnPosition = pos; }
    bool GetMapReturnPosition(sf::Vector2f& outPos) const {
        if (!m_mapReturnPositionValid) return false;
        outPos = m_mapReturnPosition;
        return true;
    }
    void ClearMapReturnPosition() { m_mapReturnPositionValid = false; }

    // エンドゲームハブへ戻す(死亡時、および自発的な帰還用ポータル使用時の両方から呼ばれる)。
    // m_mapAttemptActive/m_mapDeathsRemainingには触れない -- 死亡側はGameSceneが別途
    // ConsumeMapDeath()を呼ぶ、自発的な帰還(GameScene::ReturnToHubViaPortal)はポータルを
    // 消費せずアタemptを維持したまま戻るため、どちらもこの関数自体は中立でいる必要がある。
    void ReturnToLastTown();
    // タイトルからの新規開始
    void ResetCampaign();

    bool HasSavedPlayer() const { return m_hasSavedPlayer; }
    void SavePlayerStats(const CharacterStatsComponent& stats);
    const CharacterStatsComponent& GetSavedStats() const { return m_savedStats; }

    void SaveEquipment(const EquipmentComponent& equipment) { m_savedEquipment = equipment; }
    const EquipmentComponent& GetSavedEquipment() const { return m_savedEquipment; }

    void SaveInventory(const std::vector<ItemComponent>& items) { m_savedInventory = items; }
    const std::vector<ItemComponent>& GetSavedInventory() const { return m_savedInventory; }

    void SaveStash(const std::array<std::vector<ItemComponent>, StashComponent::kTabCount>& tabs) { m_savedStash = tabs; }
    const std::array<std::vector<ItemComponent>, StashComponent::kTabCount>& GetSavedStash() const { return m_savedStash; }

    void SavePassiveTree(const std::vector<int>& nodeIds) { m_savedPassiveTree = nodeIds; }
    const std::vector<int>& GetSavedPassiveTree() const { return m_savedPassiveTree; }

    void SaveSkillLoadout(const std::array<std::optional<ItemComponent>, 5>& items) { m_savedSkillLoadout = items; }
    const std::array<std::optional<ItemComponent>, 5>& GetSavedSkillLoadout() const { return m_savedSkillLoadout; }

    void SaveAuraLoadout(const std::array<std::optional<ItemComponent>, 5>& items) { m_savedAuraLoadout = items; }
    const std::array<std::optional<ItemComponent>, 5>& GetSavedAuraLoadout() const { return m_savedAuraLoadout; }

    void SaveAuraActive(const std::array<bool, 5>& active) { m_savedAuraActive = active; }
    const std::array<bool, 5>& GetSavedAuraActive() const { return m_savedAuraActive; }

    // Atlas progress (see AtlasComponent/AtlasData). Stored as plain (col, row) pairs
    // rather than AtlasData's packed key format, so CampaignManager stays agnostic of
    // that encoding (same reasoning as m_savedPassiveTree storing raw node ids).
    void SaveAtlasNodes(const std::vector<std::pair<int, int>>& nodes) { m_savedAtlasNodes = nodes; }
    const std::vector<std::pair<int, int>>& GetSavedAtlasNodes() const { return m_savedAtlasNodes; }

    void SaveToDisk(const std::string& path = "save.dat") const;
    bool LoadFromDisk(const std::string& path = "save.dat");
    static bool SaveFileExists(const std::string& path = "save.dat");
    static void DeleteSaveFile(const std::string& path = "save.dat");
};
