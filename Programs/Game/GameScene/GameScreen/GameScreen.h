#pragma once
#include <string>
#include <ECS.h>
#include <System/ScreenManager/ScreenBase.h>
#include "../Character/Plyaer/System/PlayerMoveSystem.h"

class GameScreen : public ScreenBase {
public:
    static const char* GetName() { return "GameScreen"; }

    GameScreen();

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