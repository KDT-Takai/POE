#pragma once

// Marks a monster as a ranged archetype: EnemyAISystem keeps it at preferredRange
// instead of closing to melee, and EnemyRangedAttackSystem fires a projectile at
// the player whenever they are within attackRange and the cooldown is ready.
struct RangedAttackerComponent {
    float preferredRange = 220.0f;
    float attackRange = 320.0f;
    float cooldownTime = 2.2f;
    float currentCooldown = 0.0f;
    float projectileSpeed = 260.0f;
    float damagePercent = 60.0f; // % of atk, same convention as SkillData::damage
};
