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

    ZoneBuildResult built = ZoneBuilder::Build(*registry, zone, campaign.GetEndgameMapTier(), campaign.CurrentAct().isEndgame, campaign.GetActiveMapMods());
    m_hasPortal = built.hasPortal;
    m_portalPos = built.portalPos;
    m_townNpcs = built.townNpcs;

    auto player = EntitySpawner::CreatePlayer(*registry, built.playerSpawn.x, built.playerSpawn.y);
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

            // Older saves (pre-gem-overhaul) have no owned-gem data; keep the
            // EntitySpawner-seeded starter loadout in that case instead of blanking it out.
            // pendingUncutGems restores independently of ownedGems -- a player who saves
            // right after picking up their very first uncut gem (before identifying
            // anything) would otherwise lose it on reload.
            player.GetComponent<SkillGemInventoryComponent>().pendingUncutGems = campaign.GetSavedPendingUncutGems();

            if (!campaign.GetSavedOwnedGems().empty()) {
                auto& gemInventory = player.GetComponent<SkillGemInventoryComponent>();
                gemInventory.ownedGems = campaign.GetSavedOwnedGems();

                auto& skillComp = player.GetComponent<PlayerSkill>();
                const auto& loadout = campaign.GetSavedSkillLoadout();
                for (size_t i = 0; i < loadout.size(); ++i) {
                    int gemId = loadout[i];
                    if (gemId < 0) {
                        skillComp.skills[i] = SkillData{};
                        continue;
                    }
                    // Rebuilds level scaling + socketed supports from gemInventory, not
                    // just a raw catalog copy -- matches SkillGemSystem::AssignGem so a
                    // saved character's skills come back exactly as they were.
                    SkillGemScaling::BuildEquippedSkillData(skillComp.skills[i], gemId, gemInventory);
                }
            }

            // The aura's stat bonus already lives in equipment.baseStats (restored above),
            // so this only restores which gem each Spirit slot displays as equipped and
            // whether it was toggled ON (for the UI's ON/OFF button state -- the actual
            // reserved amount is CharacterStatsComponent::currentSpirit, already restored
            // as part of the stats blob above).
            auto& spiritLoadout = player.GetComponent<SpiritGemLoadoutComponent>();
            spiritLoadout.auraGemIds = campaign.GetSavedAuraLoadout();
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
}

void GameScene::AdvanceToNextZone() {
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
    if (registry->HasComponent<SkillGemInventoryComponent>(playerEntity)) {
        campaign.SaveOwnedGems(registry->GetComponent<SkillGemInventoryComponent>(playerEntity).ownedGems);
        campaign.SavePendingUncutGems(registry->GetComponent<SkillGemInventoryComponent>(playerEntity).pendingUncutGems);
    }
    if (registry->HasComponent<PlayerSkill>(playerEntity)) {
        auto& skillComp = registry->GetComponent<PlayerSkill>(playerEntity);
        std::array<int, 5> loadout;
        for (size_t i = 0; i < loadout.size(); ++i) {
            loadout[i] = skillComp.skills[i].isValid ? skillComp.skills[i].gemId : -1;
        }
        campaign.SaveSkillLoadout(loadout);
    }
    if (registry->HasComponent<SpiritGemLoadoutComponent>(playerEntity)) {
        auto& spiritLoadout = registry->GetComponent<SpiritGemLoadoutComponent>(playerEntity);
        campaign.SaveAuraLoadout(spiritLoadout.auraGemIds);
        campaign.SaveAuraActive(spiritLoadout.active);
    }
    campaign.CompleteCurrentZoneAndAdvance();
    campaign.SaveToDisk();
    SceneManager::Instance().ChangeScene("GameScene");
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
    case TownNpcKind::ItemVendor: return "Click the merchant to trade";
    case TownNpcKind::WaystoneVendor: return "Click the Waystone vendor to buy Waystones";
    case TownNpcKind::Stash: return "Click the stash to store items";
    default: return "";
    }
}

