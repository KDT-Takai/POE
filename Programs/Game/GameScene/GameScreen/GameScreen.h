#pragma once
#include "../../../System/ScreenManager/ScreenBase.h"
#include <string>
#include "../../ECS/EditerSystem.h"
#include "../../ECS/RenderSystem.h"

class GameScreen : public ScreenBase {
public:
    static const char* GetName() { return "GameScreen"; }

    GameScreen();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:
    // System
    std::shared_ptr<EditorSystem> editorSystem;
    std::shared_ptr<RenderSystem> renderSystem;
    // ÉQÅ[ÉÄì‡Ç≈égópÇ∑ÇÈ
    std::shared_ptr<Entity> playerEntity;

};