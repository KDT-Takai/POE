#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <System/SceneManager/SceneBase.h>

class ResultScene : public SceneBase {
public:
    static const char* GetName() { return "ResultScene"; }

    ResultScene();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:

};