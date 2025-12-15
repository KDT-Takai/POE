#pragma once
#include <ECS.h>
#include <System/Input/InputManager.h>
#include <SFML/Graphics.hpp>
#include <cmath>

class PlayerSystem {
public:
    void Update(Registry& registry, float dt) {
        auto entities = registry.View<PlayerComponent>();
        for (auto entity : entities) {
            if (registry.HasComponent<TransformComponent>(entity)) {
                auto& player = registry.GetComponent<PlayerComponent>(entity);
                auto& transform = registry.GetComponent<TransformComponent>(entity);
                sf::Vector2f movement(0.f, 0.f);
                if (InputManager::Instance().GetKeyInput().GetKey(sf::Keyboard::Key::W)) {
                    movement.y -= 1.f;
                }
                if (InputManager::Instance().GetKeyInput().GetKey(sf::Keyboard::Key::S)) {
                    movement.y += 1.f;
                }
                if (InputManager::Instance().GetKeyInput().GetKey(sf::Keyboard::Key::A)) {
                    movement.x -= 1.f;
                }
                if (InputManager::Instance().GetKeyInput().GetKey(sf::Keyboard::Key::D)) {
                    movement.x += 1.f;
                }
				// ê≥ãKâªÇµÇƒë¨ìxÇìKópÇ∑ÇÈÇ≈
                if (movement.x != 0 || movement.y != 0) {
                    float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
                    movement /= length;
                    transform.position += movement * player.speed * dt;
                }
            }
        }
    }
};