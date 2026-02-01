#include "GameScene.h"
#include <ECS.h>
#include <System/CameraManager/CameraManager.h>
#include <System/Time/Time.h>
#include <System/DebugGui/DebugGui.h>

#include "../../../System/DebugManager/DebugManager.h"
#include "../Entity/EntitySpawner.h"

#include "../MapGenerator/MapGenerator.h"

#include "System/SceneManager/SceneManager.h"
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
	collisionSystem = std::make_shared<CollisionSystem>();
	healthBarRenderSystem = std::make_shared<HealthBarRenderSystem>();
    auto worldEntityObj = MapGenerator::CreateProceduralWorld(*registry, 250, 250);

//    EntityObject player = registry->CreateEntityObject();
//    player.AddComponent<TagComponent>({"Player"});
//    player.AddComponent<TransformComponent>({ sf::Vector2f(100, 100), sf::Vector2f(1, 1), 0.0f });
//    player.AddComponent<CircleComponent>({30.0f, sf::Color::Cyan, true});
//    player.AddComponent<PlayerComponent>({ 300.0f });

    // éÊìæ
//    auto& transform = player.GetComponent<TransformComponent>();
//    transform.position.x += 10.0f;

    float spawnX = 100.0f;
    float spawnY = 100.0f;

    if (worldEntityObj) {
        spdlog::info("World created");
        auto& map = worldEntityObj.GetComponent<MapComponent>();
        bool foundStart = false;
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                if (map.GetTile(x, y) == TileType::Wood) {
                    spawnX = x * map.tileSize;
                    spawnY = y * map.tileSize;
                    foundStart = true;
                    break;
                }
            }
            if (foundStart) break;
        }
        // ï‡ÇØÇÈà íu
        std::vector<sf::Vector2f> freeSlots = MapGenerator::GetWalkablePositions(map);
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(freeSlots.begin(), freeSlots.end(), g);
        int enemyCount = 250; // ê∂ê¨ÇµÇΩÇ¢ìGÇÃêî
        for (int i = 0; i < enemyCount && i < freeSlots.size(); ++i) {
            EntitySpawner::CreateEnemy(*registry, freeSlots[i]);
        }
        spdlog::info("Spawned {} enemies using EntitySpawner.", std::min((size_t)enemyCount, freeSlots.size()));
    }

    auto player = EntitySpawner::CreatePlayer(*registry, spawnX, spawnY);
    if (player) {
        playerEntity = player.GetID();
        spdlog::info("Player created with ID: {}", player.GetID());
    }

//	auto world = MapGenerator::CreateTestWorld(*registry);
	//auto world = MapGenerator::CreateProceduralWorld(*registry);
 //   if (world) {
 //       spdlog::info("world created");
 //   }
//	auto monument = EntitySpawner::CreateMonument(*registry, sf::Vector2f(600.f, 800.f), 1);
}

void GameScene::Update() {
    auto dt = Time::Instance().GetDeltaTime();
    // ì¸óÕ
    inputSystem->Update(*registry, dt);
    // Ç∑Ç´ÇÈ
	skillSystem->Update(*registry, dt);
    // ÉXÉpÅ[ÉN
    sparkVisualSystem->Update(*registry);
	projectileSystem->Update(*registry, dt);
    // ìñÇΩÇËîªíË
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
    // ìG
//    enemySpawnSystem->Update(*registry, dt, playerPos);
	enemyAISystem->Update(*registry, dt, playerPos);
    // à⁄ìÆ
    movementSystem->Update(*registry, dt);
    // ï®óùââéZ
    physicsSystem->Update(*registry, dt);
	// è’ìÀèàóù
	collisionSystem->Update(*registry);

    // ÉQÅ[ÉÄÇÃèIóπämîF
    if (registry->HasComponent<CharacterStatsComponent>(playerEntity)) {
        auto& state = registry->GetComponent<CharacterStatsComponent>(playerEntity);
        if (state.currentHP <= 0) {
			spdlog::info("Player has died. Ending game.");
			SceneManager::Instance().ChangeScene("ResultScene");
            return;
        }
    }
    // ìGéÄÇÒÇæÇ©
    auto enemyView = registry->View<CharacterStatsComponent>();
    bool anyEnemyAlive = false;

    for (auto entity : enemyView) {
        if (!registry->HasComponent<PlayerTag>(entity)) {
            anyEnemyAlive = true;
            break;
        }
    }
    if (!anyEnemyAlive) {
        spdlog::info("All enemies defeated! Victory!");
        SceneManager::Instance().ChangeScene("ResultScene");
    }
}

void GameScene::Render(sf::RenderTarget& target) {
    target.setView(CameraManager::Instance().GetCurrentView());
	// É}ÉbÉvï`âÊ
	mapRenderSystem->Render(*registry, target);
    renderSystem->Render(*registry, target);
    if (DebugManager::Instance().IsDebugMode()) {
        renderSystem->RenderDebug(*registry, target);
    }
    sparkRenderSystem->Render(*registry, target);
	healthBarRenderSystem->Render(*registry, target);

    target.setView(target.getDefaultView());
	uiSystem->Render(*registry, target);

	// âÊñ è„ïîÇ…écÇËìGêîï\é¶
    int aliveEnemies = 0;
    auto enemyView = registry->View<CharacterStatsComponent>();
    for (auto entity : enemyView) {
        if (!registry->HasComponent<PlayerTag>(entity)) {
            aliveEnemies++;
        }
    }
    std::shared_ptr<sf::Font> m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    if (m_font) {
        sf::Text enemyText(*m_font, "Remaining Enemies: " + std::to_string(aliveEnemies), 20);
        enemyText.setFillColor(sf::Color::White);
        enemyText.setOutlineColor(sf::Color::Black);
        enemyText.setOutlineThickness(2.0f);
        sf::FloatRect textBounds = enemyText.getLocalBounds();
        enemyText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.0f, 0.0f });
        enemyText.setPosition({ target.getSize().x / 2.0f, 10.0f });
        target.draw(enemyText);
    }
    target.setView(target.getDefaultView());
}

void GameScene::RenderImGui(const sf::Texture* renderTexture)
{
    DebugGui::Begin("Game Tools", "ÉQÅ[ÉÄÉcÅ[Éã");

    ImGui::SameLine();

    if (DebugGui::Button("Spawn Enemy", "ìGê∂ê¨")) {
        auto e = EntitySpawner::CreateEnemy(*registry, { 400, 300 });
        editorSystem->SetSelectedEntity(e.GetID());
    }

    DebugGui::End();

    editorSystem->RenderImGui(*registry);
}