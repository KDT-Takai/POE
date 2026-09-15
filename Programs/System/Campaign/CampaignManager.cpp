#include "CampaignManager.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstdio>

namespace {
    void WriteStats(std::ofstream& out, const std::string& prefix, const CharacterStatsComponent& s) {
        out << prefix << "name=" << s.name << "\n";
        out << prefix << "level=" << s.level << "\n";
        out << prefix << "currentXP=" << s.currentXP << "\n";
        out << prefix << "xpToNextLevel=" << s.xpToNextLevel << "\n";
        out << prefix << "currentHP=" << s.currentHP << "\n";
        out << prefix << "maxHP=" << s.maxHP << "\n";
        out << prefix << "healthRegen=" << s.healthRegen << "\n";
        out << prefix << "currentMP=" << s.currentMP << "\n";
        out << prefix << "maxMP=" << s.maxMP << "\n";
        out << prefix << "manaRegen=" << s.manaRegen << "\n";
        out << prefix << "currentES=" << s.currentES << "\n";
        out << prefix << "maxES=" << s.maxES << "\n";
        out << prefix << "maxSpirit=" << s.maxSpirit << "\n";
        out << prefix << "currentSpirit=" << s.currentSpirit << "\n";
        out << prefix << "str=" << s.str << "\n";
        out << prefix << "dex=" << s.dex << "\n";
        out << prefix << "intelligence=" << s.intelligence << "\n";
        out << prefix << "atk=" << s.atk << "\n";
        out << prefix << "def=" << s.def << "\n";
        out << prefix << "atkSpd=" << s.atkSpd << "\n";
        out << prefix << "critRate=" << s.critRate << "\n";
        out << prefix << "critDamage=" << s.critDamage << "\n";
        out << prefix << "moveSpeed=" << s.moveSpeed << "\n";
        out << prefix << "rollSpeed=" << s.rollSpeed << "\n";
        out << prefix << "rollDuration=" << s.rollDuration << "\n";
        out << prefix << "rollCooldownMax=" << s.rollCooldownMax << "\n";
        out << prefix << "fireRes=" << s.fireRes << "\n";
        out << prefix << "iceRes=" << s.iceRes << "\n";
        out << prefix << "lightningRes=" << s.lightningRes << "\n";
        out << prefix << "chaosRes=" << s.chaosRes << "\n";
        out << prefix << "evasion=" << s.evasion << "\n";
        out << prefix << "armour=" << s.armour << "\n";
        out << prefix << "accuracy=" << s.accuracy << "\n";
        out << prefix << "leechPercent=" << s.leechPercent << "\n";
        out << prefix << "leechRateCap=" << s.leechRateCap << "\n";
        out << prefix << "rarity=" << static_cast<int>(s.rarity) << "\n";
        out << prefix << "contactDamageType=" << static_cast<int>(s.contactDamageType) << "\n";
        out << prefix << "gold=" << s.gold << "\n";
    }

    float GetF(const std::unordered_map<std::string, std::string>& m, const std::string& k, float def) {
        auto it = m.find(k);
        return it == m.end() ? def : std::stof(it->second);
    }
    int GetI(const std::unordered_map<std::string, std::string>& m, const std::string& k, int def) {
        auto it = m.find(k);
        return it == m.end() ? def : std::stoi(it->second);
    }
    std::string GetS(const std::unordered_map<std::string, std::string>& m, const std::string& k, const std::string& def) {
        auto it = m.find(k);
        return it == m.end() ? def : it->second;
    }

    void WriteItem(std::ofstream& out, const std::string& prefix, const ItemComponent& item) {
        out << prefix << "slot=" << static_cast<int>(item.slot) << "\n";
        out << prefix << "baseName=" << item.baseName << "\n";
        out << prefix << "rarity=" << static_cast<int>(item.rarity) << "\n";
        out << prefix << "itemLevel=" << item.itemLevel << "\n";
        out << prefix << "baseValue=" << item.baseValue << "\n";
        out << prefix << "affixCount=" << item.affixes.size() << "\n";
        for (size_t a = 0; a < item.affixes.size(); ++a) {
            std::string aprefix = prefix + "affix" + std::to_string(a) + ".";
            const ItemAffix& affix = item.affixes[a];
            out << aprefix << "stat=" << static_cast<int>(affix.stat) << "\n";
            out << aprefix << "value=" << affix.value << "\n";
            out << aprefix << "tier=" << affix.tier << "\n";
            out << aprefix << "isPrefix=" << (affix.isPrefix ? 1 : 0) << "\n";
        }
    }

