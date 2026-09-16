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
	skillGemSystem = std::make_shared<SkillGemSystem>();
	gemIdentifySystem = std::make_shared<GemIdentifySystem>();
	keyBindSystem = std::make_shared<KeyBindSystem>();

    auto& campaign = CampaignManager::Instance();
    const ZoneDefinition& zone = campaign.CurrentZone();
    m_zoneKind = zone.kind;

    ZoneBuildResult built = ZoneBuilder::Build(*registry, zone, campaign.GetEndgameMapTier(), campaign.CurrentAct().isEndgame);
    m_hasPortal = built.hasPortal;
    m_portalPos = built.portalPos;
    m_hasVendor = built.hasVendor;
    m_vendorPos = built.vendorPos;

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
            player.GetComponent<WaystoneInventoryComponent>().counts = campaign.GetSavedWaystones();
        }
        spdlog::info("Player created with ID: {} in zone '{}'", player.GetID(), zone.displayName);
    }

    int totalResPenaltyPercent = 0;
    if (campaign.ConsumePendingResPenaltyNotice(totalResPenaltyPercent)) {
        itemPickupSystem->lastMessage = "幕クリア: 全耐性 -10% (通算 -" + std::to_string(totalResPenaltyPercent) + "%)";
        itemPickupSystem->messageTimer = 4.0f;
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
    if (registry->HasComponent<WaystoneInventoryComponent>(playerEntity)) {
        campaign.SaveWaystones(registry->GetComponent<WaystoneInventoryComponent>(playerEntity).counts);
    }
    campaign.CompleteCurrentZoneAndAdvance();
    campaign.SaveToDisk();
    SceneManager::Instance().ChangeScene("GameScene");
}

// "T3x2 T1x1" style summary of held Waystones for the hub's HUD prompt.
std::string GameScene::HeldWaystoneSummary() const {
    if (!registry->HasComponent<WaystoneInventoryComponent>(playerEntity)) return "none";
    const auto& waystones = registry->GetComponent<WaystoneInventoryComponent>(playerEntity);

    std::string summary;
    for (int t = WaystoneInventoryComponent::kMaxTier; t >= 1; --t) {
        int count = waystones.counts[t - 1];
        if (count <= 0) continue;
        if (!summary.empty()) summary += " ";
        summary += "T" + std::to_string(t) + "x" + std::to_string(count);
    }
    return summary.empty() ? "none" : summary;
}

