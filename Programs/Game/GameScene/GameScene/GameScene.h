#pragma once
#include <string>
#include <ECS.h>
#include <System/SceneManager/SceneBase.h>
// ‰¼
#include "../../ECS/Systems/Contorol/InputSystem.h"
#include "../../ECS/Systems/Physics/MovementSystem.h"
#include "../../ECS/Systems/Physics/PhysicsSystem.h"
#include "../../ECS/Systems/World/MapRenderSystem.h"

class GameScene : public SceneBase {
public:
    static const char* GetName() { return "GameScene"; }

    GameScene();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:
    Entity playerEntity = -1;
    
    // Registry
	std::unique_ptr<Registry> registry;
    
    // System
    std::shared_ptr<EditorSystem> editorSystem;
    std::shared_ptr<RenderSystem> renderSystem;

    std::shared_ptr<InputSystem> inputSystem;
    std::shared_ptr<MovementSystem> movementSystem;
    std::shared_ptr<PhysicsSystem> physicsSystem;
    std::shared_ptr<MapRenderSystem> mapRenderSystem;

};