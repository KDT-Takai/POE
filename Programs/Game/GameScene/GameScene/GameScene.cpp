#include "GameScene.h"
#include <ECS.h>
#include <algorithm>
#include <array>
#include <System/CameraManager/CameraManager.h>
#include <System/Time/Time.h>
#include <System/DebugGui/DebugGui.h>

#include "../../../System/DebugManager/DebugManager.h"
#include "../Entity/EntitySpawner.h"

#include "../MapGenerator/MapGenerator.h"
#include "../Zone/ZoneBuilder.h"
#include "../../ECS/Systems/Progression/AtlasData.h"

#include "System/SceneManager/SceneManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/KeyBindings/KeyBindings.h"
#include "../../ResultScene/ResultScene/ResultScene.h"

GameScene::GameScene() {
    sceneName = "GameScreen";

    registry = std::make_unique<Registry>();

    editorSystem = std::make_shared<EditorSystem>();
    renderSystem = std::make_shared<RenderSystem>();
	
    inputSystem = std::make_shared<InputSystem>();
    physicsSystem = std::make_shared<PhysicsSystem>();
    mapRenderSystem = std::make_shared<MapRenderSystem>();
    minimapSystem = std::make_shared<MinimapSystem>();
    hazardGroundSystem = std::make_shared<HazardGroundSystem>();
    hazardGroundRenderSystem = std::make_shared<HazardGroundRenderSystem>();
	skillSystem = std::make_shared<SkillSystem>();
	uiSystem = std::make_shared<UISystem>();
	sparkVisualSystem = std::make_shared<SparkVisualSystem>();
	projectileSystem = std::make_shared<ProjectileSystem>();
	sparkRenderSystem = std::make_shared<SparkRenderSystem>();
	enemySpawnSystem = std::make_shared<EnemySpawnSystem>();
	enemyAISystem = std::make_shared<EnemyAISystem>();
	enemyRangedAttackSystem = std::make_shared<EnemyRangedAttackSystem>();
	enemyAreaAttackSystem = std::make_shared<EnemyAreaAttackSystem>();
	enemySummonSystem = std::make_shared<EnemySummonSystem>();
	enemyChargeSystem = std::make_shared<EnemyChargeSystem>();
	collisionSystem = std::make_shared<CollisionSystem>();
	healthBarRenderSystem = std::make_shared<HealthBarRenderSystem>();
	statusEffectSystem = std::make_shared<StatusEffectSystem>();
	itemPickupSystem = std::make_shared<ItemPickupSystem>();
	bossPhaseSystem = std::make_shared<BossPhaseSystem>();
	characterSheetSystem = std::make_shared<CharacterSheetSystem>();
	inventorySystem = std::make_shared<InventorySystem>();
	passiveTreeSystem = std::make_shared<PassiveTreeSystem>();
	atlasSystem = std::make_shared<AtlasSystem>();
	vendorSystem = std::make_shared<VendorSystem>();
	waystoneVendorSystem = std::make_shared<WaystoneVendorSystem>();
	stashSystem = std::make_shared<StashSystem>();
	skillGemSystem = std::make_shared<SkillGemSystem>();
	gemIdentifySystem = std::make_shared<GemIdentifySystem>();
	keyBindSystem = std::make_shared<KeyBindSystem>();
	minionSystem = std::make_shared<MinionSystem>();

    auto& campaign = CampaignManager::Instance();
    const ZoneDefinition& zone = campaign.CurrentZone();
    m_zoneKind = zone.kind;

    ZoneBuildResult built = ZoneBuilder::Build(*registry, zone, campaign.GetEndgameMapTier(), campaign.CurrentAct().isEndgame, campaign.GetActiveMapMods(),
        campaign.HasActiveMapAttempt(), campaign.GetMapDeathsRemaining(), campaign.GetMapSeed());
    m_hasPortal = built.hasPortal;
    m_portalPos = built.portalPos;
    m_portalMarkers = built.portalMarkers;
    m_townNpcs = built.townNpcs;

    // このゾーン開始時点のRare/ボスの頭数を数えておく("レア敵とボスを全部倒してから
    // 帰還用ポータルを出す"の判定に使う、GameScene::Update参照)。
    m_notableEnemiesRemaining = 0;
    m_notablePortalSpawned = false;
    if (m_zoneKind == ZoneKind::Combat) {
        for (auto entity : registry->View<CharacterStatsComponent>()) {
            if (registry->HasComponent<PlayerTag>(entity) || registry->HasComponent<AllyTagComponent>(entity)) continue;
            auto& enemyStats = registry->GetComponent<CharacterStatsComponent>(entity);
            if (enemyStats.rarity == MonsterRarity::Rare || registry->HasComponent<BossTag>(entity)) {
                m_notableEnemiesRemaining++;
            }
        }
    }

    // 同一ゾーン内でのシーン再構築(Atlasでマップを開いた直後にタウンを円状ポータル
    // 入りで作り直す等)の直後は、ゾーン固定のスポーン地点ではなく直前の座標へ置く
    // ("町でポータルを出すと位置がリセットされる"というフィードバック対応)。
    sf::Vector2f spawnPos = built.playerSpawn;
    sf::Vector2f overrideSpawnPos;
    if (campaign.ConsumePendingSpawnOverride(overrideSpawnPos)) {
        spawnPos = overrideSpawnPos;
    } else if (m_zoneKind == ZoneKind::Combat) {
        // 同じマップアタempト中に帰還用ポータルを使ったことがあれば、その座標から
        // 再開する(死亡での再入場は対象外、GameScene::ReturnToHubViaPortal参照)。
        sf::Vector2f mapReturnPos;
        if (campaign.GetMapReturnPosition(mapReturnPos)) {
            spawnPos = mapReturnPos;
        }
    }

    auto player = EntitySpawner::CreatePlayer(*registry, spawnPos.x, spawnPos.y);
    if (player) {
        playerEntity = player.GetID();
        if (campaign.HasSavedPlayer()) {
            player.GetComponent<CharacterStatsComponent>() = campaign.GetSavedStats();
            auto& equipment = player.GetComponent<EquipmentComponent>();
            equipment.slots = campaign.GetSavedEquipment().slots;
            equipment.baseStats = campaign.GetSavedEquipment().baseStats;
            EquipmentSystem::RecalculateStats(player.GetComponent<CharacterStatsComponent>(), equipment);
            player.GetComponent<InventoryComponent>().items = campaign.GetSavedInventory();
            player.GetComponent<StashComponent>().tabs = campaign.GetSavedStash();
            player.GetComponent<PassiveTreeComponent>().allocatedNodeIds = campaign.GetSavedPassiveTree();

            auto& atlasComp = player.GetComponent<AtlasComponent>();
            atlasComp.completedNodeKeys.clear();
            for (const auto& [col, row] : campaign.GetSavedAtlasNodes()) {
                atlasComp.completedNodeKeys.push_back(AtlasData::PackKey(col, row));
            }

            // Skill/Spirit gems are equipped items now (see CampaignManager.h) -- Uncut
            // Gems and any identified-but-unequipped gems already came back for free above
            // as part of InventoryComponent/StashComponent, no special handling needed.
            // Older saves (pre-gem-item-overhaul) have no skillLoadout data at all; keep
            // the EntitySpawner-seeded starter loadout in that case instead of blanking it out.
            bool hasSavedSkillLoadout = false;
            for (const auto& slot : campaign.GetSavedSkillLoadout()) {
                if (slot) { hasSavedSkillLoadout = true; break; }
            }
            if (hasSavedSkillLoadout) {
                auto& skillComp = player.GetComponent<PlayerSkill>();
                const auto& loadout = campaign.GetSavedSkillLoadout();
                for (size_t i = 0; i < loadout.size(); ++i) {
                    skillComp.equippedItems[i] = loadout[i];
                    if (loadout[i]) {
                        // Rebuilds level scaling + socketed supports from the item itself,
                        // not just a raw catalog copy -- matches SkillGemSystem::AssignGem
                        // so a saved character's skills come back exactly as they were.
                        SkillGemScaling::BuildEquippedSkillData(skillComp.skills[i], *loadout[i]);
                    } else {
                        skillComp.skills[i] = SkillData{};
                    }
                }
            }

            // The aura's stat bonus already lives in equipment.baseStats (restored above),
            // so this only restores which item each Spirit slot displays as equipped and
            // whether it was toggled ON (for the UI's ON/OFF button state -- the actual
            // reserved amount is CharacterStatsComponent::currentSpirit, already restored
            // as part of the stats blob above).
            auto& spiritLoadout = player.GetComponent<SpiritGemLoadoutComponent>();
            spiritLoadout.items = campaign.GetSavedAuraLoadout();
            spiritLoadout.active = campaign.GetSavedAuraActive();
        }

        // A Waystone's "Players have reduced Elemental Resistances" mod (see Item.h /
        // ItemFactory::WaystoneModPool) only applies while physically inside that map --
        // applied directly to the live stats (not baseStats) since this whole registry/
        // GameScene gets torn down and rebuilt fresh from baseStats on the next zone
        // load, so there's nothing to explicitly revert.
        if (zone.kind == ZoneKind::Combat) {
            auto& liveStats = player.GetComponent<CharacterStatsComponent>();
            for (const auto& mod : campaign.GetActiveMapMods()) {
                if (mod.stat == WaystoneModStat::PlayerReducedElementalResistance) {
                    liveStats.fireRes += mod.value / 100.0f;
                    liveStats.iceRes += mod.value / 100.0f;
                    liveStats.lightningRes += mod.value / 100.0f;
                }
            }
        }

        spdlog::info("Player created with ID: {} in zone '{}'", player.GetID(), zone.displayName);
    }

    if (registry->HasComponent<CharacterStatsComponent>(playerEntity)) {
        m_playerHpLastFrame = registry->GetComponent<CharacterStatsComponent>(playerEntity).currentHP;
    }
}

