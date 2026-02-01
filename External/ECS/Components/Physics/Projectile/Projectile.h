#pragma once
#include <ECS.h>
#include "../ECS/Components/Stats/SkillData/Skill.h"

struct ProjectileComponent {
    float damage = 10.0f;
    float duration = 3.0f;     // ¶‘¶ŠÔ
    float currentTimer = 0.0f; // Œo‰ßŠÔ
    bool isEnemy = false;

    bool isBouncy = false;
	Entity ownerEntity = 0;
    SkillBehaviorType type;
};