    ItemComponent ReadItem(const std::unordered_map<std::string, std::string>& kv, const std::string& prefix, EquipSlot defaultSlot) {
        ItemComponent item;
        item.slot = static_cast<EquipSlot>(GetI(kv, prefix + "slot", static_cast<int>(defaultSlot)));
        item.baseName = GetS(kv, prefix + "baseName", "Item");
        item.rarity = static_cast<ItemRarity>(GetI(kv, prefix + "rarity", 0));
        item.itemLevel = GetI(kv, prefix + "itemLevel", 1);
        item.baseValue = GetF(kv, prefix + "baseValue", 0.0f);

        int affixCount = GetI(kv, prefix + "affixCount", 0);
        for (int a = 0; a < affixCount; ++a) {
            std::string aprefix = prefix + "affix" + std::to_string(a) + ".";
            ItemAffix affix;
            affix.stat = static_cast<AffixStat>(GetI(kv, aprefix + "stat", 0));
            affix.value = GetF(kv, aprefix + "value", 0.0f);
            affix.tier = GetI(kv, aprefix + "tier", 1);
            affix.isPrefix = GetI(kv, aprefix + "isPrefix", 1) != 0;
            item.affixes.push_back(affix);
        }
        return item;
    }

    void ReadStats(const std::unordered_map<std::string, std::string>& m, const std::string& prefix, CharacterStatsComponent& s) {
        s.name = GetS(m, prefix + "name", s.name);
        s.level = GetI(m, prefix + "level", s.level);
        s.currentXP = GetI(m, prefix + "currentXP", s.currentXP);
        s.xpToNextLevel = GetI(m, prefix + "xpToNextLevel", s.xpToNextLevel);
        s.currentHP = GetF(m, prefix + "currentHP", s.currentHP);
        s.maxHP = GetF(m, prefix + "maxHP", s.maxHP);
        s.healthRegen = GetF(m, prefix + "healthRegen", s.healthRegen);
        s.currentMP = GetF(m, prefix + "currentMP", s.currentMP);
        s.maxMP = GetF(m, prefix + "maxMP", s.maxMP);
        s.manaRegen = GetF(m, prefix + "manaRegen", s.manaRegen);
        s.currentES = GetF(m, prefix + "currentES", s.currentES);
        s.maxES = GetF(m, prefix + "maxES", s.maxES);
        s.maxSpirit = GetF(m, prefix + "maxSpirit", s.maxSpirit);
        s.currentSpirit = GetF(m, prefix + "currentSpirit", s.currentSpirit);
        s.str = GetI(m, prefix + "str", s.str);
        s.dex = GetI(m, prefix + "dex", s.dex);
        s.intelligence = GetI(m, prefix + "intelligence", s.intelligence);
        s.atk = GetF(m, prefix + "atk", s.atk);
        s.def = GetF(m, prefix + "def", s.def);
        s.atkSpd = GetF(m, prefix + "atkSpd", s.atkSpd);
        s.critRate = GetF(m, prefix + "critRate", s.critRate);
        s.critDamage = GetF(m, prefix + "critDamage", s.critDamage);
        s.moveSpeed = GetF(m, prefix + "moveSpeed", s.moveSpeed);
        s.rollSpeed = GetF(m, prefix + "rollSpeed", s.rollSpeed);
        s.rollDuration = GetF(m, prefix + "rollDuration", s.rollDuration);
        s.rollCooldownMax = GetF(m, prefix + "rollCooldownMax", s.rollCooldownMax);
        s.fireRes = GetF(m, prefix + "fireRes", s.fireRes);
        s.iceRes = GetF(m, prefix + "iceRes", s.iceRes);
        s.lightningRes = GetF(m, prefix + "lightningRes", s.lightningRes);
        s.chaosRes = GetF(m, prefix + "chaosRes", s.chaosRes);
        s.evasion = GetF(m, prefix + "evasion", s.evasion);
        s.armour = GetF(m, prefix + "armour", s.armour);
        s.accuracy = GetF(m, prefix + "accuracy", s.accuracy);
        s.leechPercent = GetF(m, prefix + "leechPercent", s.leechPercent);
        s.leechRateCap = GetF(m, prefix + "leechRateCap", s.leechRateCap);
        s.rarity = static_cast<MonsterRarity>(GetI(m, prefix + "rarity", static_cast<int>(s.rarity)));
        s.contactDamageType = static_cast<DamageElement>(GetI(m, prefix + "contactDamageType", static_cast<int>(s.contactDamageType)));
        s.gold = GetI(m, prefix + "gold", s.gold);
    }
}