// AdvanceToNextZone/ReturnToHubViaPortalの両方が使う保存処理(以前はAdvanceToNextZoneに
// 直接書かれていた重複コード)。
void GameScene::SavePlayerStateToCampaign() {
    auto& campaign = CampaignManager::Instance();
    if (registry->HasComponent<CharacterStatsComponent>(playerEntity)) {
        campaign.SavePlayerStats(registry->GetComponent<CharacterStatsComponent>(playerEntity));
    }
    if (registry->HasComponent<EquipmentComponent>(playerEntity)) {
        campaign.SaveEquipment(registry->GetComponent<EquipmentComponent>(playerEntity));
    }
    if (registry->HasComponent<InventoryComponent>(playerEntity)) {
        campaign.SaveInventory(registry->GetComponent<InventoryComponent>(playerEntity).items);
    }
    if (registry->HasComponent<StashComponent>(playerEntity)) {
        campaign.SaveStash(registry->GetComponent<StashComponent>(playerEntity).tabs);
    }
    if (registry->HasComponent<PassiveTreeComponent>(playerEntity)) {
        campaign.SavePassiveTree(registry->GetComponent<PassiveTreeComponent>(playerEntity).allocatedNodeIds);
    }
    if (registry->HasComponent<AtlasComponent>(playerEntity)) {
        std::vector<std::pair<int, int>> nodes;
        for (int64_t key : registry->GetComponent<AtlasComponent>(playerEntity).completedNodeKeys) {
            int col, row;
            AtlasData::UnpackKey(key, col, row);
            nodes.push_back({ col, row });
        }
        campaign.SaveAtlasNodes(nodes);
    }
    // Skill/Spirit gems are equipped items now (see CampaignManager.h) -- Uncut Gems and
    // any identified-but-unequipped gems already save for free above as part of
    // InventoryComponent/StashComponent, no separate owned-gems list to save.
    if (registry->HasComponent<PlayerSkill>(playerEntity)) {
        campaign.SaveSkillLoadout(registry->GetComponent<PlayerSkill>(playerEntity).equippedItems);
    }
    if (registry->HasComponent<SpiritGemLoadoutComponent>(playerEntity)) {
        auto& spiritLoadout = registry->GetComponent<SpiritGemLoadoutComponent>(playerEntity);
        campaign.SaveAuraLoadout(spiritLoadout.items);
        campaign.SaveAuraActive(spiritLoadout.active);
    }
}

void GameScene::AdvanceToNextZone() {
    SavePlayerStateToCampaign();
    auto& campaign = CampaignManager::Instance();
    campaign.CompleteCurrentZoneAndAdvance();
    campaign.SaveToDisk();
    SceneManager::Instance().ChangeScene("GameScene");
}

// 「マップ上ではスキルの横に帰還用ポータルを出現させる」「レア敵・ボスを倒すとポータルが
// 出現」の両方から呼ばれる自発的な帰還。AdvanceToNextZoneと違いCompleteCurrentZoneAndAdvance
// (Atlasノード完了判定+マップアタempt終了)を経由せず、CampaignManager::ReturnToLastTown()
// のみを呼ぶため、進行中のマップアタempt(残りポータル数)はそのまま維持される(町側の
// 周回ポータルからいつでも無料で再入場できる、死亡でもクリアでもない「ただの一時帰宅」)。
void GameScene::ReturnToHubViaPortal() {
    SavePlayerStateToCampaign();
    auto& campaign = CampaignManager::Instance();

    // 生存中に離脱する個体(MapSlotComponent持ち)のHPを覚えておく。次に同じマップ
    // アタempト内で再入場した時、死んでいない個体は同じ座標・同じ状態でそのまま
    // 続きから戦える("雑魚敵も復活しないようにして…同じ個体で"という指示対応)。
    for (auto entity : registry->View<MapSlotComponent, CharacterStatsComponent>()) {
        auto& slot = registry->GetComponent<MapSlotComponent>(entity);
        auto& stats = registry->GetComponent<CharacterStatsComponent>(entity);
        if (stats.currentHP > 0.0f) {
            campaign.SetEnemySlotHp(slot.slotIndex, stats.currentHP);
        }
    }

    // ミニマップ/Tab全体マップの探索済みタイルも覚えておく("ミニマップがリセット
    // されてる"というバグ報告対応)。
    {
        auto mapView = registry->View<MapComponent>();
        if (!mapView.empty()) {
            campaign.SetMapVisitedTiles(registry->GetComponent<MapComponent>(mapView[0]).visited);
        }
    }

    // 次に同じマップアタempト内で再入場した時、ここ(帰還用ポータルを使った座標)から
    // 再開できるように覚えておく("ポータルで町に戻った際に再度ポータルに入ると先ほど
    // 帰還用ポータルで出た位置からにする"という指示対応)。
    if (registry->HasComponent<TransformComponent>(playerEntity)) {
        campaign.SetMapReturnPosition(registry->GetComponent<TransformComponent>(playerEntity).position);
    }
    campaign.ReturnToLastTown();
    campaign.SaveToDisk();
    SceneManager::Instance().ChangeScene("GameScene");
}

