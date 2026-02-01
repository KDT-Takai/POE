#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "../../Components/Physics/Velocity/Velocity.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Physics/Facing/Facing.h"
#include "../../Components/Physics/BoxCollider/BoxCollider.h"
#include "../../Components/Control/State/State.h"

class MovementSystem {
public:
    void Update(Registry& registry, float dt) {
        auto view = registry.View<PlayerInputComponent>();

        for (auto entity : view) {
            // 必要なコンポーネントチェック
            if (!registry.HasComponent<VelocityComponent>(entity) ||
                !registry.HasComponent<CharacterStatsComponent>(entity)) {
                continue;
            }
           
            auto& state = registry.GetComponent<StateComponent>(entity);
            if (state.currentState == ActorState::Roll || state.currentState == ActorState::Attack) {
                continue;
            }

            auto& input = registry.GetComponent<PlayerInputComponent>(entity);
            auto& velocity = registry.GetComponent<VelocityComponent>(entity);
            auto& charStats = registry.GetComponent<CharacterStatsComponent>(entity);


            // 左右移動
            // X速度をリセット
            velocity.velocity.x = 0.0f;

            if (input.moveLeft) {
                velocity.velocity.x = -charStats.moveSpeed;

                if (registry.HasComponent<FacingComponent>(entity)) {
                    registry.GetComponent<FacingComponent>(entity).direction = -1;
                }
            }
            else if (input.moveRight) {
                velocity.velocity.x = charStats.moveSpeed;

                if (registry.HasComponent<FacingComponent>(entity)) {
                    registry.GetComponent<FacingComponent>(entity).direction = 1;
                }
            }

            // ジャンプ処理
            if (input.jump) {
                if (registry.HasComponent<BoxColliderComponent>(entity)) {
                    auto& col = registry.GetComponent<BoxColliderComponent>(entity);
                    if (col.isGrounded) {
                        velocity.velocity.y = -500.0f; // ジャンプ力
                        col.isGrounded = false;
                    }
                }
            }
        }
    }
};