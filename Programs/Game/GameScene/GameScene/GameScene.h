#pragma once
#include <string>
#include <ECS.h>
#include <System/SceneManager/SceneBase.h>
#include "../Character/Plyaer/System/PlayerMoveSystem.h"

class GameScene : public SceneBase {
public:
    static const char* GetName() { return "GameScene"; }

    GameScene();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:
    // Registry
	std::unique_ptr<Registry> registry;
    
    // System
    std::shared_ptr<EditorSystem> editorSystem;
    std::shared_ptr<RenderSystem> renderSystem;

    std::shared_ptr<PlayerMoveSystem> playerMoveSystem;

    // ÉQÅ[ÉÄì‡Ç≈égópÇ∑ÇÈ
    std::shared_ptr<Entity> playerEntity;

};