#pragma once
#include <string>
#include <vector>
#include <SFML/Graphics/Color.hpp>
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "Components/Item/Equipment.h"
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
    bool m_isHardcore = false;

    void BuildActs();

public:
    bool IsHardcore() const { return m_isHardcore; }
    void SetHardcore(bool hardcore) { m_isHardcore = hardcore; }
    const ActDefinition& CurrentAct() const { return m_acts[m_actIndex]; }
    const ZoneDefinition& CurrentZone() const { return CurrentAct().zones[m_zoneIndex]; }
    int GetEndgameMapTier() const { return m_endgameMapTier; }

    std::string GetProgressLabel() const;

    // 現在ゾーンをクリアして次へ進める(タウン/エンドゲームも含めて自動判定)
    void CompleteCurrentZoneAndAdvance();
    // 死亡時: 現在の幕の入口タウンへ戻す
    void ReturnToLastTown();
    // タイトルからの新規開始
    void ResetCampaign();

    bool HasSavedPlayer() const { return m_hasSavedPlayer; }
    void SavePlayerStats(const CharacterStatsComponent& stats);
    const CharacterStatsComponent& GetSavedStats() const { return m_savedStats; }

    void SaveEquipment(const EquipmentComponent& equipment) { m_savedEquipment = equipment; }
    const EquipmentComponent& GetSavedEquipment() const { return m_savedEquipment; }

    void SaveToDisk(const std::string& path = "save.dat") const;
    bool LoadFromDisk(const std::string& path = "save.dat");
    static bool SaveFileExists(const std::string& path = "save.dat");
    static void DeleteSaveFile(const std::string& path = "save.dat");
};
