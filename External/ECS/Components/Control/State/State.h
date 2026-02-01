#pragma once

// アニメーションや行動制御のための状態
enum class ActorState {
    Idle,
    Run,
    Jump,
    Fall,
    Attack,
    Roll,
    Hit,
    Dead
};

struct StateComponent {
    ActorState currentState = ActorState::Idle;
    float stateTimer = 0.0f; // その状態になってからの経過時間
    bool isFacingRight = true;
};