namespace {
    ZoneDefinition MakeTown(std::string id, std::string name) {
        ZoneDefinition z;
        z.id = std::move(id);
        z.displayName = std::move(name);
        z.kind = ZoneKind::Town;
        z.mapWidth = 30;
        z.mapHeight = 30;
        z.enemyCount = 0;
        return z;
    }

    ZoneDefinition MakeCombat(std::string id, std::string name, int w, int h, int count,
        float hpMult, float atkMult, std::string enemyType, sf::Color color,
        DamageElement element = DamageElement::Physical) {
        ZoneDefinition z;
        z.id = std::move(id);
        z.displayName = std::move(name);
        z.kind = ZoneKind::Combat;
        z.mapWidth = w;
        z.mapHeight = h;
        z.enemyCount = count;
        z.enemyHpMult = hpMult;
        z.enemyAtkMult = atkMult;
        z.enemyTypeName = std::move(enemyType);
        z.enemyColor = color;
        z.enemyContactType = element;
        return z;
    }

    ZoneDefinition MakeBoss(std::string id, std::string name, int w, int h, int trashCount,
        float hpMult, float atkMult, std::string enemyType, sf::Color color,
        std::string bossName, float bossHpMult, float bossAtkMult, sf::Color bossColor,
        DamageElement element = DamageElement::Physical) {
        ZoneDefinition z = MakeCombat(std::move(id), std::move(name), w, h, trashCount, hpMult, atkMult, std::move(enemyType), color, element);
        z.isBossZone = true;
        z.bossName = std::move(bossName);
        z.bossHpMult = bossHpMult;
        z.bossAtkMult = bossAtkMult;
        z.bossColor = bossColor;
        return z;
    }
}

CampaignManager::CampaignManager() {
    BuildActs();
}

