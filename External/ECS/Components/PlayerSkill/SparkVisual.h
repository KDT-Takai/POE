#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <deque>
enum class VisualStyle {
    Line,
    Explosion
};

struct SparkVisualComponent {
    std::vector<sf::Vector2f> trailHistory;

    size_t maxTrailLength = 10;

    float thickness = 2.0f;             // 太さ
    VisualStyle style = VisualStyle::Line; // 描画タイプ
    float maxDuration = 1.0f;           // フェードアウト用
    sf::Color color = sf::Color(200, 255, 255); // 色
};