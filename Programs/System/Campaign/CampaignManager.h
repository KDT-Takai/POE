#pragma once
#include <string>
#include <vector>
#include <array>
#include <SFML/Graphics/Color.hpp>
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "Components/Item/Equipment.h"
#include "Components/Item/Inventory.h"
#include "Components/Item/Waystone.h"
#include "Components/Item/Stash.h"
#include "Components/Item/SkillGem.h"
#include "../Singleton/Singleton.h"

enum class ZoneKind { Town, Combat };

struct ZoneDefinition {
    std::string id;
    std::string displayName;
    ZoneKind kind = ZoneKind::Combat;
    bool isBossZone = false;

    int mapWidth = 60;
    int mapHeight = 60;

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

    bool m_hasSavedPlayer = false;
    CharacterStatsComponent m_savedStats;
    EquipmentComponent m_savedEquipment;
    std::vector<ItemComponent> m_savedInventory;
    std::array<std::vector<ItemComponent>, StashComponent::kTabCount> m_savedStash;
    std::vector<int> m_savedPassiveTree;
    std::vector<OwnedGemInstance> m_savedOwnedGems;
    std::vector<PendingUncutGem> m_savedPendingUncutGems;
    std::array<int, 5> m_savedSkillLoadout = { -1, -1, -1, -1, -1 };
    std::array<int, 5> m_savedAuraLoadout = { -1, -1, -1, -1, -1 };
    std::array<bool, 5> m_savedAuraActive = { false, false, false, false, false };
    std::array<int, WaystoneInventoryComponent::kMaxTier> m_savedWaystones{};
    bool m_isHardcore = false;

    void BuildActs();

public:
    bool IsHardcore() const { return m_isHardcore; }
    void SetHardcore(bool hardcore) { m_isHardcore = hardcore; }
    const ActDefinition& CurrentAct() const { return m_acts[m_actIndex]; }
    const ZoneDefinition& CurrentZone() const { return CurrentAct().zones[m_zoneIndex]; }
    int GetEndgameMapTier() const { return m_endgameMapTier; }

    // Spends a held Waystone (tier chosen by the caller, see WaystoneInventoryComponent)
    // to open the endgame map zone at that tier. Returns false for an out-of-range tier.
    bool OpenEndgameMap(int tier);

    std::string GetProgressLabel() const;

    // 現在ゾーンをクリアして次へ進める(タウン↔マップの往復、常にエンドゲームループ)
    void CompleteCurrentZoneAndAdvance();

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

    void SaveOwnedGems(const std::vector<OwnedGemInstance>& gems) { m_savedOwnedGems = gems; }
    const std::vector<OwnedGemInstance>& GetSavedOwnedGems() const { return m_savedOwnedGems; }

    void SavePendingUncutGems(const std::vector<PendingUncutGem>& gems) { m_savedPendingUncutGems = gems; }
    const std::vector<PendingUncutGem>& GetSavedPendingUncutGems() const { return m_savedPendingUncutGems; }

    void SaveSkillLoadout(const std::array<int, 5>& gemIds) { m_savedSkillLoadout = gemIds; }
    const std::array<int, 5>& GetSavedSkillLoadout() const { return m_savedSkillLoadout; }

    void SaveAuraLoadout(const std::array<int, 5>& gemIds) { m_savedAuraLoadout = gemIds; }
    const std::array<int, 5>& GetSavedAuraLoadout() const { return m_savedAuraLoadout; }

    void SaveAuraActive(const std::array<bool, 5>& active) { m_savedAuraActive = active; }
    const std::array<bool, 5>& GetSavedAuraActive() const { return m_savedAuraActive; }

    void SaveWaystones(const std::array<int, WaystoneInventoryComponent::kMaxTier>& counts) { m_savedWaystones = counts; }
    const std::array<int, WaystoneInventoryComponent::kMaxTier>& GetSavedWaystones() const { return m_savedWaystones; }

    void SaveToDisk(const std::string& path = "save.dat") const;
    bool LoadFromDisk(const std::string& path = "save.dat");
    static bool SaveFileExists(const std::string& path = "save.dat");
    static void DeleteSaveFile(const std::string& path = "save.dat");
};
