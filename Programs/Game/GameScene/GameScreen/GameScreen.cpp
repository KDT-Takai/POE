#include "GameScreen.h"
#include <ECS.h>
#include "../Entity/EntitySpawner.h"
#include <System/CameraManager/CameraManager.h>
#include <System/Time/Time.h>

GameScreen::GameScreen() {
    screenName = "GameScreen";

    registry = std::make_unique<Registry>();

    editorSystem = std::make_shared<EditorSystem>();
    renderSystem = std::make_shared<RenderSystem>();
	playerSystem = std::make_shared<PlayerSystem>();

    EntityObject player = registry->CreateEntityObject();
    player.AddComponent<TagComponent>({"Player"});
    player.AddComponent<TransformComponent>({ sf::Vector2f(100, 100), sf::Vector2f(1, 1), 0.0f });
    player.AddComponent<CircleComponent>({30.0f, sf::Color::Cyan, true});
    player.AddComponent<PlayerComponent>({ 300.0f });

    // Žæ“¾‚àŠÈ’P
    auto& transform = player.GetComponent<TransformComponent>();
    transform.position.x += 10.0f;
}

void GameScreen::Update() {
	playerSystem->Update(*registry,Time::Instance().GetDeltaTime());
}

void GameScreen::Render(sf::RenderTarget& target) {
    target.setView(CameraManager::Instance().GetCurrentView());
    renderSystem->Render(*registry, target);
    target.setView(target.getDefaultView());
}

void GameScreen::RenderImGui(const sf::Texture* renderTexture)
{
    ImGui::Begin("Game Tools");

    ImGui::SameLine();

    if (ImGui::Button("Spawn Enemy")) {
        auto e = EntitySpawner::CreateEnemy(*registry, { 400, 300 });
        editorSystem->SetSelectedEntity(e.GetID());
    }

    ImGui::Text("Current Scene: Game Screen");
    ImGui::End();

    editorSystem->RenderImGui(*registry);
}