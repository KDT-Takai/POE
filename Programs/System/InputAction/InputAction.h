#pragma once
#include <ECS.h>

// 入力抽象
enum class InputAction {
	Move,   // 移動
    Roll,   // ローリング
    Jump,   // ジャンプ
    Attack, // アタック
    Dash,   // ダッシュ
    Skill1, // スキル１
    Skill2, // スキル２
    Skill3, // スキル３
    Skill4, // スキル４
    Skill5, // スキル５
};
// ゲームイベント
enum class Trigger {
    OnInput,        // InputAction
    OnRoll,
    OnDash,
    OnJump,
    OnAttack,
    OnSkillCast,

    OnHit,
    OnKill,
    OnDamageTaken,

    OnLowHP,        // 条件系
    OnCritical,

    OnUpdate        // 常時監視
};

struct TriggerContext {
    Entity source;
    Entity target;

    InputAction action;
    double damage;
    bool isCritical;
};
