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

    // ターゲットID
    // 精霊碑の場合：獲得する精霊のID
    // 宝箱の場合：中身のアイテムID
    // ポータルの場合：行き先のマップID
    int targetID = -1;
};