#pragma once
#include <iostream>
#include <memory>
#include <SFML/Graphics.hpp>
#include "../../ECS/ECS.h"
#include "../../ECS/Registry.h"

class ScreenBase {
protected:

    std::string screenName;
    std::unique_ptr<Registry> registry;

public:

    ScreenBase() {
        registry = std::make_unique<Registry>();
    };
    virtual ~ScreenBase() = default;

    virtual void Update() = 0;
    virtual void Render(sf::RenderTarget& target) = 0;
    virtual void RenderImGui(const sf::Texture* renderTexture) = 0;
};