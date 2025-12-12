#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../../../System/ScreenManager/ScreenBase.h"

class TitleScreen : public ScreenBase {
public:
    static const char* GetName() { return "TitleScreen"; }

    TitleScreen();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:
    sf::CircleShape movingCircle;
    sf::Vector2f circleVelocity;

	std::unique_ptr<sf::Sprite> testSprite;
};