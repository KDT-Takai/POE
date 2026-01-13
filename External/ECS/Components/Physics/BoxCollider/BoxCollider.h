#pragma once

struct BoxColliderComponent {
    float width = 32.0f;
    float height = 64.0f;

    // 中心からのオフセット（必要な場合）
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    // 状態フラグ
    bool isGrounded = false; // 地面に足がついているか
    bool hitWall = false;    // 壁にぶつかっているか
};