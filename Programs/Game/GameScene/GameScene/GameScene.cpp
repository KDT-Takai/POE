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
    movementSystem = std::make_shared<MovementSystem>();
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
	keyBindSystem = std::make_shared<KeyBindSystem>();

    auto& campaign = CampaignManager::Instance();
    const ZoneDefinition& zone = campaign.CurrentZone();
    m_zoneKind = zone.kind;

    ZoneBuildResult built = ZoneBuilder::Build(*registry, zone, campaign.GetEndgameMapTier());
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

            // Older saves (pre-skill-gem feature) have no unlocked-gem data; keep the
            // EntitySpawner-seeded starter loadout in that case instead of blanking it out.
            if (!campaign.GetSavedUnlockedGems().empty()) {
                player.GetComponent<SkillGemInventoryComponent>().unlockedGemIds = campaign.GetSavedUnlockedGems();

                auto& skillComp = player.GetComponent<PlayerSkill>();
                const auto& loadout = campaign.GetSavedSkillLoadout();
                for (size_t i = 0; i < loadout.size(); ++i) {
                    int gemId = loadout[i];
                    if (gemId < 0) {
                        skillComp.skills[i] = SkillData{};
                        continue;
                    }
                    const GemDefinition* def = SkillGemData::Find(gemId);
                    if (!def) continue;
                    skillComp.skills[i] = def->skill;
                    skillComp.skills[i].gemId = gemId;
                    skillComp.skills[i].isValid = true;
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
    if (registry->HasComponent<PassiveTreeComponent>(playerEntity)) {
        campaign.SavePassiveTree(registry->GetComponent<PassiveTreeComponent>(playerEntity).allocatedNodeIds);
    }
    if (registry->HasComponent<SkillGemInventoryComponent>(playerEntity)) {
        campaign.SaveUnlockedGems(registry->GetComponent<SkillGemInventoryComponent>(playerEntity).unlockedGemIds);
    }
    if (registry->HasComponent<PlayerSkill>(playerEntity)) {
        auto& skillComp = registry->GetComponent<PlayerSkill>(playerEntity);
        std::array<int, 5> loadout;
        for (size_t i = 0; i < loadout.size(); ++i) {
            loadout[i] = skillComp.skills[i].isValid ? skillComp.skills[i].gemId : -1;
        }
        campaign.SaveSkillLoadout(loadout);
    }
    campaign.CompleteCurrentZoneAndAdvance();
    campaign.SaveToDisk();
    SceneManager::Instance().ChangeScene("GameScene");
}

void GameScene::Update() {
    auto dt = Time::Instance().GetDeltaTime();
    CameraManager::Instance().UpdateShake(dt);

    auto& binds = KeyBindings::Instance();
    bool awaitingRebind = keyBindSystem->IsAwaitingKey();

    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleCharacterSheet))) {
        characterSheetSystem->Toggle();
        if (characterSheetSystem->isOpen) { inventorySystem->Close(); passiveTreeSystem->Close(); vendorSystem->Close(); skillGemSystem->Close(); keyBindSystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleInventory))) {
        inventorySystem->Toggle();
        if (inventorySystem->isOpen) { characterSheetSystem->isOpen = false; passiveTreeSystem->Close(); vendorSystem->Close(); skillGemSystem->Close(); keyBindSystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::TogglePassiveTree))) {
        passiveTreeSystem->Toggle();
        if (passiveTreeSystem->isOpen) { characterSheetSystem->isOpen = false; inventorySystem->Close(); vendorSystem->Close(); skillGemSystem->Close(); keyBindSystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::ToggleSkillGems))) {
        skillGemSystem->Toggle();
        if (skillGemSystem->isOpen) { characterSheetSystem->isOpen = false; inventorySystem->Close(); passiveTreeSystem->Close(); vendorSystem->Close(); keyBindSystem->Close(); }
    }
    if (!awaitingRebind && m_playerNearVendor && InputManager::Instance().GetKeyInput().IsGetKey(binds.Get(GameAction::VendorToggle))) {
        int playerLevel = registry->HasComponent<CharacterStatsComponent>(playerEntity)
            ? registry->GetComponent<CharacterStatsComponent>(playerEntity).level : 1;
        vendorSystem->Toggle(playerLevel);
        if (vendorSystem->isOpen) { characterSheetSystem->isOpen = false; inventorySystem->Close(); passiveTreeSystem->Close(); skillGemSystem->Close(); keyBindSystem->Close(); }
    }
    if (!awaitingRebind && InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::O)) {
        keyBindSystem->Toggle();
        if (keyBindSystem->isOpen) { characterSheetSystem->isOpen = false; inventorySystem->Close(); passiveTreeSystem->Close(); vendorSystem->Close(); skillGemSystem->Close(); }
    }
    keyBindSystem->Update(*registry, dt);
    inventorySystem->Update(*registry, dt);
    passiveTreeSystem->Update(*registry, dt);
    vendorSystem->Update(*registry, dt);
    skillGemSystem->Update(*registry, dt);

    // いずれかのメニューが開いている間はワールドシミュレーションを止める
    // (パッシブツリー等をゆっくり操作できるようにするため)
    bool isPaused = characterSheetSystem->isOpen || inventorySystem->isOpen || passiveTreeSystem->isOpen
        || vendorSystem->isOpen || skillGemSystem->isOpen || keyBindSystem->isOpen;

    if (!isPaused) {
        // ����
        inputSystem->Update(*registry, dt);
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
        // �ړ�
        movementSystem->Update(*registry, dt);
        statusEffectSystem->Update(*registry, dt);
        // �������Z
        physicsSystem->Update(*registry, dt);
        // �Փˏ���
        collisionSystem->Update(*registry, dt);
        itemPickupSystem->Update(*registry, dt);
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
                    spdlog::info("Entering next zone.");
                    AdvanceToNextZone();
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

    target.setView(target.getDefaultView());
	uiSystem->Render(*registry, target);

    std::string hudLine;
    if (m_zoneKind == ZoneKind::Town) {
        if (m_playerNearPortal) hudLine = "Press Enter to proceed";
        else if (m_playerNearVendor) hudLine = "Press B to trade";
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
            sf::Text hudText(*m_font, hudLine, 20);
            hudText.setFillColor(sf::Color::White);
            hudText.setOutlineColor(sf::Color::Black);
            hudText.setOutlineThickness(2.0f);
            sf::FloatRect hudBounds = hudText.getLocalBounds();
            hudText.setOrigin({ hudBounds.position.x + hudBounds.size.x / 2.0f, 0.0f });
            hudText.setPosition({ target.getSize().x / 2.0f, 36.0f });
            target.draw(hudText);
        }

        if (itemPickupSystem->messageTimer > 0.0f && !itemPickupSystem->lastMessage.empty()) {
            sf::Text pickupText(*m_font, itemPickupSystem->lastMessage, 22);
            pickupText.setFillColor(sf::Color(255, 230, 120));
            pickupText.setOutlineColor(sf::Color::Black);
            pickupText.setOutlineThickness(2.0f);
            sf::FloatRect pickupBounds = pickupText.getLocalBounds();
            pickupText.setOrigin({ pickupBounds.position.x + pickupBounds.size.x / 2.0f, 0.0f });
            pickupText.setPosition({ target.getSize().x / 2.0f, 64.0f });
            target.draw(pickupText);
        }

        if (collisionSystem->levelUpMessageTimer > 0.0f && !collisionSystem->levelUpMessage.empty()) {
            sf::Text levelText(*m_font, collisionSystem->levelUpMessage, 32);
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