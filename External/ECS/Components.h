#pragma once
#include <SFML/Graphics.hpp>
#include <string>

// 座標・回転・スケール
struct TransformComponent {
    sf::Vector2f position = { 0.f, 0.f };
    sf::Vector2f scale = { 1.f, 1.f };
    float rotation = 0.f;
};

// 描画情報
struct SpriteComponent {
    std::string textureName = ""; // リソースマネージャーのキー
    sf::Color color = sf::Color::White;
    sf::IntRect textureRect{ {0, 0}, {0, 0} };
    int layer = 0; // 描画レイヤー (数値が大きいほど手前)
    bool isVisible = true;
};

// 名前
struct TagComponent {
    std::string name = "Entity";
};

// 円
struct CircleComponent {
    float radius = 20.0f;
    sf::Color color = sf::Color::Green;
    bool isVisible = true;
};