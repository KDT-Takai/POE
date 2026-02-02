#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "../../Components/PlayerSkill/PlayerSkill.h" 
#include <spdlog/spdlog.h>
#include <System/Input/InputManager.h>
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Control/State/State.h"
#include "../../Components/Physics/Velocity/Velocity.h"
#include <cmath> // sqrt用

class InputSystem {
public:
    void Update(Registry& registry, float dt) {
        auto& keyInput = InputManager::Instance().GetKeyInput();
		auto& mouseInput = InputManager::Instance().GetMouseInput();
        auto view = registry.View<PlayerInputComponent, CharacterStatsComponent, StateComponent, VelocityComponent, TransformComponent>();

        for (auto entity : view) {
            auto& input = registry.GetComponent<PlayerInputComponent>(entity);
            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);
            auto& state = registry.GetComponent<StateComponent>(entity);
            auto& velocity = registry.GetComponent<VelocityComponent>(entity);
            auto& trans = registry.GetComponent<TransformComponent>(entity);

            if (stats.rollCooldownTimer > 0.0f) {
                stats.rollCooldownTimer -= dt;
            }

            if (state.currentState != ActorState::Idle && state.currentState != ActorState::Run) {
                state.stateTimer += dt;
            }

            if (state.currentState == ActorState::Roll) {
                if (state.stateTimer >= stats.rollDuration) {
                    state.currentState = ActorState::Idle;
                    state.stateTimer = 0.0f;
                    velocity.velocity = { 0.0f, 0.0f }; // 停止させる
                }
                else {
                    continue; // ロール中は操作を受け付けない
                }
            }

            if (state.currentState == ActorState::Attack) {
                continue;
            }

            input.moveLeft = false;
            input.moveRight = false;
            input.jump = false;
            input.attack = false;
            input.skillInputs.fill(false);

            velocity.velocity.x = 0.0f;
            velocity.velocity.y = 0.0f;

            bool rollInput = keyInput.IsGetKey(sf::Keyboard::Key::Space);

            if (rollInput && stats.rollCooldownTimer <= 0.0f) {

                state.currentState = ActorState::Roll;
                state.stateTimer = 0.0f;
                stats.rollCooldownTimer = stats.rollCooldownMax; 
                sf::Vector2f inputDir(0.0f, 0.0f);
                if (keyInput.GetKey(sf::Keyboard::Key::A) || keyInput.GetKey(sf::Keyboard::Key::Left))  inputDir.x -= 1.0f;
                if (keyInput.GetKey(sf::Keyboard::Key::D) || keyInput.GetKey(sf::Keyboard::Key::Right)) inputDir.x += 1.0f;
                if (keyInput.GetKey(sf::Keyboard::Key::W) || keyInput.GetKey(sf::Keyboard::Key::Up))    inputDir.y -= 1.0f;
                if (keyInput.GetKey(sf::Keyboard::Key::S) || keyInput.GetKey(sf::Keyboard::Key::Down))  inputDir.y += 1.0f;

                if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
                    float length = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
                    inputDir /= length; // 正規化
                    velocity.velocity = inputDir * stats.rollSpeed;
                    // 向きの更新
                    if (inputDir.x > 0) state.isFacingRight = true;
                    if (inputDir.x < 0) state.isFacingRight = false;
                }
                else {
                    float facingDir = state.isFacingRight ? 1.0f : -1.0f;
                    velocity.velocity = sf::Vector2f(facingDir * stats.rollSpeed, 0.0f);
                }
                if (state.isFacingRight) trans.scale.x = std::abs(trans.scale.x);
                else                     trans.scale.x = -std::abs(trans.scale.x);

                continue; // 今フレームはこれで終了
            }
            float moveSpeed = stats.moveSpeed;
            sf::Vector2f moveDir(0.0f, 0.0f); // 移動方向ベクトル

            // X軸入力
            if (keyInput.GetKey(sf::Keyboard::Key::A) || keyInput.GetKey(sf::Keyboard::Key::Left)) {
                moveDir.x -= 1.0f;
                input.moveLeft = true;
            }
            if (keyInput.GetKey(sf::Keyboard::Key::D) || keyInput.GetKey(sf::Keyboard::Key::Right)) {
                moveDir.x += 1.0f;
                input.moveRight = true;
            }

            // Y軸入力
            if (keyInput.GetKey(sf::Keyboard::Key::W)) {
                moveDir.y -= 1.0f;
            }
            if (keyInput.GetKey(sf::Keyboard::Key::S)) {
                moveDir.y += 1.0f;
            }

            if (moveDir.x != 0.0f || moveDir.y != 0.0f) {
                float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
                moveDir /= length;

                velocity.velocity = moveDir * moveSpeed;

                state.currentState = ActorState::Run;

                if (moveDir.x > 0.0f) state.isFacingRight = true;
                if (moveDir.x < 0.0f) state.isFacingRight = false;
            }
            else {
                state.currentState = ActorState::Idle;
                velocity.velocity = { 0.0f, 0.0f };
            }

            // 向き更新
            if (state.isFacingRight) trans.scale.x = std::abs(trans.scale.x);
            else                     trans.scale.x = -std::abs(trans.scale.x);


            //if (keyInput.IsGetKey(sf::Keyboard::Key::E)) {
            //    input.attack = true;
            //}
            if (registry.HasComponent<PlayerSkill>(entity)) {
                if (keyInput.IsGetKey(sf::Keyboard::Key::E) || mouseInput.GetMouse(sf::Mouse::Button::Left)) input.skillInputs[0] = true;
                if (keyInput.IsGetKey(sf::Keyboard::Key::Q)) input.skillInputs[1] = true;
                if (keyInput.IsGetKey(sf::Keyboard::Key::R)) input.skillInputs[2] = true;
                if (keyInput.IsGetKey(sf::Keyboard::Key::V)) input.skillInputs[3] = true;
                if (keyInput.IsGetKey(sf::Keyboard::Key::F)) input.skillInputs[4] = true;
            }
        }
    }
};