// Endgame hub's map device: spends the player's highest-tier held Waystone to open a
// map at that tier (matches PoE2's advice to always run your best Waystone). Refuses
// (with a message, no zone change) if the player holds none.
void GameScene::TryOpenEndgameMapFromHub() {
    if (!registry->HasComponent<WaystoneInventoryComponent>(playerEntity)) return;
    auto& waystones = registry->GetComponent<WaystoneInventoryComponent>(playerEntity);

    int highestTier = -1;
    for (int t = WaystoneInventoryComponent::kMaxTier; t >= 1; --t) {
        if (waystones.counts[t - 1] > 0) { highestTier = t; break; }
    }

    if (highestTier < 0) {
        itemPickupSystem->lastMessage = "Need a Waystone to open a map";
        itemPickupSystem->messageTimer = 2.5f;
        return;
    }

    waystones.counts[highestTier - 1]--;
    CampaignManager::Instance().OpenEndgameMap(highestTier);
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
        keyBindSystem->Close();
        gemIdentifySystem->Close();
    }

    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleCharacterSheet))) {
        characterSheetSystem->Toggle();
        if (characterSheetSystem->isOpen) { skillGemSystem->Close(); passiveTreeSystem->Close(); vendorSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleInventory))) {
        inventorySystem->Toggle();
        if (inventorySystem->isOpen) { passiveTreeSystem->Close(); vendorSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::TogglePassiveTree))) {
        passiveTreeSystem->Toggle();
        if (passiveTreeSystem->isOpen) { closeFreePanels(); vendorSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleSkillGems))) {
        skillGemSystem->Toggle();
        if (skillGemSystem->isOpen) { characterSheetSystem->isOpen = false; passiveTreeSystem->Close(); vendorSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    // PoE2同様、NPCへの話しかけは左クリックのみ(Bキーの「話しかける」操作は廃止)。
    // 商人本体にカーソルが乗っているかは常時判定し、Render側で当たり判定の輪を
    // 表示することでクリック可能な範囲を視覚的に分かるようにする。
    m_hoveringVendor = false;
    m_clickedOnVendor = false;
    if (m_hasVendor && m_playerNearVendor && !vendorSystem->isOpen) {
        sf::Vector2f mouseWorldForVendor = InputManager::Instance().GetMouseWorldPosition();
        float vdx = mouseWorldForVendor.x - m_vendorPos.x;
        float vdy = mouseWorldForVendor.y - m_vendorPos.y;
        m_hoveringVendor = (vdx * vdx + vdy * vdy) < (kVendorClickRadius * kVendorClickRadius);
        m_clickedOnVendor = m_hoveringVendor && InputManager::Instance().GetMouseInput().IsGetMouse(sf::Mouse::Button::Left);
    }
    if (m_clickedOnVendor) {
        int playerLevel = registry->HasComponent<CharacterStatsComponent>(playerEntity)
            ? registry->GetComponent<CharacterStatsComponent>(playerEntity).level : 1;
        vendorSystem->Toggle(playerLevel);
        if (vendorSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); keyBindSystem->Close(); gemIdentifySystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::O)) {
        keyBindSystem->Toggle();
        if (keyBindSystem->isOpen) { closeFreePanels(); passiveTreeSystem->Close(); vendorSystem->Close(); gemIdentifySystem->Close(); }
    }
    keyBindSystem->Update(*registry, dt);
    inventorySystem->Update(*registry, dt);
    passiveTreeSystem->Update(*registry, dt);
    vendorSystem->Update(*registry, dt);
    skillGemSystem->Update(*registry, dt, *gemIdentifySystem);
    gemIdentifySystem->Update(*registry, dt);

    // パッシブツリー等をゆっくり操作できるよう、それらのメニューが開いている間は
    // ワールドシミュレーションを止める。ただしインベントリ(アイテム)/キャラクター
    // シート(ステータス)/スキルジェムはPoE2同様、開いたまま戦闘・移動を続けられる
    // ようにする(スキルジェム画面を開くとゲームが固まる不具合の修正)。未鑑定ジェムの
    // 選択画面(GemIdentifySystem)も選んでいる間は戦闘が進まないようポーズ系に含める。
    bool isPaused = passiveTreeSystem->isOpen || vendorSystem->isOpen || keyBindSystem->isOpen || gemIdentifySystem->isOpen;
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
        inputSystem->Update(*registry, dt, uiOwnsClicks || registry->IsValid(hoveredPickup) || m_clickedOnVendor);
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
			spdlog::info("Player has died. Ending game.");
			SceneManager::Instance().ChangeScene("ResultScene");
            return;
        }
    }
    if (m_zoneKind == ZoneKind::Town) {
        m_playerNearPortal = false;
        m_playerNearVendor = false;
        if (registry->HasComponent<TransformComponent>(playerEntity)) {
            auto& trans = registry->GetComponent<TransformComponent>(playerEntity);

            if (m_hasVendor) {
                float vdx = trans.position.x - m_vendorPos.x;
                float vdy = trans.position.y - m_vendorPos.y;
                m_playerNearVendor = (vdx * vdx + vdy * vdy) < (100.0f * 100.0f);
            }

            if (m_hasPortal) {
                float dx = trans.position.x - m_portalPos.x;
                float dy = trans.position.y - m_portalPos.y;
                float distSq = dx * dx + dy * dy;
                m_playerNearPortal = distSq < (150.0f * 150.0f);

                if (m_playerNearPortal && !inventorySystem->isOpen && !passiveTreeSystem->isOpen && !vendorSystem->isOpen && !skillGemSystem->isOpen && !keyBindSystem->isOpen &&
                    InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::Enter)) {
                    if (CampaignManager::Instance().CurrentAct().isEndgame) {
                        TryOpenEndgameMapFromHub();
                    } else {
                        spdlog::info("Entering next zone.");
                        AdvanceToNextZone();
                    }
                    return;
                }
            }
        }
    } else {
        auto enemyView = registry->View<CharacterStatsComponent>();
        bool anyEnemyAlive = false;

        for (auto entity : enemyView) {
            if (!registry->HasComponent<PlayerTag>(entity)) {
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

    // 商人がクリック可能な範囲を視覚的に示す輪(近づいている間のみ表示、
    // カーソルが範囲内なら明るく強調してクリックできることが分かるようにする)。
    if (m_hasVendor && m_playerNearVendor && !vendorSystem->isOpen) {
        sf::CircleShape ring(kVendorClickRadius);
        ring.setOrigin({ kVendorClickRadius, kVendorClickRadius });
        ring.setPosition(m_vendorPos);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.0f);
        ring.setOutlineColor(m_hoveringVendor ? sf::Color(255, 255, 120, 220) : sf::Color(80, 200, 255, 140));
        target.draw(ring);
    }

    target.setView(target.getDefaultView());
	uiSystem->Render(*registry, target);

    std::string hudLine;
    if (m_zoneKind == ZoneKind::Town) {
        if (m_playerNearPortal) {
            if (CampaignManager::Instance().CurrentAct().isEndgame) {
                hudLine = "Press Enter to open a map with your highest Waystone (" + HeldWaystoneSummary() + ")";
            } else {
                hudLine = "Press Enter to proceed";
            }
        }
        else if (m_playerNearVendor) hudLine = "Click the merchant to trade";
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

    editorSystem->RenderImGui(*registry);
}