void CampaignManager::BuildActs() {
    m_acts.clear();

    // Act1: 海岸の章
    {
        ActDefinition act;
        act.id = "act1";
        act.displayName = "第一幕：海岸の章";
        act.zones.push_back(MakeTown("act1_town", "漂着の野営地"));
        act.zones.push_back(MakeCombat("act1_z1", "潮騒の浜辺", 50, 50, 15, 1.0f, 1.0f, "漂流者", sf::Color(60, 160, 160), DamageElement::Cold));
        act.zones.push_back(MakeCombat("act1_z2", "朽ちた桟橋", 55, 55, 20, 1.1f, 1.1f, "漂流者", sf::Color(60, 160, 160), DamageElement::Cold));
        act.zones.push_back(MakeCombat("act1_z3", "難破船の内部", 60, 60, 22, 1.2f, 1.2f, "呪われし船員", sf::Color(40, 120, 140), DamageElement::Cold));
        act.zones.push_back(MakeBoss("act1_boss", "波止場の番人", 45, 45, 8, 1.2f, 1.2f, "漂流者", sf::Color(60, 160, 160),
            "溺れの巨兵ドレウォス", 9.0f, 2.5f, sf::Color(20, 90, 120), DamageElement::Cold));
        m_acts.push_back(act);
    }

    // Act2: 砂漠の章
    {
        ActDefinition act;
        act.id = "act2";
        act.displayName = "第二幕：砂漠の章";
        act.zones.push_back(MakeTown("act2_town", "涸れ井戸の集落"));
        act.zones.push_back(MakeCombat("act2_z1", "灼熱の砂丘", 60, 60, 18, 1.4f, 1.3f, "砂賊", sf::Color(200, 170, 80), DamageElement::Fire));
        act.zones.push_back(MakeCombat("act2_z2", "埋没した神殿", 60, 60, 22, 1.5f, 1.4f, "神殿の守り手", sf::Color(180, 140, 60), DamageElement::Fire));
        act.zones.push_back(MakeCombat("act2_z3", "盗賊の隠れ家", 65, 65, 24, 1.6f, 1.5f, "砂賊", sf::Color(200, 170, 80), DamageElement::Fire));
        act.zones.push_back(MakeBoss("act2_boss", "神殿の最奥", 50, 50, 10, 1.6f, 1.5f, "神殿の守り手", sf::Color(180, 140, 60),
            "砂の女王ネフェリス", 10.0f, 3.0f, sf::Color(220, 190, 40), DamageElement::Fire));
        m_acts.push_back(act);
    }

    // 幕間I: 静寂の幕間
    {
        ActDefinition act;
        act.id = "interlude1";
        act.displayName = "幕間：静寂の幕間";
        act.zones.push_back(MakeTown("interlude1_town", "隠れ家の避難所"));
        act.zones.push_back(MakeCombat("interlude1_trial", "試練の回廊", 55, 55, 20, 1.7f, 1.6f, "彷徨う影", sf::Color(120, 100, 160), DamageElement::Chaos));
        m_acts.push_back(act);
    }

    // Act3: 王都の章
    {
        ActDefinition act;
        act.id = "act3";
        act.displayName = "第三幕：王都の章";
        act.zones.push_back(MakeTown("act3_town", "灰の門前町"));
        act.zones.push_back(MakeCombat("act3_z1", "崩れた外壁", 65, 65, 20, 1.9f, 1.8f, "王都の兵", sf::Color(150, 60, 60)));
        act.zones.push_back(MakeCombat("act3_z2", "地下墓地", 65, 65, 24, 2.0f, 1.9f, "蘇りし死者", sf::Color(120, 120, 120)));
        act.zones.push_back(MakeCombat("act3_z3", "王城の広間", 70, 70, 26, 2.1f, 2.0f, "王都の兵", sf::Color(150, 60, 60)));
        act.zones.push_back(MakeBoss("act3_boss", "玉座の間", 55, 55, 12, 2.1f, 2.0f, "王都の兵", sf::Color(150, 60, 60),
            "堕ちた王グレイヴァルト", 12.0f, 3.5f, sf::Color(90, 20, 30)));
        m_acts.push_back(act);
    }

    // Act4: 山嶺の章
    {
        ActDefinition act;
        act.id = "act4";
        act.displayName = "第四幕：山嶺の章";
        act.zones.push_back(MakeTown("act4_town", "山麓の砦"));
        act.zones.push_back(MakeCombat("act4_z1", "溶岩の亀裂", 70, 70, 22, 2.4f, 2.2f, "灰塵の獣", sf::Color(220, 110, 40), DamageElement::Fire));
        act.zones.push_back(MakeCombat("act4_z2", "灰塵の洞窟", 70, 70, 26, 2.6f, 2.4f, "灰塵の獣", sf::Color(220, 110, 40), DamageElement::Fire));
        act.zones.push_back(MakeCombat("act4_z3", "神々の座への道", 75, 75, 28, 2.8f, 2.6f, "山嶺の守護者", sf::Color(200, 90, 30), DamageElement::Fire));
        act.zones.push_back(MakeBoss("act4_boss", "最果ての祭壇", 60, 60, 14, 2.8f, 2.6f, "山嶺の守護者", sf::Color(200, 90, 30),
            "終焉の守護者イグナロス", 15.0f, 4.0f, sf::Color(255, 60, 20), DamageElement::Fire));
        m_acts.push_back(act);
    }

    // 幕間II: 終端の幕間
    {
        ActDefinition act;
        act.id = "interlude2";
        act.displayName = "幕間：終端の幕間";
        act.zones.push_back(MakeTown("interlude2_town", "終端の祭壇"));
        act.zones.push_back(MakeCombat("interlude2_trial", "深淵の裂け目", 65, 65, 26, 3.0f, 2.8f, "深淵の眷属", sf::Color(80, 40, 120), DamageElement::Chaos));
        m_acts.push_back(act);
    }

    // Endgame: 地図の狭間 (無限ループ、ティアで強くなる)
    {
        ActDefinition act;
        act.id = "endgame";
        act.displayName = "エンドゲーム：地図の狭間";
        act.isEndgame = true;
        act.zones.push_back(MakeTown("endgame_hub", "地図の間"));
        act.zones.push_back(MakeBoss("endgame_map", "歪んだ地図", 70, 70, 18, 3.2f, 3.0f, "歪みの落とし子", sf::Color(200, 40, 160),
            "地図の歪みの化身", 14.0f, 3.5f, sf::Color(255, 0, 120), DamageElement::Lightning));
        m_acts.push_back(act);
    }
}

std::string CampaignManager::GetProgressLabel() const {
    const auto& act = CurrentAct();
    const auto& zone = CurrentZone();
    std::string label = act.displayName + " - " + zone.displayName;
    if (act.isEndgame && zone.kind == ZoneKind::Combat) {
        label += "  (Tier " + std::to_string(m_endgameMapTier) + ")";
    }
    return label;
}