// プレイヤーの現在地に帰還用ポータルを1つ出す。スキル欄隣のボタンでの詠唱完了、
// および全Rare/ボス討伐(GameScene::Update)の両方から呼ばれる共通処理。
void GameScene::SpawnMapReturnPortalAtPlayer() {
    if (!registry->HasComponent<TransformComponent>(playerEntity)) return;
    auto& pTrans = registry->GetComponent<TransformComponent>(playerEntity);
    sf::Vector2f playerCenter = pTrans.position + sf::Vector2f(16.0f, 16.0f);
    constexpr float kReturnPortalRadius = 22.0f;
    auto portal = registry->CreateEntityObject();
    portal.AddComponent(TransformComponent{ playerCenter - sf::Vector2f(kReturnPortalRadius, kReturnPortalRadius), {1.f, 1.f}, 0.f });
    portal.AddComponent(CircleComponent{ kReturnPortalRadius, sf::Color(255, 210, 60), true });
    portal.AddComponent(TagComponent{ "Return Portal" });
    portal.AddComponent(MapReturnPortalTag{});
}

// "T3x2 T1x1" style summary of held Waystones (now a normal inventory item, see Item.h)
// for the hub's HUD prompt.
std::string GameScene::HeldWaystoneSummary() const {
    if (!registry->HasComponent<InventoryComponent>(playerEntity)) return "none";
    const auto& inventory = registry->GetComponent<InventoryComponent>(playerEntity);

    std::array<int, kMaxWaystoneTier> counts{};
    for (const auto& item : inventory.items) {
        if (item.category != ItemCategory::Waystone) continue;
        if (item.waystoneTier < 1 || item.waystoneTier > kMaxWaystoneTier) continue;
        counts[item.waystoneTier - 1]++;
    }

    std::string summary;
    for (int t = kMaxWaystoneTier; t >= 1; --t) {
        if (counts[t - 1] <= 0) continue;
        if (!summary.empty()) summary += " ";
        summary += "T" + std::to_string(t) + "x" + std::to_string(counts[t - 1]);
    }
    return summary.empty() ? "none" : summary;
}

sf::Color GameScene::NpcRingColor(TownNpcKind kind) {
    switch (kind) {
    case TownNpcKind::ItemVendor: return sf::Color(80, 200, 255, 140);
    case TownNpcKind::WaystoneVendor: return sf::Color(200, 150, 255, 140);
    case TownNpcKind::Stash: return sf::Color(150, 110, 60, 140);
    default: return sf::Color(200, 200, 200, 140);
    }
}

std::string GameScene::NpcHudHint(TownNpcKind kind) {
    switch (kind) {
    case TownNpcKind::ItemVendor: return "クリックして商人と取引する";
    case TownNpcKind::WaystoneVendor: return "クリックしてウェイストーンを購入する";
    case TownNpcKind::Stash: return "クリックして保管庫を開く";
    default: return "";
    }
}

// Endgame hub's map device. If a map attempt is still open (<=6 deaths used, see
// CampaignManager::HasActiveMapAttempt), re-enters that same tier/mods for free -- one
// of the up-to-6 portals real PoE2 grants per Waystone, letting the player try again
// after dying without needing another Waystone. Otherwise opens the Atlas screen
// (AtlasSystem) so the player picks which map node to run; the actual Waystone
// consumption + CampaignManager::OpenEndgameMap call happens there once a node is chosen
// (see AtlasSystem::TryOpenSelected), and GameScene::Update picks up the resulting
// ConsumeMapStartRequest() to transition in exactly like this function used to do directly.
void GameScene::TryOpenEndgameMapFromHub() {
    auto& campaign = CampaignManager::Instance();

    if (campaign.HasActiveMapAttempt()) {
        AdvanceToNextZone();
        return;
    }

    atlasSystem->Open();
}

