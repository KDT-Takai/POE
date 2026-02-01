#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Control/PlayerInput/PlayerInput.h"
#include "../../Components/Stats/SkillData/Skill.h"
#include "../../Components/Physics/Velocity/Velocity.h"
#include "../../Components/Physics/Facing/Facing.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/PlayerSkill/PlayerSkill.h"
#include "../../Components/Control/State/State.h"
#include "../../Components/Physics/Projectile/Projectile.h"
#include "../../Components/Physics/BoxCollider/BoxCollider.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/PlayerSkill/SparkVisual.h"
#include "../../../../Programs/System/Input/InputManager.h"
#include <cmath>
#include <random>

class SkillSystem {
public:
    void Update(Registry& registry, float dt) {
        auto view = registry.View<PlayerSkill>();

        for (auto entity : view) {
            // 必要なコンポーネント取得
            if (!registry.HasComponent<PlayerInputComponent>(entity) ||
                !registry.HasComponent<VelocityComponent>(entity) ||
                !registry.HasComponent<StateComponent>(entity)) {
                continue;
            }

            auto& skillComp = registry.GetComponent<PlayerSkill>(entity);
            auto& input = registry.GetComponent<PlayerInputComponent>(entity);
            auto& velocity = registry.GetComponent<VelocityComponent>(entity);
            auto& state = registry.GetComponent<StateComponent>(entity);

            auto& stats = registry.GetComponent<CharacterStatsComponent>(entity);

            // HP
            if (stats.currentHP < stats.maxHP) {
                stats.currentHP += stats.healthRegen * dt;
                if (stats.currentHP > stats.maxHP) stats.currentHP = stats.maxHP;
            }

            // MP回復
            if (stats.currentMP < stats.maxMP) {
                stats.currentMP += stats.manaRegen * dt;
                if (stats.currentMP > stats.maxMP) stats.currentMP = stats.maxMP;
            }

            // 発射位置用にTransform取得
            sf::Vector2f pos(0, 0);
            if (registry.HasComponent<TransformComponent>(entity)) {
                pos = registry.GetComponent<TransformComponent>(entity).position;
            }

            float direction = 1.0f;
            if (registry.HasComponent<FacingComponent>(entity)) {
                direction = (float)registry.GetComponent<FacingComponent>(entity).direction;
            }

            for (auto& skill : skillComp.skills) {
                if (skill.isValid && skill.currentCooldown > 0.0f) {
                    skill.currentCooldown -= dt;
                }
            }

            if (skillComp.castingSkillIndex != -1) {
                skillComp.castTimer -= dt;

                // 持続時間終了
                if (skillComp.castTimer <= 0.0f) {
                    FinishSkill(skillComp, state, velocity);
                }
                else {
                    // 実行中の継続処理
                    UpdateActiveSkill(skillComp, velocity, direction);
                }
            }
            else if (state.currentState == ActorState::Idle || state.currentState == ActorState::Run) {
                for (int i = 0; i < 5; ++i) {
                    if (input.skillInputs[i] && skillComp.skills[i].isValid && skillComp.skills[i].currentCooldown <= 0.0f) {
                        float cost = (float)skillComp.skills[i].mpCost;
                        if (stats.currentMP >= cost) {
                            // MP消費実行
                            stats.currentMP -= cost;
                            // スキル発動
                            sf::Vector2f mousePos = input.mouseWorldPos;
                            ActivateSkill(registry, entity, i, skillComp, state, velocity, direction, pos, mousePos, stats);
                            break;
                        }
                        else {
                            // いつかMP不足のときのために
                        }
                    }
                }
            }
        }
    }

private:
    // 簡易乱数
    float GetRandom(float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    }

