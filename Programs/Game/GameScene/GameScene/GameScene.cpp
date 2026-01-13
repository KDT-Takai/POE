#include "GameScene.h"
#include <ECS.h>
#include <System/CameraManager/CameraManager.h>
#include <System/Time/Time.h>
#include <System/DebugGui/DebugGui.h>

#include "../../../System/DebugManager/DebugManager.h"
#include "../Entity/EntitySpawner.h"
#include "../MapGenerator/MapGenerator.h"

GameScene::GameScene() {
    sceneName = "GameScreen";

    registry = std::make_unique<Registry>();

    editorSystem = std::make_shared<EditorSystem>();
    renderSystem = std::make_shared<RenderSystem>();
	
    inputSystem = std::make_shared<InputSystem>();
    movementSystem = std::make_shared<MovementSystem>();
    physicsSystem = std::make_shared<PhysicsSystem>();
    mapRenderSystem = std::make_shared<MapRenderSystem>();

//    EntityObject player = registry->CreateEntityObject();
//    player.AddComponent<TagComponent>({"Player"});
//    player.AddComponent<TransformComponent>({ sf::Vector2f(100, 100), sf::Vector2f(1, 1), 0.0f });
//    player.AddComponent<CircleComponent>({30.0f, sf::Color::Cyan, true});
//    player.AddComponent<PlayerComponent>({ 300.0f });

    // Žæ“¾
//    auto& transform = player.GetComponent<TransformComponent>();
//    transform.position.x += 10.0f;

    auto player = EntitySpawner::CreatePlayer(*registry, 100.0f, 300.0f);
    if (player) {
        playerEntity = player.GetID();
        spdlog::info("Player created with ID: {}", player.GetID());
    }
//	auto world = MapGenerator::CreateTestWorld(*registry);
	auto world = MapGenerator::CreateProceduralWorld(*registry);
    if (world) {
        spdlog::info("world created");
    }
	auto monument = EntitySpawner::CreateMonument(*registry, sf::Vector2f(600.f, 800.f), 1);
}

void GameScene::Update() {
    auto dt = Time::Instance().GetDeltaTime();
    // “ü—Í
    inputSystem->Update(*registry);
    // ˆÚ“®
    movementSystem->Update(*registry, dt);
    // •¨—‰‰ŽZ
    physicsSystem->Update(*registry, dt);
    // “–‚½‚è”»’è
    if (registry->IsValid(playerEntity)) {
        auto& trans = registry->GetComponent<TransformComponent>(playerEntity);
        sf::Vector2f centerPos = trans.position + sf::Vector2f(16.0f, 32.0f);
        CameraManager::Instance().SetCenter(centerPos);
    }
}

void GameScene::Render(sf::RenderTarget& target) {
    target.setView(CameraManager::Instance().GetCurrentView());
	// ƒ}ƒbƒv•`‰æ
	mapRenderSystem->Render(*registry, target);
    renderSystem->Render(*registry, target);
    if (DebugManager::Instance().IsDebugMode()) {
        renderSystem->RenderDebug(*registry, target);
    }
    target.setView(target.getDefaultView());
}

void GameScene::RenderImGui(const sf::Texture* renderTexture)
{
    DebugGui::Begin("Game Tools", "ƒQ[ƒ€ƒc[ƒ‹");

    ImGui::SameLine();

    if (DebugGui::Button("Spawn Enemy", "“G¶¬")) {
        auto e = EntitySpawner::CreateEnemy(*registry, { 400, 300 });
        editorSystem->SetSelectedEntity(e.GetID());
    }

    DebugGui::End();

    editorSystem->RenderImGui(*registry);
}