// Endgame hub's map device. If a map attempt is still open (<=6 deaths used, see
// CampaignManager::HasActiveMapAttempt), re-enters that same tier/mods for free -- one
// of the up-to-6 portals real PoE2 grants per Waystone, letting the player try again
// after dying without needing another Waystone. Otherwise spends the player's
// highest-tier held Waystone (matches PoE2's advice to always run your best one),
// consuming it and rolling a fresh 6-portal attempt. Refuses (with a message, no zone
// change) if the player holds none.
void GameScene::TryOpenEndgameMapFromHub() {
    auto& campaign = CampaignManager::Instance();

    if (campaign.HasActiveMapAttempt()) {
        AdvanceToNextZone();
        return;
    }

    if (!registry->HasComponent<InventoryComponent>(playerEntity)) return;
    auto& inventory = registry->GetComponent<InventoryComponent>(playerEntity);

    int bestIndex = -1;
    int highestTier = -1;
    for (size_t i = 0; i < inventory.items.size(); ++i) {
        const auto& item = inventory.items[i];
        if (item.category != ItemCategory::Waystone) continue;
        if (item.waystoneTier > highestTier) {
            highestTier = item.waystoneTier;
            bestIndex = static_cast<int>(i);
        }
    }

    if (bestIndex < 0) {
        itemPickupSystem->lastMessage = "Need a Waystone to open a map";
        itemPickupSystem->messageTimer = 2.5f;
        return;
    }

    std::vector<WaystoneMod> mods = inventory.items[bestIndex].waystoneMods;
    inventory.items.erase(inventory.items.begin() + bestIndex);
    campaign.OpenEndgameMap(highestTier, mods);
    spdlog::info("Opening endgame map at tier {}.", highestTier);
    AdvanceToNextZone();
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
        vendorSystem->Close();
        waystoneVendorSystem->Close();
        stashSystem->Close();
        keyBindSystem->Close();
        gemIdentifySystem->Close();
    }

    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleCharacterSheet))) {
        characterSheetSystem->Toggle();
        if (characterSheetSystem->isOpen) { skillGemSystem->Close(); passiveTreeSystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleInventory))) {
        inventorySystem->Toggle();
        if (inventorySystem->isOpen) { passiveTreeSystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::TogglePassiveTree))) {
        passiveTreeSystem->Toggle();
        if (passiveTreeSystem->isOpen) { closeFreePanels(); vendorSystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleSkillGems))) {
        skillGemSystem->Toggle();
        if (skillGemSystem->isOpen) { characterSheetSystem->isOpen = false; passiveTreeSystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
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
    if (m_clickedOnNpc) {
        switch (m_townNpcs[m_nearNpcIndex].kind) {
        case TownNpcKind::ItemVendor: {
            int playerLevel = registry->HasComponent<CharacterStatsComponent>(playerEntity)
                ? registry->GetComponent<CharacterStatsComponent>(playerEntity).level : 1;
            vendorSystem->Toggle(playerLevel);
            if (vendorSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); }
            break;
        }
        case TownNpcKind::WaystoneVendor:
            waystoneVendorSystem->Toggle();
            if (waystoneVendorSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); vendorSystem->Close(); stashSystem->Close(); }
            break;
        case TownNpcKind::Stash:
            stashSystem->Toggle();
            if (stashSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); vendorSystem->Close(); waystoneVendorSystem->Close(); }
            break;
        }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::O)) {
        keyBindSystem->Toggle();
        if (keyBindSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); vendorSystem->Close(); gemIdentifySystem->Close(); waystoneVendorSystem->Close(); stashSystem->Close(); }
    }
    keyBindSystem->Update(*registry, dt);
    inventorySystem->Update(*registry, dt);
    passiveTreeSystem->Update(*registry, dt);
    vendorSystem->Update(*registry, dt);
    waystoneVendorSystem->Update(*registry, dt);
    stashSystem->Update(*registry, dt);
    skillGemSystem->Update(*registry, dt, *gemIdentifySystem);
    gemIdentifySystem->Update(*registry, dt);

    // パッシブツリー等をゆっくり操作できるよう、それらのメニューが開いている間は
    // ワールドシミュレーションを止める。ただしインベントリ(アイテム)/キャラクター
    // シート(ステータス)/スキルジェムはPoE2同様、開いたまま戦闘・移動を続けられる
    // ようにする(スキルジェム画面を開くとゲームが固まる不具合の修正)。未鑑定ジェムの
    // 選択画面(GemIdentifySystem)も選んでいる間は戦闘が進まないようポーズ系に含める。
    bool isPaused = passiveTreeSystem->isOpen || vendorSystem->isOpen || waystoneVendorSystem->isOpen || stashSystem->isOpen || keyBindSystem->isOpen || gemIdentifySystem->isOpen;
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

    if (!isPaused) {
        // ����
        inputSystem->Update(*registry, dt, uiOwnsClicks || registry->IsValid(hoveredPickup) || m_clickedOnNpc);
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

        // Safe to run every frame regardless of what changed maxSpirit/baseStats since the
        // last call (equip swap, level up, passive spend) -- see SpiritAuraSystem::
        // ReevaluateReservations. Not gated by isPaused since it's pure data consistency,
        // not world simulation.
        if (registry->HasComponent<SpiritGemLoadoutComponent>(playerEntity) && registry->HasComponent<SkillGemInventoryComponent>(playerEntity)
            && registry->HasComponent<EquipmentComponent>(playerEntity) && registry->HasComponent<CharacterStatsComponent>(playerEntity)) {
            SpiritAuraSystem::ReevaluateReservations(*registry, playerEntity,
                registry->GetComponent<SpiritGemLoadoutComponent>(playerEntity),
                registry->GetComponent<SkillGemInventoryComponent>(playerEntity),
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
        statusEffectSystem->Update(*registry, dt);
        // �������Z
        physicsSystem->Update(*registry, dt);
        // �Փˏ���
        collisionSystem->Update(*registry, dt);
        itemPickupSystem->Update(*registry, dt, clickedPickup);
        bossPhaseSystem->Update(*registry, dt);
    }

    // �Q�[���̏I���m�F
    if (registry->HasComponent<CharacterStatsComponent>(playerEntity)) {
        auto& state = registry->GetComponent<CharacterStatsComponent>(playerEntity);
        if (state.currentHP <= 0) {
            // Dying inside a map consumes one of its up to 6 portals instead of just
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

                if (m_playerNearPortal && !inventorySystem->isOpen && !passiveTreeSystem->isOpen && !vendorSystem->isOpen && !skillGemSystem->isOpen && !keyBindSystem->isOpen &&
                    InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::Enter)) {
                    TryOpenEndgameMapFromHub();
                    return;
                }
            }
        }
    } else {
        auto enemyView = registry->View<CharacterStatsComponent>();
        bool anyEnemyAlive = false;

        for (auto entity : enemyView) {
            if (!registry->HasComponent<PlayerTag>(entity) && !registry->HasComponent<AllyTagComponent>(entity)) {
                anyEnemyAlive = true;
                break;
            }
        }
        if (!anyEnemyAlive) {
            spdlog::info("Zone cleared!");
            AdvanceToNextZone();
            return;
        }
    }
}

