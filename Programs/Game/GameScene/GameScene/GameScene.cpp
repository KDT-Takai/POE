#include "GameScene.h"
#include <ECS.h>
#include "../Entity/EntitySpawner.h"
#include <System/CameraManager/CameraManager.h>
#include <System/Time/Time.h>
#include <System/DebugGui/DebugGui.h>

GameScene::GameScene() {
    sceneName = "GameScreen";

    registry = std::make_unique<Registry>();

    editorSystem = std::make_shared<EditorSystem>();
    renderSystem = std::make_shared<RenderSystem>();
	playerMoveSystem = std::make_shared<PlayerMoveSystem>();

    EntityObject player = registry->CreateEntityObject();
    player.AddComponent<TagComponent>({"Player"});
    player.AddComponent<TransformComponent>({ sf::Vector2f(100, 100), sf::Vector2f(1, 1), 0.0f });
    player.AddComponent<CircleComponent>({30.0f, sf::Color::Cyan, true});
    player.AddComponent<PlayerComponent>({ 300.0f });

    // Žæ“¾‚àŠÈ’P
    auto& transform = player.GetComponent<TransformComponent>();
    transform.position.x += 10.0f;
}

void GameScene::Update() {
	playerMoveSystem->Update(*registry,Time::Instance().GetDeltaTime());
}

void GameScene::Render(sf::RenderTarget& target) {
    target.setView(CameraManager::Instance().GetCurrentView());
    renderSystem->Render(*registry, target);
    target.setView(target.getDefaultView());
}

void GameScene::RenderImGui(const sf::Texture* renderTexture)
{
    DebugGui::Begin("Game Tools###GameTools", "ƒQ[ƒ€ƒc[ƒ‹###GameTools");

    ImGui::SameLine();

    if (DebugGui::Button("Spawn Enemy", "“G¶¬")) {
        auto e = EntitySpawner::CreateEnemy(*registry, { 400, 300 });
        editorSystem->SetSelectedEntity(e.GetID());
    }

    DebugGui::End();
    editorSystem->RenderImGui(*registry);
}