void GameScene::Update() {
    auto dt = Time::Instance().GetDeltaTime();
    CameraManager::Instance().UpdateShake(dt);

    auto& binds = KeyBindings::Instance();
    bool awaitingRebind = keyBindSystem->IsAwaitingKey();

    // 画面は左(キャラクターシート/スキルジェム)・右(インベントリ)の2枠構成。
    // 左枠はキャラクターシートとスキルジェムが排他(最後に開いた方だけ表示、
    // タブ切り替えのように振る舞う)、右枠のインベントリは左枠と独立に同時に
    // 開ける。これらは全て「非ポーズ」系。「ポーズ」系(パッシブツリー/ベンダー/
    // キーバインド)を開くときだけ全パネルを閉じる(ポーズ系同士も排他)。
    // 自分自身は閉じ対象から除く。
    auto closeFreePanels = [&]() { characterSheetSystem->isOpen = false; inventorySystem->Close(); skillGemSystem->Close(); };

    // Escape closes whatever menu(s) happen to be open (fixed key, not rebindable --
    // same convention as Up/Down/Enter/Backspace navigation keys, see AI/DECISIONS.md).
    // Guarded by !awaitingRebind so it doesn't fight KeyBindSystem's own use of Escape
    // to cancel a pending "press a key to rebind" wait without closing the whole panel.
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::Escape)) {
        closeFreePanels();
        passiveTreeSystem->Close();
        atlasSystem->Close();
        vendorSystem->Close();
        waystoneVendorSystem->Close();
        stashSystem->Close();
        keyBindSystem->Close();
        gemIdentifySystem->Close();
        minimapSystem->CloseFullMap();
    }

    // Tab: PoE2風の全画面マップ(MinimapSystem)。他の全画面/モーダル系メニューが
    // 開いている間は競合を避けるため開かない(Escapeでの一括クローズには含める、上記)。
    bool anyOtherPauseMenuOpen = passiveTreeSystem->isOpen || atlasSystem->isOpen || vendorSystem->isOpen ||
        waystoneVendorSystem->isOpen || stashSystem->isOpen || keyBindSystem->isOpen || gemIdentifySystem->isOpen;
    if (!awaitingRebind && !anyOtherPauseMenuOpen && InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::Tab)) {
        minimapSystem->ToggleFullMap();
    }

    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleCharacterSheet))) {
        characterSheetSystem->Toggle();
        if (characterSheetSystem->isOpen) { skillGemSystem->Close(); passiveTreeSystem->Close(); atlasSystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleInventory))) {
        inventorySystem->Toggle();
        if (inventorySystem->isOpen) { passiveTreeSystem->Close(); atlasSystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::TogglePassiveTree))) {
        passiveTreeSystem->Toggle();
        if (passiveTreeSystem->isOpen) { closeFreePanels(); atlasSystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleSkillGems))) {
        skillGemSystem->Toggle();
        if (skillGemSystem->isOpen) { characterSheetSystem->isOpen = false; passiveTreeSystem->Close(); atlasSystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    // PoE2同様、NPCへの話しかけは左クリックのみ(Bキーの「話しかける」操作は廃止)。
    // NPC本体にカーソルが乗っているかは常時判定し、Render側で当たり判定の輪を
    // 表示することでクリック可能な範囲を視覚的に分かるようにする。
    m_hoveringNpc = false;
    m_clickedOnNpc = false;
    bool anyNpcPanelOpen = vendorSystem->isOpen || waystoneVendorSystem->isOpen || stashSystem->isOpen;
    if (m_nearNpcIndex >= 0 && !anyNpcPanelOpen) {
        sf::Vector2f mouseWorldForNpc = InputManager::Instance().GetMouseWorldPosition();
        sf::Vector2f npcPos = m_townNpcs[m_nearNpcIndex].pos;
        float vdx = mouseWorldForNpc.x - npcPos.x;
        float vdy = mouseWorldForNpc.y - npcPos.y;
        m_hoveringNpc = (vdx * vdx + vdy * vdy) < (kVendorClickRadius * kVendorClickRadius);
        m_clickedOnNpc = m_hoveringNpc && InputManager::Instance().GetMouseInput().IsGetMouse(sf::Mouse::Button::Left);
    }

    // マップデバイス(ポータル)もNPCと同じく左クリックで操作する(Enterキーでの
    // 起動は廃止、「エンターで開くのではなく左クリックで開く」というフィードバック対応)。
    // 進行中のマップアタempトがあるとm_portalMarkersが複数(円状の再入場口)になるため、
    // どれか1つにカーソルが乗っているかを走査する(どれをクリックしても同じ処理)。
    m_hoveringPortal = false;
    m_clickedOnPortal = false;
    m_hoveredPortalMarker = -1;
    bool anyMenuBlockingPortal = inventorySystem->isOpen || passiveTreeSystem->isOpen || atlasSystem->isOpen ||
        vendorSystem->isOpen || skillGemSystem->isOpen || keyBindSystem->isOpen || anyNpcPanelOpen || minimapSystem->IsFullMapOpen();
    if (m_hasPortal && m_playerNearPortal && !anyMenuBlockingPortal) {
        sf::Vector2f mouseWorldForPortal = InputManager::Instance().GetMouseWorldPosition();
        for (size_t i = 0; i < m_portalMarkers.size(); ++i) {
            float pdx = mouseWorldForPortal.x - m_portalMarkers[i].x;
            float pdy = mouseWorldForPortal.y - m_portalMarkers[i].y;
            if ((pdx * pdx + pdy * pdy) < (kVendorClickRadius * kVendorClickRadius)) {
                m_hoveringPortal = true;
                m_hoveredPortalMarker = static_cast<int>(i);
                break;
            }
        }
        m_clickedOnPortal = m_hoveringPortal && InputManager::Instance().GetMouseInput().IsGetMouse(sf::Mouse::Button::Left);
    }

    // マップ内の帰還用ポータル(詠唱生成/レア・ボス討伐で出現)。位置が動的なので毎フレーム
    // 最寄りを探す(町のNPC/マップデバイスと同じ「近づくと輪、クリックで発動」パターン)。
    m_nearReturnPortal = ItemPickupSystem::kInvalidEntity;
    m_hoveringReturnPortal = false;
    m_clickedOnReturnPortal = false;
    sf::Vector2f nearReturnPortalCenter;
    if (m_zoneKind == ZoneKind::Combat && registry->IsValid(playerEntity) && registry->HasComponent<TransformComponent>(playerEntity)) {
        auto& pTransForPortal = registry->GetComponent<TransformComponent>(playerEntity);
        sf::Vector2f playerCenterForPortal = pTransForPortal.position + sf::Vector2f(16.0f, 16.0f);
        float bestDistSq = 150.0f * 150.0f;
        for (auto portalEnt : registry->View<MapReturnPortalTag, TransformComponent, CircleComponent>()) {
            auto& pt = registry->GetComponent<TransformComponent>(portalEnt);
            auto& pc = registry->GetComponent<CircleComponent>(portalEnt);
            sf::Vector2f center = pt.position + sf::Vector2f(pc.radius, pc.radius);
            float dx = playerCenterForPortal.x - center.x, dy = playerCenterForPortal.y - center.y;
            float distSq = dx * dx + dy * dy;
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                m_nearReturnPortal = portalEnt;
                nearReturnPortalCenter = center;
            }
        }
    }
    if (registry->IsValid(m_nearReturnPortal) && !anyMenuBlockingPortal) {
        sf::Vector2f mouseWorldForReturn = InputManager::Instance().GetMouseWorldPosition();
        float rdx = mouseWorldForReturn.x - nearReturnPortalCenter.x;
        float rdy = mouseWorldForReturn.y - nearReturnPortalCenter.y;
        m_hoveringReturnPortal = (rdx * rdx + rdy * rdy) < (kVendorClickRadius * kVendorClickRadius);
        m_clickedOnReturnPortal = m_hoveringReturnPortal && InputManager::Instance().GetMouseInput().IsGetMouse(sf::Mouse::Button::Left);
    }
    if (m_clickedOnNpc) {
        switch (m_townNpcs[m_nearNpcIndex].kind) {
        case TownNpcKind::ItemVendor: {
            int playerLevel = registry->HasComponent<CharacterStatsComponent>(playerEntity)
                ? registry->GetComponent<CharacterStatsComponent>(playerEntity).level : 1;
            vendorSystem->Toggle(playerLevel);
            if (vendorSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); atlasSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); }
            break;
        }
        case TownNpcKind::WaystoneVendor:
            waystoneVendorSystem->Toggle();
            if (waystoneVendorSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); atlasSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); vendorSystem->Close(); stashSystem->Close(); }
            break;
        case TownNpcKind::Stash:
            stashSystem->Toggle();
            if (stashSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); atlasSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); }
            break;
        }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::O)) {
        keyBindSystem->Toggle();
        if (keyBindSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); atlasSystem->Close(); vendorSystem->Close(); gemIdentifySystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); }
    }
    keyBindSystem->Update(*registry, dt);
    inventorySystem->Update(*registry, dt, *gemIdentifySystem);
    passiveTreeSystem->Update(*registry, dt);
    atlasSystem->Update(*registry, dt);
    if (atlasSystem->ConsumeMapStartRequest()) {
        // Waystoneを消費してアタemptを開始しただけで、まだマップへは入らない
        // ("ポータルに触る前にマップに行く"というフィードバック対応)。
        // CampaignManager::OpenEndgameMapはzoneIndexを変えないため、ここではまだ
        // タウン(index 0)のまま -- シーンを再構築するだけでHasActiveMapAttempt()が
        // trueになった状態のZoneBuilder::Buildが呼ばれ、デバイス周りに円状の入場
        // ポータルが出現する。実際にマップへ入るのはそのどれかをクリックした時
        // (m_clickedOnPortal、TryOpenEndgameMapFromHubと同じ経路)。
        SavePlayerStateToCampaign();
        if (registry->HasComponent<TransformComponent>(playerEntity)) {
            CampaignManager::Instance().SetPendingSpawnOverride(registry->GetComponent<TransformComponent>(playerEntity).position);
        }
        CampaignManager::Instance().SaveToDisk();
        SceneManager::Instance().ChangeScene("GameScene");
        return;
    }
    vendorSystem->Update(*registry, dt);
    waystoneVendorSystem->Update(*registry, dt);
    stashSystem->Update(*registry, dt);
    skillGemSystem->Update(*registry, dt);
    gemIdentifySystem->Update(*registry, dt);

    // パッシブツリー等をゆっくり操作できるよう、それらのメニューが開いている間は
    // ワールドシミュレーションを止める。ただしインベントリ(アイテム)/キャラクター
    // シート(ステータス)/スキルジェムはPoE2同様、開いたまま戦闘・移動を続けられる
    // ようにする(スキルジェム画面を開くとゲームが固まる不具合の修正)。未鑑定ジェムの
    // 選択画面(GemIdentifySystem)も選んでいる間は戦闘が進まないようポーズ系に含める。
    bool isPaused = passiveTreeSystem->isOpen || atlasSystem->isOpen || vendorSystem->isOpen || waystoneVendorSystem->isOpen || stashSystem->isOpen || keyBindSystem->isOpen || gemIdentifySystem->isOpen || minimapSystem->IsFullMapOpen();
    // 非ポーズ系メニュー(インベントリ/キャラクターシート/スキルジェム)はゲームを
    // 止めないので、開いている間もフィールド上でのスキル発動クリックは通したい。
    // マウスが実際にそのパネルの上に重なっている時だけクリックをUI側へ譲る
    // (単に「開いているかどうか」ではなく、カーソル位置で判定する)。
    sf::RenderWindow* gameWindow = InputManager::Instance().GetWindow();
    sf::Vector2u winSizeForUI = gameWindow ? gameWindow->getSize() : sf::Vector2u{ 1280, 720 };
    sf::Vector2f mouseScreenPos = InputManager::Instance().GetMouseInput().GetMousePointF();
    bool mouseOverFreeMenu =
        inventorySystem->IsPointInPanel(mouseScreenPos, winSizeForUI) ||
        characterSheetSystem->IsPointInPanel(mouseScreenPos) ||
        skillGemSystem->IsPointInPanel(mouseScreenPos);
    bool uiOwnsClicks = isPaused || mouseOverFreeMenu;

    // 地面のアイテムはクリックで拾う(徘徊での自動拾得は廃止)。カーソルが
    // アイテムの上にある間はSkill1の左クリック割当を抑制し、誤爆を防ぐ。
    Entity hoveredPickup = ItemPickupSystem::kInvalidEntity;
    Entity clickedPickup = ItemPickupSystem::kInvalidEntity;
    if (!uiOwnsClicks) {
        sf::Vector2f mouseWorldPosForPickup = InputManager::Instance().GetMouseWorldPosition();
        hoveredPickup = ItemPickupSystem::FindNearestPickup(*registry, mouseWorldPosForPickup, ItemPickupSystem::kClickRadius);
        if (registry->IsValid(hoveredPickup) && InputManager::Instance().GetMouseInput().IsGetMouse(sf::Mouse::Button::Left)) {
            clickedPickup = hoveredPickup;
        }
    }

    // マップ内(Combat)で、スキルスロット隣のボタンをクリックすると帰還用ポータルの
    // 詠唱を開始する("スキルの横に帰還用ポータルを出現させるものを用意。クリックで
    // 実行"という指示)。詠唱時間が経過するとGameScene::UpdateのSpawnMapReturnPortal
    // 相当処理(後述)で実際にポータルが出現する。既に詠唱中なら再クリックは無視。
    bool clickedPortalButton = false;
    if (m_zoneKind == ZoneKind::Combat && !uiOwnsClicks && m_portalChannelRemaining <= 0.0f) {
        sf::FloatRect portalBtnRect = UISystem::PortalButtonRect(winSizeForUI);
        if (portalBtnRect.contains(mouseScreenPos) && InputManager::Instance().GetMouseInput().IsGetMouse(sf::Mouse::Button::Left)) {
            clickedPortalButton = true;
        }
    }
    if (clickedPortalButton) {
        m_portalChannelRemaining = kPortalChannelDuration;
    }

    if (!isPaused) {
        // ����
        inputSystem->Update(*registry, dt, uiOwnsClicks || registry->IsValid(hoveredPickup) || m_clickedOnNpc || m_clickedOnPortal || m_clickedOnReturnPortal || clickedPortalButton);
        // ������
        skillSystem->Update(*registry, dt);
        // �X�p�[�N
        sparkVisualSystem->Update(*registry);
        projectileSystem->Update(*registry, dt);
    }
    // �����蔻��
    if (registry->IsValid(playerEntity)) {
        auto& input = registry->GetComponent<PlayerInputComponent>(playerEntity);
        input.mouseWorldPos = InputManager::Instance().GetMouseWorldPosition();

        auto& trans = registry->GetComponent<TransformComponent>(playerEntity);
        sf::Vector2f centerPos = trans.position + sf::Vector2f(16.0f, 32.0f);
        CameraManager::Instance().SetCenter(centerPos);

        minimapSystem->RevealAndPan(*registry, playerEntity, dt);

        // Safe to run every frame regardless of what changed maxSpirit/baseStats since the
        // last call (equip swap, level up, passive spend) -- see SpiritAuraSystem::
        // ReevaluateReservations. Not gated by isPaused since it's pure data consistency,
        // not world simulation.
        if (registry->HasComponent<SpiritGemLoadoutComponent>(playerEntity)
            && registry->HasComponent<EquipmentComponent>(playerEntity) && registry->HasComponent<CharacterStatsComponent>(playerEntity)) {
            SpiritAuraSystem::ReevaluateReservations(*registry, playerEntity,
                registry->GetComponent<SpiritGemLoadoutComponent>(playerEntity),
                registry->GetComponent<EquipmentComponent>(playerEntity),
                registry->GetComponent<CharacterStatsComponent>(playerEntity));
        }
    }
    sf::Vector2f playerPos(0, 0);
    if (registry->HasComponent<TransformComponent>(playerEntity)) {
        playerPos = registry->GetComponent<TransformComponent>(playerEntity).position;
    }
    if (!isPaused) {
        // �G
    //    enemySpawnSystem->Update(*registry, dt, playerPos);
        enemyAISystem->Update(*registry, dt, playerPos);
        enemyRangedAttackSystem->Update(*registry, dt, playerPos);
        enemyAreaAttackSystem->Update(*registry, dt, playerPos);
        enemySummonSystem->Update(*registry, dt, playerPos);
        enemyChargeSystem->Update(*registry, dt, playerPos);
        minionSystem->Update(*registry, dt);
        hazardGroundSystem->Update(*registry, dt);
        statusEffectSystem->Update(*registry, dt);
        // �������Z
        physicsSystem->Update(*registry, dt);
        // �Փˏ���
        collisionSystem->Update(*registry, dt);
        itemPickupSystem->Update(*registry, dt, clickedPickup);
        bossPhaseSystem->Update(*registry, dt);
    }

    if (m_portalMessageTimer > 0.0f) m_portalMessageTimer -= dt;

    // 帰還用ポータルの詠唱進行。ダメージを受けると(直前フレームよりHPが減っていたら)
    // キャンセルする("出現させるには少し時間がかかる。その間攻撃されるとキャンセル
    // される"という指示)。環境ダメージ(床属性ギミック等)も含め、HPが減る要因なら
    // 種類を問わずキャンセル対象にする。
    if (registry->HasComponent<CharacterStatsComponent>(playerEntity)) {
        float currentHP = registry->GetComponent<CharacterStatsComponent>(playerEntity).currentHP;
        if (m_portalChannelRemaining > 0.0f) {
            if (currentHP < m_playerHpLastFrame) {
                m_portalChannelRemaining = 0.0f;
                m_portalMessage = "ポータルの詠唱が中断されました！";
                m_portalMessageTimer = 2.0f;
            } else {
                m_portalChannelRemaining -= dt;
                if (m_portalChannelRemaining <= 0.0f) {
                    m_portalChannelRemaining = 0.0f;
                    SpawnMapReturnPortalAtPlayer();
                    m_portalMessage = "帰還用ポータルを生成しました！";
                    m_portalMessageTimer = 2.0f;
                }
            }
        }
        m_playerHpLastFrame = currentHP;
    }

    if (m_clickedOnReturnPortal) {
        ReturnToHubViaPortal();
        return;
    }

    // �Q�[���̏I���m�F
    if (registry->HasComponent<CharacterStatsComponent>(playerEntity)) {
        auto& state = registry->GetComponent<CharacterStatsComponent>(playerEntity);
        if (state.currentHP <= 0) {
            // Dying inside a map consumes one of its portals (up to CampaignManager::
            // MaxMapDeathsForTier, fewer at higher Waystone tiers) instead of just
            // ending the run outright -- softcore's existing ResultScene "Continue"
            // already sends the player back to the hub either way, this only tracks how
            // many more times that's still free before the map attempt closes for good.
            if (m_zoneKind == ZoneKind::Combat) {
                CampaignManager::Instance().ConsumeMapDeath();
            }
			spdlog::info("Player has died. Ending game.");
			SceneManager::Instance().ChangeScene("ResultScene");
            return;
        }
    }
    if (m_zoneKind == ZoneKind::Town) {
        m_playerNearPortal = false;
        m_nearNpcIndex = -1;
        if (registry->HasComponent<TransformComponent>(playerEntity)) {
            auto& trans = registry->GetComponent<TransformComponent>(playerEntity);

            for (size_t i = 0; i < m_townNpcs.size(); ++i) {
                float vdx = trans.position.x - m_townNpcs[i].pos.x;
                float vdy = trans.position.y - m_townNpcs[i].pos.y;
                if ((vdx * vdx + vdy * vdy) < (100.0f * 100.0f)) {
                    m_nearNpcIndex = static_cast<int>(i);
                    break;
                }
            }

            if (m_hasPortal) {
                float dx = trans.position.x - m_portalPos.x;
                float dy = trans.position.y - m_portalPos.y;
                float distSq = dx * dx + dy * dy;
                m_playerNearPortal = distSq < (150.0f * 150.0f);

                if (m_clickedOnPortal) {
                    TryOpenEndgameMapFromHub();
                    return;
                }
            }
        }
    } else {
        auto enemyView = registry->View<CharacterStatsComponent>();
        bool anyNotableAlive = false;

        for (auto entity : enemyView) {
            if (registry->HasComponent<PlayerTag>(entity) || registry->HasComponent<AllyTagComponent>(entity)) continue;
            auto& enemyStats = registry->GetComponent<CharacterStatsComponent>(entity);
            if (enemyStats.rarity == MonsterRarity::Rare || registry->HasComponent<BossTag>(entity)) {
                anyNotableAlive = true;
            }
        }

        // マップの完了条件はボス+Rareを全滅させること(このゾーンは常にisBossZone、
        // ZoneDefinitionのMakeBoss参照)。以前はここで全滅(名前の無いトラッシュ雑魚も
        // 含む)判定と同時にAdvanceToNextZoneで即座に町へ強制送還していたため、
        // トラッシュを先に倒し切ってからボスを倒すと、帰還用ポータルが出現する間も
        // 無くそのまま町へ飛ばされ、ボスのドロップ品を拾えなかった
        // ("雑魚敵、レア敵を倒してからボスを倒すとポータルの出現じゃなく直接町に
        // 送られる…ボスのドロップ品回収できない"というバグ報告対応)。Atlasノードの
        // 完了判定もこのタイミングへ移し、以後は帰還用ポータルをクリックするまで
        // 転送しない(自分のタイミングでドロップ品を拾ってから離脱できる)。
        if (m_notableEnemiesRemaining > 0 && !m_notablePortalSpawned && !anyNotableAlive) {
            m_notablePortalSpawned = true;
            spdlog::info("Notable enemies cleared!");

            auto& campaign = CampaignManager::Instance();
            int pendingCol, pendingRow;
            if (campaign.GetPendingAtlasNode(pendingCol, pendingRow) && registry->HasComponent<AtlasComponent>(playerEntity)) {
                AtlasData::MarkCompleted(registry->GetComponent<AtlasComponent>(playerEntity), pendingCol, pendingRow);
                campaign.ClearPendingAtlasNode();
            }

            SpawnMapReturnPortalAtPlayer();
            m_portalMessage = "主要な敵を全て倒しました。帰還用ポータルが出現しました！";
            m_portalMessageTimer = 2.5f;
        }
    }
}

