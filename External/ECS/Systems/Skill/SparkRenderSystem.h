#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include "../../Registry/Registry.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/PlayerSkill/SparkVisual.h"

class SparkRenderSystem {
public:
    // Deterministic 0..1 noise from a float key (classic GLSL-style hash) -- used instead of
    // an RNG so a bolt's jag shape is stable within a single draw call but still reshuffles
    // frame-to-frame as its inputs (position/progress) change, giving a crackling flicker
    // without any per-entity RNG state to store.
    static float Hash(float x) {
        float v = std::sin(x * 12.9898f) * 43758.5453f;
        return v - std::floor(v);
    }
    static float Noise(float x) { return Hash(x) * 2.0f - 1.0f; }

    void Render(Registry& registry, sf::RenderTarget& target) {
        auto view = registry.View<SparkVisualComponent>();
        for (auto entity : view) {
            if (!registry.HasComponent<TransformComponent>(entity)) continue;
            auto& transform = registry.GetComponent<TransformComponent>(entity);
            auto& spark = registry.GetComponent<SparkVisualComponent>(entity);

            // 寿命の取得
            float alphaRatio = 1.0f;
            if (registry.HasComponent<ProjectileComponent>(entity)) {
                auto& proj = registry.GetComponent<ProjectileComponent>(entity);
                if (spark.maxDuration > 0) {
                    alphaRatio = proj.duration / spark.maxDuration;
                    if (alphaRatio < 0) alphaRatio = 0;
                }
            }

            sf::Color currentColor = spark.color;
            currentColor.a = static_cast<std::uint8_t>(255 * alphaRatio);

            if (spark.style == VisualStyle::Explosion) {
                float radius = 20.0f;

                if (registry.HasComponent<BoxColliderComponent>(entity)) {
                    // 幅の半分を半径にする (直径 = width)
                    radius = registry.GetComponent<BoxColliderComponent>(entity).width / 2.0f;
                }

                sf::CircleShape circle(radius);
                circle.setOrigin({ radius, radius });
                circle.setPosition(transform.position);
                circle.setFillColor(currentColor);

                float progress = 1.0f - alphaRatio;

                // ★修正: 0.1倍から始まり、最大でも 1.0倍(Colliderと同じ大きさ) で止める
                float scale = 0.1f + progress * 0.9f;

                circle.setScale({ scale, scale });

                target.draw(circle);

                if (spark.electric) {
                    // Crackling arcs radiating outward from the flash -- makes the impact
                    // read as an electric discharge rather than a plain colored puff.
                    int arcCount = 8;
                    float maxLen = radius * (1.6f + progress * 1.2f);
                    sf::Color arcColor(255, 255, 255, currentColor.a);
                    for (int a = 0; a < arcCount; ++a) {
                        float baseAngle = (6.2831853f / arcCount) * a
                            + Noise(spark.seed + a * 3.1f + progress * 5.0f) * 0.6f;
                        sf::Vector2f dir(std::cos(baseAngle), std::sin(baseAngle));
                        sf::Vector2f normal(-dir.y, dir.x);

                        sf::Vector2f p0 = transform.position;
                        sf::Vector2f p1 = transform.position + dir * (maxLen * 0.45f)
                            + normal * (Noise(spark.seed + a * 7.7f + progress * 9.0f) * radius * 0.35f);
                        sf::Vector2f p2 = transform.position + dir * maxLen;

                        sf::Vertex arc[3]{ sf::Vertex(p0), sf::Vertex(p1), sf::Vertex(p2) };
                        for (auto& v : arc) v.color = arcColor;
                        target.draw(arc, 3, sf::PrimitiveType::LineStrip);
                    }
                }
            }
            else {
                if (spark.trailHistory.empty()) continue;

                sf::Vector2f start = spark.trailHistory[0];
                sf::Vector2f end = transform.position;
                sf::Vector2f diff = end - start;
                float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                if (length < 1.0f) continue;
                sf::Vector2f dir = diff / length;
                sf::Vector2f normal(-dir.y, dir.x);

                if (!spark.electric) {
                    // 太さの半分
                    float halfThick = spark.thickness * 0.5f;
                    sf::Vector2f offset = normal * halfThick;
                    sf::Vertex vertices[4];
                    // 始点側
                    vertices[0].position = start + offset;
                    vertices[1].position = start - offset;
                    // 終点側
                    vertices[2].position = end - offset;
                    vertices[3].position = end + offset;
                    // 色設定
                    for (int i = 0; i < 4; ++i) vertices[i].color = currentColor;

                    target.draw(vertices, 4, sf::PrimitiveType::TriangleStrip);
                    continue;
                }

                // Jagged bolt path: a handful of points offset perpendicular to the
                // start->end line by deterministic noise, so it reads as a lightning
                // bolt instead of a straight beam. Recomputed every frame from the
                // (moving) position + seed, which is what makes it crackle/flicker.
                const int segments = 6;
                sf::Vector2f points[segments + 1];
                for (int i = 0; i <= segments; ++i) {
                    float t = static_cast<float>(i) / segments;
                    sf::Vector2f base = start + diff * t;
                    if (i != 0 && i != segments) {
                        float edgeFalloff = std::sin(t * 3.14159265f); // 0 at ends, 1 mid
                        float n = Noise(spark.seed + i * 13.7f + start.x * 0.017f + start.y * 0.013f);
                        base += normal * (n * spark.thickness * 3.0f * edgeFalloff);
                    }
                    points[i] = base;
                }

                float halfThick = spark.thickness * 0.5f;
                sf::VertexArray glow(sf::PrimitiveType::TriangleStrip, (segments + 1) * 2);
                for (int i = 0; i <= segments; ++i) {
                    sf::Vector2f segDir = (i < segments) ? (points[i + 1] - points[i]) : (points[i] - points[i - 1]);
                    float segLen = std::sqrt(segDir.x * segDir.x + segDir.y * segDir.y);
                    sf::Vector2f segNormal = segLen > 0.0001f
                        ? sf::Vector2f(-segDir.y / segLen, segDir.x / segLen)
                        : normal;
                    glow[i * 2].position = points[i] + segNormal * halfThick;
                    glow[i * 2 + 1].position = points[i] - segNormal * halfThick;
                    glow[i * 2].color = currentColor;
                    glow[i * 2 + 1].color = currentColor;
                }
                target.draw(glow);

                // Bright white core on top of the colored glow for the classic
                // hot-white-center look of an electric arc.
                sf::VertexArray core(sf::PrimitiveType::LineStrip, segments + 1);
                sf::Color coreColor(255, 255, 255, currentColor.a);
                for (int i = 0; i <= segments; ++i) {
                    core[i].position = points[i];
                    core[i].color = coreColor;
                }
                target.draw(core);

                // One short branch fork off the middle of the bolt.
                int forkFrom = segments / 2;
                float forkAngle = Noise(spark.seed + 42.0f) * 1.0f;
                sf::Vector2f forkDir(
                    dir.x * std::cos(forkAngle) - dir.y * std::sin(forkAngle),
                    dir.x * std::sin(forkAngle) + dir.y * std::cos(forkAngle)
                );
                sf::Vector2f forkEnd = points[forkFrom] + forkDir * (length * 0.25f);
                sf::Vertex fork[2]{ sf::Vertex(points[forkFrom]), sf::Vertex(forkEnd) };
                fork[0].color = coreColor;
                fork[1].color = sf::Color(currentColor.r, currentColor.g, currentColor.b, 0);
                target.draw(fork, 2, sf::PrimitiveType::Lines);
            }
        }
    }
};