#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include <System/Input/InputManager.h>

class InputSystem {
public:
    void Update(Registry& registry) {
        // InputManagerからキー入力を取得
        auto& keyInput = InputManager::Instance().GetKeyInput();
        // 後でパッド入力も実装したいから先に書いておく
        auto& padInput = InputManager::Instance().GetPadInput();

        auto view = registry.View<PlayerInputComponent>();

        for (auto entity : view) {
            auto& input = registry.GetComponent<PlayerInputComponent>(entity);

            // フラグをリセット
            input.moveLeft = false;
            input.moveRight = false;
            input.jump = false;
            input.attack = false;
            input.skillInputs.fill(false);

            // 移動
            bool moveInput = false;
            moveInput = keyInput.GetKey(sf::Keyboard::Key::A) || keyInput.GetKey(sf::Keyboard::Key::Left);
            if (moveInput) {
                input.moveLeft = true;
            }
            if (keyInput.GetKey(sf::Keyboard::Key::D) || keyInput.GetKey(sf::Keyboard::Key::Right)) {
                input.moveRight = true;
            }

            // ジャンプ
            bool jumpInput = false;
            jumpInput = keyInput.IsGetKey(sf::Keyboard::Key::Space);
            if (jumpInput) {
                input.jump = true;
            }

            // 攻撃
            bool attackInput = false;
            attackInput = keyInput.IsGetKey(sf::Keyboard::Key::Z);
            if (attackInput) {
                input.attack = true;
            }

            // スキル 1~5
            bool skillInput[MAX_SKILL_SLOTS] = {false, false, false, false, false};
            skillInput[0] = keyInput.IsGetKey(sf::Keyboard::Key::Num1);
            if (skillInput[0]) {
                input.skillInputs[0] = true;
            }
            skillInput[1] = keyInput.IsGetKey(sf::Keyboard::Key::Num2);
            if (skillInput[1]) {
                input.skillInputs[1] = true;
            }
            skillInput[2] = keyInput.IsGetKey(sf::Keyboard::Key::Num3);
            if (skillInput[2]) {
                input.skillInputs[2] = true;
            }
            skillInput[3] = keyInput.IsGetKey(sf::Keyboard::Key::Num4);
            if (skillInput[3]) {
                input.skillInputs[3] = true;
            }
            skillInput[4] = keyInput.IsGetKey(sf::Keyboard::Key::Num5);
            if (skillInput[4]) {
                input.skillInputs[4] = true;
            }
        }
    }
};