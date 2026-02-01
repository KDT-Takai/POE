#pragma once

// インタラクト
enum class InteractType {
    SpiritMonument, // 精霊碑
    Chest,          // 宝箱
    Lever,          // レバー
    Portal          // ワープゾーン
};

struct InteractableComponent {
    InteractType type = InteractType::SpiritMonument;

    // 一度きりか
    bool once = true;

    bool autoTrigger = true;

    // 使用済みか
    bool used = false;

    int targetID = -1;
};