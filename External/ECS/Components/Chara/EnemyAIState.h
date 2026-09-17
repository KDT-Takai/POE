#pragma once

// Aggro/leash state for regular map monsters (see EnemyAISystem): an enemy starts idle
// (does not move/attack even if the player is technically in the same zone) until the
// player comes within detection range, then chases/attacks like before. If the player
// then moves far enough away and stays away for a while, the enemy leashes back to idle
// instead of chasing forever across the whole map ("敵の挙動は一定範囲内に入ったら検知
// 遠ざかってしばらくしたら待機" -- detect within a range, then stand down again once the
// player has been gone for a bit).
struct EnemyAIStateComponent {
    bool aggro = false;
    float timeSinceLastSeen = 0.0f;
};
