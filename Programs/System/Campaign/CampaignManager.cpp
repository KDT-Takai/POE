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
        out << prefix << "increasedAttackDamage=" << s.increasedAttackDamage << "\n";
        out << prefix << "increasedMoveSpeed=" << s.increasedMoveSpeed << "\n";
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
        out << prefix << "blockChance=" << s.blockChance << "\n";
        out << prefix << "leechPercent=" << s.leechPercent << "\n";
        out << prefix << "leechRateCap=" << s.leechRateCap << "\n";
        out << prefix << "rarity=" << static_cast<int>(s.rarity) << "\n";
        out << prefix << "contactDamageType=" << static_cast<int>(s.contactDamageType) << "\n";
        out << prefix << "gold=" << s.gold << "\n";
        out << prefix << "passivePoints=" << s.passivePoints << "\n";
        out << prefix << "regretOrbs=" << s.regretOrbs << "\n";
        out << prefix << "jewellersOrbs=" << s.jewellersOrbs << "\n";
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
        out << prefix << "category=" << static_cast<int>(item.category) << "\n";
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
        out << prefix << "waystoneTier=" << item.waystoneTier << "\n";
        out << prefix << "waystoneModCount=" << item.waystoneMods.size() << "\n";
        for (size_t m = 0; m < item.waystoneMods.size(); ++m) {
            std::string mprefix = prefix + "waystoneMod" + std::to_string(m) + ".";
            const WaystoneMod& mod = item.waystoneMods[m];
            out << mprefix << "stat=" << static_cast<int>(mod.stat) << "\n";
            out << mprefix << "value=" << mod.value << "\n";
        }
    }

    ItemComponent ReadItem(const std::unordered_map<std::string, std::string>& kv, const std::string& prefix, EquipSlot defaultSlot) {
        ItemComponent item;
        // Pre-Waystone-item saves have no "category" key -- defaults to Gear, same
        // fallback convention as every other post-launch field addition.
        item.category = static_cast<ItemCategory>(GetI(kv, prefix + "category", static_cast<int>(ItemCategory::Gear)));
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

        item.waystoneTier = GetI(kv, prefix + "waystoneTier", 0);
        int modCount = GetI(kv, prefix + "waystoneModCount", 0);
        for (int m = 0; m < modCount; ++m) {
            std::string mprefix = prefix + "waystoneMod" + std::to_string(m) + ".";
            WaystoneMod mod;
            mod.stat = static_cast<WaystoneModStat>(GetI(kv, mprefix + "stat", 0));
            mod.value = GetF(kv, mprefix + "value", 0.0f);
            item.waystoneMods.push_back(mod);
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
        s.increasedAttackDamage = GetF(m, prefix + "increasedAttackDamage", s.increasedAttackDamage);
        s.increasedMoveSpeed = GetF(m, prefix + "increasedMoveSpeed", s.increasedMoveSpeed);
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
        s.blockChance = GetF(m, prefix + "blockChance", s.blockChance);
        s.leechPercent = GetF(m, prefix + "leechPercent", s.leechPercent);
        s.leechRateCap = GetF(m, prefix + "leechRateCap", s.leechRateCap);
        s.rarity = static_cast<MonsterRarity>(GetI(m, prefix + "rarity", static_cast<int>(s.rarity)));
        s.contactDamageType = static_cast<DamageElement>(GetI(m, prefix + "contactDamageType", static_cast<int>(s.contactDamageType)));
        s.gold = GetI(m, prefix + "gold", s.gold);
        s.passivePoints = GetI(m, prefix + "passivePoints", s.passivePoints);
        s.regretOrbs = GetI(m, prefix + "regretOrbs", s.regretOrbs);
        s.jewellersOrbs = GetI(m, prefix + "jewellersOrbs", s.jewellersOrbs);
    }
}

