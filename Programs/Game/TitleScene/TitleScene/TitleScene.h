#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <System/SceneManager/SceneBase.h>

class TitleScene : public SceneBase {
public:
    static const char* GetName() { return "TitleScene"; }

    TitleScene();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:
    sf::CircleShape movingCircle;
    sf::Vector2f circleVelocity;

	std::unique_ptr<sf::Sprite> testSprite;
};