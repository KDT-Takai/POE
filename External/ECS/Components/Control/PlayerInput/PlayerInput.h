#pragma once

struct PlayerInputComponent {
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool roll = false;
    bool attack = false;

    // スキル入力などが必要になればここに追加
};