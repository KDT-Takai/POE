#pragma once
#include <ECS.h>
#include "../ECS/Components/Stats/SkillData/Skill.h"
#include "Components/Combat/DamageType.h"

struct ProjectileComponent {
    float damage = 10.0f;
    float duration = 3.0f;     // ��������
    float currentTimer = 0.0f; // �o�ߎ���
    bool isEnemy = false;

    bool isBouncy = false;
	Entity ownerEntity = 0;
    SkillBehaviorType type;
    DamageElement damageType = DamageElement::Physical;
};