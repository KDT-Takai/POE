#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Physics/Velocity/Velocity.h"
#include "../../Components/Physics/Gravity/Gravity.h"
#include "../../Components/Physics/BoxCollider/BoxCollider.h"
#include "../../Components/Physics/Projectile/Projectile.h"
#include "../../Components/World/Map.h"

class PhysicsSystem {
public:
    void Update(Registry& registry, float dt) {
        // マップの取得
        const MapComponent* map = nullptr;
        auto mapEntities = registry.View<MapComponent>();
        if (!mapEntities.empty()) {
            map = &registry.GetComponent<MapComponent>(mapEntities[0]);
        }

        // 重力を速度に適用
        // GravityComponent を持つエンティティに対して
        //auto gravityView = registry.View<GravityComponent>();
        //for (auto entity : gravityView) {
        //    if (registry.HasComponent<VelocityComponent>(entity)) {
        //        auto& velocity = registry.GetComponent<VelocityComponent>(entity);
        //        auto& gravity = registry.GetComponent<GravityComponent>(entity);

        //        velocity.velocity.y += gravity.force * dt;
        //    }
        //}

        // 速度を座標に適用
        // Velocity と Transform を持つエンティティすべてに対して
        auto velocityView = registry.View<VelocityComponent>();
        for (auto entity : velocityView) {
            if (registry.HasComponent<TransformComponent>(entity)) {
                auto& vel = registry.GetComponent<VelocityComponent>(entity);
                auto& trans = registry.GetComponent<TransformComponent>(entity);

                bool hasCollider = registry.HasComponent<BoxColliderComponent>(entity);
                if (!hasCollider || !map) {
                    trans.position += vel.velocity * dt;
                    continue;
                }

                auto& col = registry.GetComponent<BoxColliderComponent>(entity);

                // 投射物チェック
                ProjectileComponent* proj = nullptr;
                if (registry.HasComponent<ProjectileComponent>(entity)) {
                    proj = &registry.GetComponent<ProjectileComponent>(entity);
                }

                // X軸
                trans.position.x += vel.velocity.x * dt;
                col.hitWall = false;
                // 衝突があったら位置補正、速度リセット
                if (ResolveMapCollision(trans, col, vel, *map, true)) {
                    if (proj) {
                        if (proj->isBouncy) vel.velocity.x *= -1; // 反射
                        else { registry.DestroyEntity(entity); continue; }
                    }
                    else {
                        vel.velocity.x = 0.0f;
                    }
                }

                // Y軸の処理
                trans.position.y += vel.velocity.y * dt;
                col.isGrounded = false;
                // 衝突があったら位置補正、速度リセット
                if (ResolveMapCollision(trans, col, vel, *map, false)) {
                    if (proj) {
                        if (proj->isBouncy) vel.velocity.y *= -1; // 反射
                        else { registry.DestroyEntity(entity); continue; }
                    }
                    else {
                        vel.velocity.y = 0.0f;
                    }
                }
                // Y=500のラインを地面
                //float groundY = 500.0f;
                //if (trans.position.y >= groundY) {
                //    // 位置補正
                //    trans.position.y = groundY;
                //    // 接地したら落下速度を0
                //    if (vel.velocity.y > 0) vel.velocity.y = 0;
                //    // 接地フラグをON
                //    if (registry.HasComponent<BoxColliderComponent>(entity)) {
                //        registry.GetComponent<BoxColliderComponent>(entity).isGrounded = true;
                //    }
                //}
                //else {
                //    // 空中にいる
                //    if (registry.HasComponent<BoxColliderComponent>(entity)) {
                //        if (vel.velocity.y > 0) // 落下中
                //            registry.GetComponent<BoxColliderComponent>(entity).isGrounded = false;
                //    }
                //}
            }
        }
    }
private:
    bool ResolveMapCollision(TransformComponent& trans, BoxColliderComponent& col, VelocityComponent& vel, const MapComponent& map, bool isXAxis) {
        bool hasCollision = false;

        // スキン幅（判定の遊び）
        const float skin = 2.0f;

        // プレイヤーの矩形
        float minX = trans.position.x + col.offsetX;
        float maxX = minX + col.width;
        float minY = trans.position.y + col.offsetY;
        float maxY = minY + col.height;

        // 判定用の矩形を縮める（スキン適用）
        // X移動中はYを少し縮め、Y移動中はXを少し縮める
        if (isXAxis) {
            minY += skin;
            maxY -= skin;
        }
        else {
            minX += skin;
            maxX -= skin;
        }

        sf::Vector2i minTile = map.WorldToTile(minX, minY);
        sf::Vector2i maxTile = map.WorldToTile(maxX, maxY);

        // 範囲内スキャン
        for (int y = minTile.y; y <= maxTile.y; ++y) {
            for (int x = minTile.x; x <= maxTile.x; ++x) {
                TileType tile = map.GetTile(x, y);
                if (tile == TileType::Dirt || tile == TileType::Wood || tile == TileType::Grass) continue;

                // タイルの矩形
                float tileLeft = x * map.tileSize;
                float tileRight = tileLeft + map.tileSize;
                float tileTop = y * map.tileSize;
                float tileBottom = tileTop + map.tileSize;

                float playerCenterX = minX + (col.width / 2.0f);
                float playerCenterY = minY + (col.height / 2.0f);
                float tileCenterX = tileLeft + (map.tileSize / 2.0f);
                float tileCenterY = tileTop + (map.tileSize / 2.0f);

                if (isXAxis) {
                    float dx1 = maxX - tileLeft; // 右へのめり込み
                    float dx2 = tileRight - minX; // 左へのめり込み

                    // プレイヤーがタイルの左側にいるなら「左へ押し出す」
                    if (playerCenterX < tileCenterX) {
                        // 妥当なめり込み量かチェック
                        if (dx1 > 0 && dx1 < map.tileSize) {
                            trans.position.x -= dx1;
                            col.hitWall = true;
                            hasCollision = true;
                        }
                    }
                    // プレイヤーがタイルの右側にいるなら「右へ押し出す」
                    else {
                        if (dx2 > 0 && dx2 < map.tileSize) {
                            trans.position.x += dx2;
                            col.hitWall = true;
                            hasCollision = true;
                        }
                    }
                    if (hasCollision) return true; // X軸は1つでも当たれば終了
                }
                else {
                    float dy1 = maxY - tileTop;    // 下へのめり込み
                    float dy2 = tileBottom - minY; // 上へのめり込み

                    if (playerCenterY < tileCenterY) {
                        if (dy1 > 0 && dy1 < map.tileSize) {
                            trans.position.y -= dy1;
                            col.isGrounded = true;
                            hasCollision = true;
                        }
                    }
                    else {
                        if (dy2 > 0 && dy2 < map.tileSize) {
                            trans.position.y += dy2;
                            col.hitWall = true; // 天井ヒット
                            hasCollision = true;
                        }
                    }
                    if (hasCollision) return true;
                }
            }
        }
        return hasCollision;
    }
};