void CampaignManager::CompleteCurrentZoneAndAdvance() {
    const auto& act = CurrentAct();

    if (act.isEndgame) {
        if (CurrentZone().kind == ZoneKind::Combat) {
            m_endgameMapTier++;
        }
        m_zoneIndex = (m_zoneIndex + 1) % static_cast<int>(act.zones.size());
        return;
    }

    m_zoneIndex++;
    if (m_zoneIndex >= static_cast<int>(act.zones.size())) {
        m_zoneIndex = 0;
        m_actIndex++;
        if (m_actIndex >= static_cast<int>(m_acts.size())) {
            m_actIndex = static_cast<int>(m_acts.size()) - 1; // Endgameに留まる
        }
    }
}

void CampaignManager::ReturnToLastTown() {
    m_zoneIndex = 0; // 各幕/幕間/エンドゲームの先頭は必ずタウン
    if (m_hasSavedPlayer) {
        m_savedStats.currentHP = m_savedStats.maxHP;
        m_savedStats.currentMP = m_savedStats.maxMP;
        m_savedStats.currentES = m_savedStats.maxES;
    }
}

void CampaignManager::ResetCampaign() {
    m_actIndex = 0;
    m_zoneIndex = 0;
    m_endgameMapTier = 1;
    m_hasSavedPlayer = false;
    m_savedEquipment = EquipmentComponent{};
    m_savedInventory.clear();
}

void CampaignManager::SavePlayerStats(const CharacterStatsComponent& stats) {
    m_savedStats = stats;
    m_hasSavedPlayer = true;
}

bool CampaignManager::SaveFileExists(const std::string& path) {
    std::ifstream in(path);
    return in.good();
}

void CampaignManager::DeleteSaveFile(const std::string& path) {
    std::remove(path.c_str());
}

void CampaignManager::SaveToDisk(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;

    out << "actIndex=" << m_actIndex << "\n";
    out << "zoneIndex=" << m_zoneIndex << "\n";
    out << "endgameMapTier=" << m_endgameMapTier << "\n";
    out << "hasSavedPlayer=" << (m_hasSavedPlayer ? 1 : 0) << "\n";
    out << "isHardcore=" << (m_isHardcore ? 1 : 0) << "\n";

    WriteStats(out, "stats.", m_savedStats);
    WriteStats(out, "base.", m_savedEquipment.baseStats);

    for (size_t i = 0; i < m_savedEquipment.slots.size(); ++i) {
        std::string prefix = "equip.slot" + std::to_string(i) + ".";
        const auto& slot = m_savedEquipment.slots[i];
        out << prefix << "present=" << (slot.has_value() ? 1 : 0) << "\n";
        if (!slot.has_value()) continue;
        WriteItem(out, prefix, *slot);
    }

    out << "inventory.count=" << m_savedInventory.size() << "\n";
    for (size_t i = 0; i < m_savedInventory.size(); ++i) {
        std::string prefix = "inventory.item" + std::to_string(i) + ".";
        WriteItem(out, prefix, m_savedInventory[i]);
    }
}

bool CampaignManager::LoadFromDisk(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;

    std::unordered_map<std::string, std::string> kv;
    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        kv[line.substr(0, eq)] = line.substr(eq + 1);
    }

    m_actIndex = GetI(kv, "actIndex", 0);
    m_zoneIndex = GetI(kv, "zoneIndex", 0);
    m_endgameMapTier = GetI(kv, "endgameMapTier", 1);
    m_isHardcore = GetI(kv, "isHardcore", 0) != 0;
    m_hasSavedPlayer = GetI(kv, "hasSavedPlayer", 0) != 0;

    if (m_actIndex >= static_cast<int>(m_acts.size())) m_actIndex = static_cast<int>(m_acts.size()) - 1;
    if (m_actIndex < 0) m_actIndex = 0;
    if (m_zoneIndex >= static_cast<int>(m_acts[m_actIndex].zones.size())) m_zoneIndex = 0;

    ReadStats(kv, "stats.", m_savedStats);
    ReadStats(kv, "base.", m_savedEquipment.baseStats);

    for (size_t i = 0; i < m_savedEquipment.slots.size(); ++i) {
        std::string prefix = "equip.slot" + std::to_string(i) + ".";
        if (GetI(kv, prefix + "present", 0) == 0) {
            m_savedEquipment.slots[i].reset();
            continue;
        }
        m_savedEquipment.slots[i] = ReadItem(kv, prefix, static_cast<EquipSlot>(i));
    }

    m_savedInventory.clear();
    int inventoryCount = GetI(kv, "inventory.count", 0);
    for (int i = 0; i < inventoryCount; ++i) {
        std::string prefix = "inventory.item" + std::to_string(i) + ".";
        m_savedInventory.push_back(ReadItem(kv, prefix, EquipSlot::Weapon));
    }

    return true;
}
