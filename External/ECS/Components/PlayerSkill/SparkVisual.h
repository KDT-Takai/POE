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

    float thickness = 2.0f;             // ����
    VisualStyle style = VisualStyle::Line; // �`��^�C�v
    float maxDuration = 1.0f;           // �t�F�[�h�A�E�g�p
    sf::Color color = sf::Color(200, 255, 255); // �F
    float explosionRadius = 150.0f;

    bool electric = false;   // jagged bolt + crackling arcs instead of a plain line/circle
    float seed = 0.0f;       // per-instance jitter offset so multiple bolts don't jag identically
};