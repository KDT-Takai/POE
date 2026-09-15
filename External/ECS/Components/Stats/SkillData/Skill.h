#pragma once
#include <string>
#include <vector>
#include "Components/Combat/DamageType.h"

enum class SkillBehaviorType {
    None,
    Melee,      // �ߐڍU��
    Projectile, // �e�𔭎�
    Dash,       // �ړ��n
    Buff,       // ���ȋ���
    AreaEffect,  // �͈͍U��
    Spark,      // ��
    GroundSlam,
    LightningWarp,
    LightningBall
};

struct SkillData {
    std::string name = "Empty";
    std::string description = "";
    int level = 1;
    int gemId = -1; // SkillGemData id that granted this skill; -1 = not from a gem (empty slot)

    SkillBehaviorType behaviorType = SkillBehaviorType::Melee;
    DamageElement element = DamageElement::Physical;

    float cooldownTime = 1.0f;      // �N�[���_�E��
    float currentCooldown = 0.0f;   // ���݂̑҂�����
    float castTime = 0.0f;          // �r������
    int mpCost = 0;                 // ����MP

    float damage = 0.0f;             // �З�
    float duration = 0.0f;          // ��������
    float range = 0.0f;             // �˒������⑬�x

    float buffAtk = 0.0f;
    float buffDef = 0.0f;
    float buffSpeed = 0.0f;

    // �L�����ǂ����̔���
    bool isValid = false;
};