    // スキル開始処理
    void ActivateSkill(Registry& reg, unsigned int pcEntity, int index, PlayerSkill& pc, StateComponent& state, VelocityComponent& vel, float dir, sf::Vector2f origin, sf::Vector2f targetPos, const CharacterStatsComponent& stats) {
        auto& skill = pc.skills[index];

        // クールダウン開始
        skill.currentCooldown = skill.cooldownTime;

        // 挙動ごとの処理
        switch (skill.behaviorType) {
        case SkillBehaviorType::Dash:
        {
            // Shadow Rollの開始
            state.currentState = ActorState::Roll;
            pc.castingSkillIndex = index;
            pc.castTimer = skill.duration;

            auto& keyInput = InputManager::Instance().GetKeyInput();
            sf::Vector2f inputDir(0.0f, 0.0f);

            if (keyInput.GetKey(sf::Keyboard::Key::A)) inputDir.x -= 1.0f;
            if (keyInput.GetKey(sf::Keyboard::Key::D)) inputDir.x += 1.0f;
            if (keyInput.GetKey(sf::Keyboard::Key::W)) inputDir.y -= 1.0f;
            if (keyInput.GetKey(sf::Keyboard::Key::S)) inputDir.y += 1.0f;

            float speed = stats.rollSpeed;

            if (inputDir.x != 0.0f || inputDir.y != 0.0f) {
                float length = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
                inputDir /= length;
                vel.velocity = inputDir * speed;
                state.isFacingRight = (inputDir.x > 0);
            }
            else {
                float facingDir = state.isFacingRight ? 1.0f : -1.0f;
                vel.velocity = sf::Vector2f(facingDir * speed, 0.0f);
            }
            break;
        }
        case SkillBehaviorType::Spark:
        {
            sf::Vector2f offset(16.0f, 16.0f);
            sf::Vector2f spawnPos = origin + offset;

            float dx = targetPos.x - spawnPos.x;
            float dy = targetPos.y - spawnPos.y;
            float baseAngle = std::atan2(dy, dx);

            int projectileCount = 7;

            for (int i = 0; i < projectileCount; ++i) {
                auto p = reg.CreateEntity();

                reg.AddComponent<TransformComponent>(p, TransformComponent{ spawnPos, sf::Vector2f(1.0f, 1.0f) });

                float spread = 0.8f;
                float randomOffset = GetRandom(-spread, spread);
                float finalAngle = baseAngle + randomOffset;

                float speed = 500.0f;

                reg.AddComponent<VelocityComponent>(p, VelocityComponent{
                    sf::Vector2f(std::cos(finalAngle) * speed, std::sin(finalAngle) * speed)
                    });

                reg.AddComponent<BoxColliderComponent>(p, BoxColliderComponent{ 10.0f, 10.0f });

                ProjectileComponent proj;
                proj.duration = 3.5f;
                proj.isBouncy = true;
                proj.damage = stats.atk * (skill.damage / 100.0f);
                if (proj.damage == 0) proj.damage = skill.damage;
                reg.AddComponent<ProjectileComponent>(p, proj);
                SparkVisualComponent sparkVis;
                sparkVis.trailHistory.push_back(spawnPos);
                reg.AddComponent<SparkVisualComponent>(p, sparkVis);
            }
            break;
        }
        case SkillBehaviorType::GroundSlam:
        {
            sf::Vector2f centerOffset(16.0f, 16.0f);
            sf::Vector2f spawnPos = origin + centerOffset;

            float dx = targetPos.x - spawnPos.x;
            float dy = targetPos.y - spawnPos.y;
            float baseAngle = std::atan2(dy, dx);

            int projectileCount = 1;
            float angleStep = 0.2f; // 角度の開き具合 (狭めにして指向性を持たせる)
            float startAngle = baseAngle - ((projectileCount - 1) * angleStep / 2.0f);

            for (int i = 0; i < projectileCount; ++i) {
                auto p = reg.CreateEntity();
                reg.AddComponent<TransformComponent>(p, TransformComponent{ spawnPos, sf::Vector2f(1.5f, 1.5f) }); // 弾を少し大きく

                float currentAngle = startAngle + (i * angleStep);
                float speed = 600.0f;

                reg.AddComponent<VelocityComponent>(p, VelocityComponent{sf::Vector2f(std::cos(currentAngle) * speed, std::sin(currentAngle) * speed) });

                reg.AddComponent<BoxColliderComponent>(p, BoxColliderComponent{ 20.0f, 20.0f });

                ProjectileComponent proj;
                proj.duration = 3.0f;
                proj.isBouncy = false;
                proj.damage = stats.atk * (skill.damage / 100.0f);
                reg.AddComponent<ProjectileComponent>(p, proj);

                SparkVisualComponent sparkVis;
                sparkVis.trailHistory.push_back(spawnPos);
                sparkVis.thickness = 3.0f;
                sparkVis.color = sf::Color(255, 255, 150);
                reg.AddComponent<SparkVisualComponent>(p, sparkVis);
            }
            break;
        }
        case SkillBehaviorType::LightningWarp:
        {
            sf::Vector2f centerOffset(16.0f, 16.0f);
            sf::Vector2f currentPos = origin + centerOffset;

            sf::Vector2f diff = targetPos - currentPos;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            // 0除算対策
            if (dist > 0.0001f) {
                if (dist > skill.range) {
                    diff = (diff / dist) * skill.range;
                    targetPos = currentPos + diff;
                }
            }
            else {
                targetPos = currentPos;
            }

            // プレイヤー移動
            if (reg.HasComponent<TransformComponent>(pcEntity)) {
                reg.GetComponent<TransformComponent>(pcEntity).position = targetPos - centerOffset;
            }

            {
                auto visualEntity = reg.CreateEntity();
                reg.AddComponent<TransformComponent>(visualEntity, TransformComponent{ targetPos, sf::Vector2f(1.0f, 1.0f) });

                ProjectileComponent timerProj;
                timerProj.duration = 0.4f; // アニメーション時間
                timerProj.damage = 0;      // ダメージなし
                reg.AddComponent<ProjectileComponent>(visualEntity, timerProj);

                SparkVisualComponent sparkVis;
                sparkVis.style = VisualStyle::Explosion;
                sparkVis.maxDuration = 0.4f;
                sparkVis.color = sf::Color(200, 255, 255, 255);
                reg.AddComponent<SparkVisualComponent>(visualEntity, sparkVis);

                reg.AddComponent<VelocityComponent>(visualEntity, VelocityComponent{ sf::Vector2f(0.f, 0.f) });
            }

            {
                auto damageEntity = reg.CreateEntity();
                reg.AddComponent<TransformComponent>(damageEntity, TransformComponent{ targetPos, sf::Vector2f(1.0f, 1.0f) });

                // ★修正: 見た目が「半径150」なので、判定は「直径300」にします
                float blastSize = 300.0f;
                float offset = -blastSize / 2.0f;

                reg.AddComponent<BoxColliderComponent>(damageEntity, BoxColliderComponent{
                    blastSize, blastSize,
                    offset, offset,
                    false, false
                    });

                ProjectileComponent dmgProj;
                dmgProj.duration = 0.4f;
                dmgProj.damage = stats.atk * (skill.damage / 100.0f);
                dmgProj.isBouncy = true;
                reg.AddComponent<ProjectileComponent>(damageEntity, dmgProj);

                reg.AddComponent<VelocityComponent>(damageEntity, VelocityComponent{ sf::Vector2f(0.f, 0.f) });
            }
            break;
        }
        case SkillBehaviorType::LightningBall:
        {
            sf::Vector2f playerPos = reg.GetComponent<TransformComponent>(pcEntity).position;
            sf::Vector2f centerOffset(16.0f, 16.0f);
            sf::Vector2f spawnPos = playerPos + centerOffset;

            int bulletCount = 12;
            float speed = 100.0f;

            for (int i = 0; i < bulletCount; ++i) {
                float angle = (360.0f / bulletCount) * i;
                float radian = angle * (3.14159f / 180.0f); 

                sf::Vector2f velocity(std::cos(radian) * speed, std::sin(radian) * speed);

                auto ball = reg.CreateEntity();
                reg.AddComponent<TransformComponent>(ball, TransformComponent{ spawnPos, {1.0f, 1.0f} });

                float ballSize = 80.0f;
                reg.AddComponent<BoxColliderComponent>(ball, BoxColliderComponent{ ballSize, ballSize, -ballSize / 2.f, -ballSize / 2.f, false, false});

                ProjectileComponent proj;
                proj.duration = 2.0f;
                proj.damage = stats.atk;
                proj.isBouncy = false;
                reg.AddComponent<ProjectileComponent>(ball, proj);

                // 見た目
                SparkVisualComponent sparkVis;
                sparkVis.style = VisualStyle::Explosion;
                sparkVis.maxDuration = 2.0f;
                sparkVis.color = sf::Color(150, 230, 255, 255);
                reg.AddComponent<SparkVisualComponent>(ball, sparkVis);

                // 移動速度をセット
                reg.AddComponent<VelocityComponent>(ball, VelocityComponent{ velocity });
            }
            break;
        }
        default:
            break;
        }
    }

    // スキル実行中の毎フレーム処理
    void UpdateActiveSkill(PlayerSkill& pc, VelocityComponent& vel, float dir) {
        auto& skill = pc.skills[pc.castingSkillIndex];

        if (skill.behaviorType == SkillBehaviorType::Dash) {
            //vel.velocity.x = dir * skill.range;
            //vel.velocity.y = 0.0f;
        }
    }

    // スキル終了処理
    void FinishSkill(PlayerSkill& pc, StateComponent& state, VelocityComponent& vel) {
        if (state.currentState == ActorState::Roll) {
            state.currentState = ActorState::Idle;
            vel.velocity.x = 0.0f;
            vel.velocity.y = 0.0f;
        }
        pc.castingSkillIndex = -1;
    }
};