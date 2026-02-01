#pragma once
#include <SFML/Graphics.hpp>

// 座標・回転・スケール
struct TransformComponent {
    sf::Vector2f position = { 0.f, 0.f };
    sf::Vector2f scale = { 1.f, 1.f };
    float rotation = 0.f;
};