namespace {
    ZoneDefinition MakeTown(std::string id, std::string name) {
        ZoneDefinition z;
        z.id = std::move(id);
        z.displayName = std::move(name);
        z.kind = ZoneKind::Town;
        z.mapWidth = 16;
        z.mapHeight = 12;
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

    // Act1-4/幕間のストーリーキャンペーンは廃止し、Waystone→Map→クリアのPoE2エンドゲーム
    // ループのみを再現する構成にした(2026-09-17、ユーザー指示)。プレイヤーは最初から
    // このエンドゲームハブでスタートする。
    // Endgame: 地図の狭間 (無限ループ、ティアで強くなる)
    {
        ActDefinition act;
        act.id = "endgame";
        act.displayName = "エンドゲーム：地図の狭間";
        act.isEndgame = true;
        act.zones.push_back(MakeTown("endgame_hub", "地図の間"));
        // 90x90(旧70x70から拡張、「マップをもう少し広くして」の指示対応)。敵数は
        // 面積比(8100/4900)に合わせて18→30へ増やし、広くなった分だけ間延びしないようにした。
        act.zones.push_back(MakeBoss("endgame_map", "歪んだ地図", 90, 90, 30, 3.2f, 3.0f, "歪みの落とし子", sf::Color(200, 40, 160),
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

bool CampaignManager::OpenEndgameMap(int tier, const std::vector<WaystoneMod>& mods) {
    if (tier < 1 || tier > kMaxWaystoneTier) return false;
    m_endgameMapTier = tier;
    m_activeMapMods = mods;
    m_mapAttemptActive = true;
    m_mapDeathsRemaining = 6;
    // Deliberately does NOT touch m_zoneIndex here -- this is always called from the hub
    // (index 0), and the caller follows up with CompleteCurrentZoneAndAdvance's 0<->1
    // toggle to actually move into the map. Setting it directly here used to fight that
    // same toggle (both ran back-to-back and cancelled out, leaving the player stuck at
    // the hub instead of entering the map -- fixed while adding the portal-attempt system).
    return true;
}

void CampaignManager::CompleteCurrentZoneAndAdvance() {
    const auto& act = CurrentAct();
    // The map's tier is chosen by which Waystone the player spends at the hub's map
    // device (see OpenEndgameMap), not by an auto-incrementing counter here. Only one
    // Act (the endgame loop) exists, so this always just toggles hub(0)<->map(1).
    m_zoneIndex = (m_zoneIndex + 1) % static_cast<int>(act.zones.size());

    // Arriving back at the hub this way only ever happens by clearing the map (see
    // GameScene's "Zone cleared!" check) -- a death goes through ReturnToLastTown
    // instead, which doesn't call this. So this is always "the attempt is over,
    // successfully" -- close it out so a free re-entry isn't offered for a map that's
    // already been cleared.
    if (m_zoneIndex == 0) {
        m_mapAttemptActive = false;
    }
}

void CampaignManager::ConsumeMapDeath() {
    if (m_mapDeathsRemaining <= 0) return;
    m_mapDeathsRemaining--;
    if (m_mapDeathsRemaining <= 0) {
        m_mapAttemptActive = false;
        // The attempt ended in failure (portals exhausted), not a clear -- don't leave a
        // stale pending node around for a future successful attempt to wrongly complete.
        m_pendingAtlasNodeValid = false;
    }
}

void CampaignManager::ReturnToLastTown() {
    m_zoneIndex = 0; // エンドゲームハブは常にindex0
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
    for (auto& tab : m_savedStash) tab.clear();
    m_savedPassiveTree.clear();
    m_mapAttemptActive = false;
    m_mapDeathsRemaining = 0;
    m_activeMapMods.clear();
    m_pendingAtlasNodeValid = false;
    m_savedAtlasNodes.clear();
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

    for (size_t t = 0; t < m_savedStash.size(); ++t) {
        const auto& tab = m_savedStash[t];
        out << "stash.tab" << t << ".count=" << tab.size() << "\n";
        for (size_t i = 0; i < tab.size(); ++i) {
            std::string prefix = "stash.tab" + std::to_string(t) + ".item" + std::to_string(i) + ".";
            WriteItem(out, prefix, tab[i]);
        }
    }

    out << "passiveTree.count=" << m_savedPassiveTree.size() << "\n";
    for (size_t i = 0; i < m_savedPassiveTree.size(); ++i) {
        out << "passiveTree.node" << i << "=" << m_savedPassiveTree[i] << "\n";
    }

    out << "ownedGems.count=" << m_savedOwnedGems.size() << "\n";
    for (size_t i = 0; i < m_savedOwnedGems.size(); ++i) {
        const OwnedGemInstance& g = m_savedOwnedGems[i];
        std::string p = "ownedGems.gem" + std::to_string(i) + ".";
        out << p << "id=" << g.gemId << "\n";
        out << p << "isSupport=" << (g.isSupport ? 1 : 0) << "\n";
        out << p << "level=" << g.level << "\n";
        out << p << "maxSockets=" << g.maxSockets << "\n";
        for (size_t s = 0; s < g.supportGemIds.size(); ++s) {
            out << p << "support" << s << "=" << g.supportGemIds[s] << "\n";
        }
    }

    out << "pendingUncutGems.count=" << m_savedPendingUncutGems.size() << "\n";
    for (size_t i = 0; i < m_savedPendingUncutGems.size(); ++i) {
        const PendingUncutGem& g = m_savedPendingUncutGems[i];
        std::string p = "pendingUncutGems.gem" + std::to_string(i) + ".";
        out << p << "level=" << g.level << "\n";
        out << p << "kind=" << static_cast<int>(g.kind) << "\n";
    }

    for (size_t i = 0; i < m_savedSkillLoadout.size(); ++i) {
        out << "skillLoadout.slot" << i << "=" << m_savedSkillLoadout[i] << "\n";
    }

    for (size_t i = 0; i < m_savedAuraLoadout.size(); ++i) {
        out << "auraLoadout.slot" << i << "=" << m_savedAuraLoadout[i] << "\n";
        out << "auraLoadout.active" << i << "=" << (m_savedAuraActive[i] ? 1 : 0) << "\n";
    }

    out << "atlas.count=" << m_savedAtlasNodes.size() << "\n";
    for (size_t i = 0; i < m_savedAtlasNodes.size(); ++i) {
        out << "atlas.node" << i << ".col=" << m_savedAtlasNodes[i].first << "\n";
        out << "atlas.node" << i << ".row=" << m_savedAtlasNodes[i].second << "\n";
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

    // Pre-stash saves have no "stash.tab{t}.count" key -- defaults to an empty stash,
    // same fallback convention as the other post-launch additions above. Pre-multi-tab
    // saves (single "stash.count"/"stash.item{i}.*") are treated the same way: they fall
    // back to empty tabs rather than being migrated, since the stash was brand new and
    // very unlikely to hold anything yet at that point.
    for (size_t t = 0; t < m_savedStash.size(); ++t) {
        m_savedStash[t].clear();
        int count = GetI(kv, "stash.tab" + std::to_string(t) + ".count", 0);
        for (int i = 0; i < count; ++i) {
            std::string prefix = "stash.tab" + std::to_string(t) + ".item" + std::to_string(i) + ".";
            m_savedStash[t].push_back(ReadItem(kv, prefix, EquipSlot::Weapon));
        }
    }

    m_savedPassiveTree.clear();
    int passiveTreeCount = GetI(kv, "passiveTree.count", 0);
    for (int i = 0; i < passiveTreeCount; ++i) {
        m_savedPassiveTree.push_back(GetI(kv, "passiveTree.node" + std::to_string(i), 0));
    }

    // Pre-gem-overhaul saves have no "ownedGems.count" key at all (only the old flat
    // "unlockedGems.count"/"unlockedGems.gem{i}" ids); GetI's default (0) makes this
    // naturally resolve to an empty list for those saves, and GameScene falls back to
    // EntitySpawner's starter loadout exactly like it already does for a totally fresh
    // character (see the existing !GetSavedOwnedGems().empty() guard there).
    m_savedOwnedGems.clear();
    int ownedGemCount = GetI(kv, "ownedGems.count", 0);
    for (int i = 0; i < ownedGemCount; ++i) {
        std::string p = "ownedGems.gem" + std::to_string(i) + ".";
        OwnedGemInstance g;
        g.gemId = GetI(kv, p + "id", -1);
        g.isSupport = GetI(kv, p + "isSupport", 0) != 0;
        g.level = GetI(kv, p + "level", 1);
        g.maxSockets = GetI(kv, p + "maxSockets", 2);
        for (size_t s = 0; s < g.supportGemIds.size(); ++s) {
            g.supportGemIds[s] = GetI(kv, p + "support" + std::to_string(s), -1);
        }
        if (g.gemId >= 0) m_savedOwnedGems.push_back(g);
    }

    // Pre-inventory-flow saves have no "pendingUncutGems.count" key -- defaults to an
    // empty list, same fallback convention as m_savedOwnedGems above.
    m_savedPendingUncutGems.clear();
    int pendingUncutCount = GetI(kv, "pendingUncutGems.count", 0);
    for (int i = 0; i < pendingUncutCount; ++i) {
        std::string p = "pendingUncutGems.gem" + std::to_string(i) + ".";
        PendingUncutGem g;
        g.level = GetI(kv, p + "level", 1);
        g.kind = static_cast<GemPickupKind>(GetI(kv, p + "kind", 0));
        m_savedPendingUncutGems.push_back(g);
    }

    for (size_t i = 0; i < m_savedSkillLoadout.size(); ++i) {
        m_savedSkillLoadout[i] = GetI(kv, "skillLoadout.slot" + std::to_string(i), -1);
    }

    for (size_t i = 0; i < m_savedAuraLoadout.size(); ++i) {
        m_savedAuraLoadout[i] = GetI(kv, "auraLoadout.slot" + std::to_string(i), -1);
        m_savedAuraActive[i] = GetI(kv, "auraLoadout.active" + std::to_string(i), 0) != 0;
    }

    // Pre-Atlas saves have no "atlas.count" key -- defaults to no completed nodes beyond
    // the always-cleared start node, same fallback convention as the other post-launch
    // additions above.
    m_savedAtlasNodes.clear();
    int atlasCount = GetI(kv, "atlas.count", 0);
    for (int i = 0; i < atlasCount; ++i) {
        std::string p = "atlas.node" + std::to_string(i) + ".";
        int col = GetI(kv, p + "col", 0);
        int row = GetI(kv, p + "row", 0);
        m_savedAtlasNodes.push_back({ col, row });
    }

    return true;
}