void GameScene::Render(sf::RenderTarget& target) {
    target.setView(CameraManager::Instance().GetCurrentView());
	// �}�b�v�`��
	mapRenderSystem->Render(*registry, target);
	hazardGroundRenderSystem->Render(*registry, target);
    renderSystem->Render(*registry, target);
    if (DebugManager::Instance().IsDebugMode()) {
        renderSystem->RenderDebug(*registry, target);
    }
    sparkRenderSystem->Render(*registry, target);
	healthBarRenderSystem->Render(*registry, target);

    // NPCがクリック可能な範囲を視覚的に示す輪(近づいている間のみ表示、
    // カーソルが範囲内なら明るく強調してクリックできることが分かるようにする)。
    bool anyNpcPanelOpenForRing = vendorSystem->isOpen || waystoneVendorSystem->isOpen || stashSystem->isOpen;
    if (m_nearNpcIndex >= 0 && !anyNpcPanelOpenForRing) {
        sf::CircleShape ring(kVendorClickRadius);
        ring.setOrigin({ kVendorClickRadius, kVendorClickRadius });
        ring.setPosition(m_townNpcs[m_nearNpcIndex].pos);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.0f);
        sf::Color baseColor = NpcRingColor(m_townNpcs[m_nearNpcIndex].kind);
        ring.setOutlineColor(m_hoveringNpc ? sf::Color(255, 255, 120, 220) : baseColor);
        target.draw(ring);
    }

    // マップデバイス(ポータル)もNPCと同じクリック範囲リングを表示する。進行中のマップ
    // アタempトがあれば円状に並んだ全マーカーそれぞれにリングを描く。
    if (m_hasPortal && m_playerNearPortal) {
        for (size_t i = 0; i < m_portalMarkers.size(); ++i) {
            sf::CircleShape portalRing(kVendorClickRadius);
            portalRing.setOrigin({ kVendorClickRadius, kVendorClickRadius });
            portalRing.setPosition(m_portalMarkers[i]);
            portalRing.setFillColor(sf::Color::Transparent);
            portalRing.setOutlineThickness(2.0f);
            bool hovered = m_hoveringPortal && static_cast<int>(i) == m_hoveredPortalMarker;
            portalRing.setOutlineColor(hovered ? sf::Color(255, 255, 120, 220) : sf::Color(120, 220, 255, 140));
            target.draw(portalRing);
        }
    }

    // マップ内の帰還用ポータル(詠唱生成/レア・ボス討伐)も同じクリック範囲リング。
    if (registry->IsValid(m_nearReturnPortal) && registry->HasComponent<TransformComponent>(m_nearReturnPortal) && registry->HasComponent<CircleComponent>(m_nearReturnPortal)) {
        auto& rpTrans = registry->GetComponent<TransformComponent>(m_nearReturnPortal);
        auto& rpCircle = registry->GetComponent<CircleComponent>(m_nearReturnPortal);
        sf::Vector2f rpCenter = rpTrans.position + sf::Vector2f(rpCircle.radius, rpCircle.radius);
        sf::CircleShape returnRing(kVendorClickRadius);
        returnRing.setOrigin({ kVendorClickRadius, kVendorClickRadius });
        returnRing.setPosition(rpCenter);
        returnRing.setFillColor(sf::Color::Transparent);
        returnRing.setOutlineThickness(2.0f);
        returnRing.setOutlineColor(m_hoveringReturnPortal ? sf::Color(255, 255, 120, 220) : sf::Color(120, 220, 255, 140));
        target.draw(returnRing);
    }

    target.setView(target.getDefaultView());
	uiSystem->Render(*registry, target, m_zoneKind == ZoneKind::Combat, m_portalChannelRemaining, kPortalChannelDuration);
	minimapSystem->RenderMinimap(*registry, target, playerEntity);

    std::string hudLine;
    if (m_zoneKind == ZoneKind::Town) {
        if (m_playerNearPortal) {
            auto& campaign = CampaignManager::Instance();
            if (campaign.HasActiveMapAttempt()) {
                hudLine = "ポータルをクリックしてTier " + std::to_string(campaign.GetEndgameMapTier())
                    + " のマップへ再入場 (残りポータル " + std::to_string(campaign.GetMapDeathsRemaining()) + "/" + std::to_string(campaign.GetMapDeathsMax()) + ")";
            } else {
                hudLine = "クリックしてAtlasを開く (所持ウェイストーン: " + HeldWaystoneSummary() + ")";
            }
        }
        else if (m_nearNpcIndex >= 0) hudLine = NpcHudHint(m_townNpcs[m_nearNpcIndex].kind);
    } else {
        int aliveEnemies = 0;
        auto enemyView = registry->View<CharacterStatsComponent>();
        for (auto entity : enemyView) {
            if (!registry->HasComponent<PlayerTag>(entity)) {
                aliveEnemies++;
            }
        }
        if (registry->IsValid(m_nearReturnPortal)) {
            hudLine = "ポータルをクリックして町へ帰還";
        } else if (m_portalChannelRemaining > 0.0f) {
            hudLine = "帰還用ポータルを詠唱中...(被弾でキャンセル)";
        } else {
            hudLine = "残り敵数: " + std::to_string(aliveEnemies);
        }
    }

    std::shared_ptr<sf::Font> m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    if (m_font) {
        std::string zoneLabel = CampaignManager::Instance().GetProgressLabel();
        sf::Text zoneText(*m_font, sf::String::fromUtf8(zoneLabel.begin(), zoneLabel.end()), 20);
        zoneText.setFillColor(sf::Color::Yellow);
        zoneText.setOutlineColor(sf::Color::Black);
        zoneText.setOutlineThickness(2.0f);
        sf::FloatRect zoneBounds = zoneText.getLocalBounds();
        zoneText.setOrigin({ zoneBounds.position.x + zoneBounds.size.x / 2.0f, 0.0f });
        zoneText.setPosition({ target.getSize().x / 2.0f, 10.0f });
        target.draw(zoneText);

        if (!hudLine.empty()) {
            sf::Text hudText(*m_font, sf::String::fromUtf8(hudLine.begin(), hudLine.end()), 20);
            hudText.setFillColor(sf::Color::White);
            hudText.setOutlineColor(sf::Color::Black);
            hudText.setOutlineThickness(2.0f);
            sf::FloatRect hudBounds = hudText.getLocalBounds();
            hudText.setOrigin({ hudBounds.position.x + hudBounds.size.x / 2.0f, 0.0f });
            hudText.setPosition({ target.getSize().x / 2.0f, 36.0f });
            target.draw(hudText);
        }

        // 地面のアイテムにカーソルが乗っている間、名前をカーソル脇に表示する
        // (マウスが実際にUIパネルへ重なっている間だけ誤表示を防ぐため止める)。
        sf::Vector2f mouseScreenPosForHover = InputManager::Instance().GetMouseInput().GetMousePointF();
        bool uiOwnsClicksForHover = passiveTreeSystem->isOpen || atlasSystem->isOpen || vendorSystem->isOpen || keyBindSystem->isOpen ||
            inventorySystem->IsPointInPanel(mouseScreenPosForHover, target.getSize()) ||
            characterSheetSystem->IsPointInPanel(mouseScreenPosForHover) ||
            skillGemSystem->IsPointInPanel(mouseScreenPosForHover);
        if (!uiOwnsClicksForHover) {
            sf::Vector2f mouseWorldPos = InputManager::Instance().GetMouseWorldPosition();
            Entity hovered = ItemPickupSystem::FindNearestPickup(*registry, mouseWorldPos, ItemPickupSystem::kClickRadius);
            std::string hoverName;
            sf::Color hoverColor;
            if (ItemPickupSystem::GetPickupDisplay(*registry, hovered, hoverName, hoverColor)) {
                sf::Vector2f mouseScreen = InputManager::Instance().GetMouseInput().GetMousePointF();
                sf::Text hoverText(*m_font, sf::String::fromUtf8(hoverName.begin(), hoverName.end()), 16);
                hoverText.setFillColor(hoverColor);
                hoverText.setOutlineColor(sf::Color::Black);
                hoverText.setOutlineThickness(2.0f);
                hoverText.setPosition({ mouseScreen.x + 16.0f, mouseScreen.y - 22.0f });
                target.draw(hoverText);
            }
        }

        if (itemPickupSystem->messageTimer > 0.0f && !itemPickupSystem->lastMessage.empty()) {
            sf::Text pickupText(*m_font, sf::String::fromUtf8(itemPickupSystem->lastMessage.begin(), itemPickupSystem->lastMessage.end()), 22);
            pickupText.setFillColor(sf::Color(255, 230, 120));
            pickupText.setOutlineColor(sf::Color::Black);
            pickupText.setOutlineThickness(2.0f);
            sf::FloatRect pickupBounds = pickupText.getLocalBounds();
            pickupText.setOrigin({ pickupBounds.position.x + pickupBounds.size.x / 2.0f, 0.0f });
            pickupText.setPosition({ target.getSize().x / 2.0f, 64.0f });
            target.draw(pickupText);
        }

        if (m_portalMessageTimer > 0.0f && !m_portalMessage.empty()) {
            sf::Text portalText(*m_font, sf::String::fromUtf8(m_portalMessage.begin(), m_portalMessage.end()), 22);
            portalText.setFillColor(sf::Color(255, 210, 60));
            portalText.setOutlineColor(sf::Color::Black);
            portalText.setOutlineThickness(2.0f);
            sf::FloatRect portalBounds = portalText.getLocalBounds();
            portalText.setOrigin({ portalBounds.position.x + portalBounds.size.x / 2.0f, 0.0f });
            portalText.setPosition({ target.getSize().x / 2.0f, 64.0f });
            target.draw(portalText);
        }

        if (collisionSystem->levelUpMessageTimer > 0.0f && !collisionSystem->levelUpMessage.empty()) {
            sf::Text levelText(*m_font, sf::String::fromUtf8(collisionSystem->levelUpMessage.begin(), collisionSystem->levelUpMessage.end()), 32);
            levelText.setFillColor(sf::Color(255, 255, 255));
            levelText.setOutlineColor(sf::Color(255, 160, 0));
            levelText.setOutlineThickness(3.0f);
            sf::FloatRect levelBounds = levelText.getLocalBounds();
            levelText.setOrigin({ levelBounds.position.x + levelBounds.size.x / 2.0f, 0.0f });
            levelText.setPosition({ target.getSize().x / 2.0f, 96.0f });
            target.draw(levelText);
        }

        for (auto bossEntity : registry->View<BossTag, CharacterStatsComponent>()) {
            auto& bossStats = registry->GetComponent<CharacterStatsComponent>(bossEntity);
            if (bossStats.currentHP <= 0.0f) continue;

            float barWidth = 500.0f;
            float barHeight = 22.0f;
            float barX = target.getSize().x / 2.0f - barWidth / 2.0f;
            float barY = 130.0f;

            sf::RectangleShape bg({ barWidth, barHeight });
            bg.setPosition({ barX, barY });
            bg.setFillColor(sf::Color(30, 30, 30));
            bg.setOutlineColor(sf::Color::White);
            bg.setOutlineThickness(2.0f);
            target.draw(bg);

            float ratio = std::clamp(bossStats.currentHP / bossStats.maxHP, 0.0f, 1.0f);
            sf::RectangleShape fill({ barWidth * ratio, barHeight });
            fill.setPosition({ barX, barY });
            fill.setFillColor(sf::Color(200, 30, 30));
            target.draw(fill);

            sf::Text bossName(*m_font, sf::String::fromUtf8(bossStats.name.begin(), bossStats.name.end()), 18);
            bossName.setFillColor(sf::Color::White);
            bossName.setOutlineColor(sf::Color::Black);
            bossName.setOutlineThickness(2.0f);
            sf::FloatRect nameBounds = bossName.getLocalBounds();
            bossName.setOrigin({ nameBounds.position.x + nameBounds.size.x / 2.0f, 0.0f });
            bossName.setPosition({ target.getSize().x / 2.0f, barY - 22.0f });
            target.draw(bossName);

            // プレイヤーがボスへ与えた累計ダメージをHPバー右上に表示する
            // ("プレイヤーがどれだけダメージを与えているか分かりやすく"という指示対応)。
            // 一定時間ダメージを与えられていないとCollisionSystem側で0にリセットされ、
            // ここでは0以下なら単に表示しない(=自動的に消える)。
            if (collisionSystem->bossDamageDealt > 0.0f) {
                std::string dmgStr = "与ダメージ: " + std::to_string(static_cast<int>(collisionSystem->bossDamageDealt));
                sf::Text dmgText(*m_font, sf::String::fromUtf8(dmgStr.begin(), dmgStr.end()), 18);
                dmgText.setFillColor(sf::Color(255, 220, 120));
                dmgText.setOutlineColor(sf::Color::Black);
                dmgText.setOutlineThickness(2.0f);
                sf::FloatRect dmgBounds = dmgText.getLocalBounds();
                dmgText.setOrigin({ dmgBounds.position.x + dmgBounds.size.x, 0.0f });
                dmgText.setPosition({ barX + barWidth, barY - 22.0f });
                target.draw(dmgText);
            }
            break;
        }

        if (bossPhaseSystem->enrageMessageTimer > 0.0f && !bossPhaseSystem->enrageMessage.empty()) {
            const std::string& enrageMsg = bossPhaseSystem->enrageMessage;
            sf::Text enrageText(*m_font, sf::String::fromUtf8(enrageMsg.begin(), enrageMsg.end()), 26);
            enrageText.setFillColor(sf::Color(255, 60, 60));
            enrageText.setOutlineColor(sf::Color::Black);
            enrageText.setOutlineThickness(2.0f);
            sf::FloatRect enrageBounds = enrageText.getLocalBounds();
            enrageText.setOrigin({ enrageBounds.position.x + enrageBounds.size.x / 2.0f, 0.0f });
            enrageText.setPosition({ target.getSize().x / 2.0f, 160.0f });
            target.draw(enrageText);
        }
    }

    characterSheetSystem->Render(*registry, target);
    inventorySystem->Render(*registry, target);
    passiveTreeSystem->Render(*registry, target);
    atlasSystem->Render(*registry, target);
    vendorSystem->Render(*registry, target);
    waystoneVendorSystem->Render(*registry, target);
    stashSystem->Render(*registry, target);
    skillGemSystem->Render(*registry, target);
    gemIdentifySystem->Render(*registry, target);
    keyBindSystem->Render(*registry, target);
    minimapSystem->RenderFullMap(*registry, target, playerEntity);

    target.setView(target.getDefaultView());
}