void GameScene::Render(sf::RenderTarget& target) {
    target.setView(CameraManager::Instance().GetCurrentView());
	// �}�b�v�`��
	mapRenderSystem->Render(*registry, target);
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

    target.setView(target.getDefaultView());
	uiSystem->Render(*registry, target);

    std::string hudLine;
    if (m_zoneKind == ZoneKind::Town) {
        if (m_playerNearPortal) {
            auto& campaign = CampaignManager::Instance();
            if (campaign.HasActiveMapAttempt()) {
                hudLine = "Press Enter to re-enter your Tier " + std::to_string(campaign.GetEndgameMapTier())
                    + " map (" + std::to_string(campaign.GetMapDeathsRemaining()) + " portals left)";
            } else {
                hudLine = "Press Enter to open a map with your highest Waystone (" + HeldWaystoneSummary() + ")";
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
        hudLine = "Remaining Enemies: " + std::to_string(aliveEnemies);
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
        bool uiOwnsClicksForHover = passiveTreeSystem->isOpen || vendorSystem->isOpen || keyBindSystem->isOpen ||
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
    vendorSystem->Render(*registry, target);
    waystoneVendorSystem->Render(*registry, target);
    stashSystem->Render(*registry, target);
    skillGemSystem->Render(*registry, target);
    gemIdentifySystem->Render(*registry, target);
    keyBindSystem->Render(*registry, target);

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
    if (!registry->HasComponent<SkillGemInventoryComponent>(playerEntity) || !registry->HasComponent<CharacterStatsComponent>(playerEntity)
        || !registry->HasComponent<EquipmentComponent>(playerEntity)) return;

    auto& gemInventory = registry->GetComponent<SkillGemInventoryComponent>(playerEntity);
    auto& stats = registry->GetComponent<CharacterStatsComponent>(playerEntity);
    auto& equipment = registry->GetComponent<EquipmentComponent>(playerEntity);

    DebugGui::Begin("Gem Debug", "ジェムデバッグ");

    static int debugGemLevel = 1;
    ImGui::SliderInt("Gem Level", &debugGemLevel, 1, 20);
    bool full = gemInventory.pendingUncutGems.size() >= SkillGemInventoryComponent::kPendingCapacity;
    if (full) ImGui::TextDisabled("Uncut Gem storage full");
    ImGui::BeginDisabled(full);
    if (ImGui::Button("Spawn Uncut Skill Gem")) gemInventory.pendingUncutGems.push_back(PendingUncutGem{ debugGemLevel, GemPickupKind::Skill });
    ImGui::SameLine();
    if (ImGui::Button("Spawn Uncut Support Gem")) gemInventory.pendingUncutGems.push_back(PendingUncutGem{ debugGemLevel, GemPickupKind::Support });
    ImGui::SameLine();
    if (ImGui::Button("Spawn Uncut Spirit Gem")) gemInventory.pendingUncutGems.push_back(PendingUncutGem{ debugGemLevel, GemPickupKind::Spirit });
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
        for (int i = 0; i < static_cast<int>(loadout.auraGemIds.size()); ++i) {
            if (loadout.auraGemIds[i] < 0) continue;
            const GemDefinition* def = SkillGemData::Find(loadout.auraGemIds[i]);
            ImGui::Text("  Slot %d: %s [%s] cost=%.1f", i + 1, def ? def->skill.name.c_str() : "?",
                loadout.active[i] ? "ON" : "OFF", SpiritAuraSystem::SpiritCostOf(loadout.auraGemIds[i], gemInventory));
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