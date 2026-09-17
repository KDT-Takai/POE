#pragma once
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <SFML/Graphics/Color.hpp>
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
    std::vector<WaystoneMod> m_activeMapMods;

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
    // open the endgame map zone at that tier, granting a fresh 6-portal attempt. Returns
    // false for an out-of-range tier.
    bool OpenEndgameMap(int tier, const std::vector<WaystoneMod>& mods);

    std::string GetProgressLabel() const;

    // 現在ゾーンをクリアして次へ進める(タウン↔マップの往復、常にエンドゲームループ)
    void CompleteCurrentZoneAndAdvance();

    // True while a map attempt (opened via OpenEndgameMap) still has portals left --
    // i.e. the hub's map device should let the player step back into THIS map for free
    // instead of requiring another Waystone (see GameScene::TryOpenEndgameMapFromHub).
    bool HasActiveMapAttempt() const { return m_mapAttemptActive; }
    int GetMapDeathsRemaining() const { return m_mapDeathsRemaining; }
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

    // 死亡時: エンドゲームハブへ戻す
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