void GameScene::RenderImGui(const sf::Texture* renderTexture)
{
    DebugGui::Begin("Game Tools", "�Q�[���c�[��");

    ImGui::SameLine();

    if (DebugGui::Button("Spawn Enemy", "�G����")) {
        auto e = EntitySpawner::CreateEnemy(*registry, { 400, 300 });
        editorSystem->SetSelectedEntity(e.GetID());
    }

    DebugGui::End();

    RenderGemDebugTools();

    editorSystem->RenderImGui(*registry);
}

// F1 debug menu only (see Application::Render's IsDebugMode gate). Covers the gem-system
// debug tools from the spec: spawning each Uncut Gem kind at a chosen level, setting
// Str/Dex/Int/current Mana, granting Jeweller's Orbs (the existing in-game path to add a
// support socket, reused here instead of a separate one-off socket-adder), and read-only
// panels showing the same Spirit reservation / final skill / support compatibility numbers
// the real UI computes -- so this can never show a different result than actual gameplay
// (see SkillActivationSystem's doc comment for the same "UI and Combat share one source of
// truth" principle).
void GameScene::RenderGemDebugTools() {
    if (!registry->IsValid(playerEntity)) return;
    if (!registry->HasComponent<InventoryComponent>(playerEntity) || !registry->HasComponent<CharacterStatsComponent>(playerEntity)
        || !registry->HasComponent<EquipmentComponent>(playerEntity)) return;

    auto& inventory = registry->GetComponent<InventoryComponent>(playerEntity);
    auto& stats = registry->GetComponent<CharacterStatsComponent>(playerEntity);
    auto& equipment = registry->GetComponent<EquipmentComponent>(playerEntity);

    DebugGui::Begin("Gem Debug", "ジェムデバッグ");

    static int debugGemLevel = 1;
    ImGui::SliderInt("Gem Level", &debugGemLevel, 1, 20);
    int col, row;
    bool full = !ItemUIHelpers::FindBagFreeSpace(inventory.items, 1, 1, col, row);
    if (full) ImGui::TextDisabled("Bag full");
    ImGui::BeginDisabled(full);
    auto spawnUncut = [&](GemPickupKind kind) {
        int c, r;
        if (!ItemUIHelpers::FindBagFreeSpace(inventory.items, 1, 1, c, r)) return;
        ItemComponent item = ItemFactory::GenerateUncutSkillGem(debugGemLevel, kind);
        item.gridCol = c;
        item.gridRow = r;
        inventory.items.push_back(item);
    };
    if (ImGui::Button("Spawn Uncut Skill Gem")) spawnUncut(GemPickupKind::Skill);
    ImGui::SameLine();
    if (ImGui::Button("Spawn Uncut Support Gem")) spawnUncut(GemPickupKind::Support);
    ImGui::SameLine();
    if (ImGui::Button("Spawn Uncut Spirit Gem")) spawnUncut(GemPickupKind::Spirit);
    ImGui::EndDisabled();

    ImGui::Separator();
    bool statsChanged = false;
    statsChanged |= ImGui::SliderInt("Strength", &stats.str, 1, 200);
    statsChanged |= ImGui::SliderInt("Dexterity", &stats.dex, 1, 200);
    statsChanged |= ImGui::SliderInt("Intelligence", &stats.intelligence, 1, 200);
    if (statsChanged) EquipmentSystem::RecalculateStats(stats, equipment);
    if (ImGui::SliderFloat("Max Spirit (base)", &equipment.baseStats.maxSpirit, 0.0f, 500.0f)) {
        EquipmentSystem::RecalculateStats(stats, equipment);
    }
    ImGui::SliderFloat("Current Mana", &stats.currentMP, 0.0f, stats.maxMP);
    if (ImGui::Button("Give 5 Jeweller's Orbs")) stats.jewellersOrbs += 5;

    ImGui::Separator();
    ImGui::Text("Spirit: %.1f reserved / %.1f max", stats.currentSpirit, stats.maxSpirit);
    if (registry->HasComponent<SpiritGemLoadoutComponent>(playerEntity)) {
        auto& loadout = registry->GetComponent<SpiritGemLoadoutComponent>(playerEntity);
        for (int i = 0; i < static_cast<int>(loadout.items.size()); ++i) {
            if (!loadout.items[i]) continue;
            const GemDefinition* def = SkillGemData::Find(loadout.items[i]->skillGemId);
            ImGui::Text("  Slot %d: %s [%s] cost=%.1f", i + 1, def ? def->skill.name.c_str() : "?",
                loadout.active[i] ? "ON" : "OFF", SpiritAuraSystem::SpiritCostOf(*loadout.items[i]));
        }
    }

    ImGui::Separator();
    ImGui::Text("Skill Calculation (final, post-Support/level):");
    if (registry->HasComponent<PlayerSkill>(playerEntity)) {
        auto& skillComp = registry->GetComponent<PlayerSkill>(playerEntity);
        for (int i = 0; i < static_cast<int>(skillComp.skills.size()); ++i) {
            const SkillData& s = skillComp.skills[i];
            if (!s.isValid) continue;
            ImGui::Text("  [%d] %s Lv%d dmg=%.1f mp=%d cd=%.2fs", i + 1, s.name.c_str(), s.level, s.damage, s.mpCost, s.cooldownTime);
        }

        ImGui::Separator();
        ImGui::Text("Support Compatibility vs Skill Slot 1:");
        if (skillComp.skills[0].isValid) {
            unsigned int tags = SkillTags::TagsFor(skillComp.skills[0].behaviorType, skillComp.skills[0].element);
            for (const auto& def : SupportGemData::Gems()) {
                bool ok = SkillTags::IsCompatible(tags, def.requiredTag);
                ImGui::Text("  %s: %s", def.name.c_str(), ok ? "Compatible" : "Incompatible (tag)");
            }
        } else {
            ImGui::TextDisabled("  (Slot 1 is empty)");
        }
    }

    DebugGui::End();
}