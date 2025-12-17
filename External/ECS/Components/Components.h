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

// プレイヤー制御用
struct PlayerComponent {
    // 基本ステータス
    float moveSpeed = 200.0f;     // 通常時の歩行速度

    // ローリング
    bool isRolling = false;
    sf::Vector2f rollDirection;

    float rollSpeed = 600.0f;
    float rollDuration = 0.4f;
    float currentRollTime = 0.0f;

    float rollCooldown = 0.0f;
    float rollCooldownMax = 0.8f;
};