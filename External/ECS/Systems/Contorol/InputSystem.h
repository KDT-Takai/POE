#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include <System/Input/InputManager.h>

class InputSystem {
public:
    void Update(Registry& registry) {
        // InputManagerからキー入力を取得
        auto& keyInput = InputManager::Instance().GetKeyInput();

        auto view = registry.View<PlayerInputComponent>();

        for (auto entity : view) {
            auto& input = registry.GetComponent<PlayerInputComponent>(entity);

            // フラグをリセット
            input.moveLeft = false;
            input.moveRight = false;
            input.jump = false;
            input.attack = false;

            // --- 移動 (押しっぱなし = GetKey) ---
            if (keyInput.GetKey(sf::Keyboard::Key::A) || keyInput.GetKey(sf::Keyboard::Key::Left)) {
                input.moveLeft = true;
            }
            if (keyInput.GetKey(sf::Keyboard::Key::D) || keyInput.GetKey(sf::Keyboard::Key::Right)) {
                input.moveRight = true;
            }

            // --- ジャンプ (押した瞬間 = IsGetKey) ---
            // ※IsGetKeyを使うことで、押しっぱなしでの連打（空中浮遊）を防ぎます
            if (keyInput.IsGetKey(sf::Keyboard::Key::Space) || keyInput.IsGetKey(sf::Keyboard::Key::W)) {
                input.jump = true;
            }

            // --- 攻撃など ---
            if (keyInput.IsGetKey(sf::Keyboard::Key::Z)) {
                input.attack = true;
            }
        }
    }
};