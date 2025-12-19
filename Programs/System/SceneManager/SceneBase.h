#pragma once
#include <iostream>
#include <memory>
#include <SFML/Graphics.hpp>
#include <ECS.h>

class SceneBase {
protected:

    std::string sceneName;
    std::unique_ptr<Registry> registry;

public:

    SceneBase() {
        registry = std::make_unique<Registry>();
    };
    virtual ~SceneBase() = default;

    virtual void Update() = 0;
    virtual void Render(sf::RenderTarget& target) = 0;
    virtual void RenderImGui(const sf::Texture* renderTexture) = 0;
};