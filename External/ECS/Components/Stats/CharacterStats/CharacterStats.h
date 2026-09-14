#pragma once
#include <string>
#include "Components/Combat/DamageType.h"

struct CharacterStatsComponent {
    // Basic Info
    std::string name = "Unknown";
    int level = 1;
    int currentXP = 0;
    int xpToNextLevel = 100;
    float currentHP = 100.0f;
    float maxHP = 100.0f;
	float healthRegen = 3.0f;
    float currentMP = 200.0f;
    float maxMP = 200.0f;
    float manaRegen = 20.0f;

    float currentES = 0.0f;
    float maxES = 0.0f;
    float esRegenDelay = 0.0f;

    float maxSpirit = 0.0f;
    float currentSpirit = 0.0f;

    // Primary Attributes (Passive Tree�\�� ���ԂȂ����[)
    int str = 10; // Strength:MaxHP�Ȃǂɉe��
    int dex = 10; // Dexterity:AtkSpd�Ȃǂɉe��
    int intelligence = 10; // Intelligence:MaxMP�Ȃǂɉe��

    // Combat Stats
    float atk = 10.0f;
    float def = 0.0f;
    float atkSpd = 1.0f;
    float critRate = 0.05f;
    float critDamage = 1.50f;

    // Movement Stats
    float moveSpeed = 50.0f;

    float rollSpeed = 800.0f;       // ��𒆂̈ړ����x (�ʏ�ړ��̖�4�{)
    float rollDuration = 0.35f;     // ����̎������� (�b)
    float rollCooldownMax = 2.0f;   // �N�[���_�E���̍ő厞�� (�b)
    float hitInvincibilityTimer = 0.0f;
    float rollCooldownTimer = 0.0f;// ���݂̃N�[���_�E���c�莞�� (0�Ȃ�g�p�\)

    // Resistances (Max 75%)
    float fireRes = 0.0f;
    float iceRes = 0.0f;
    float lightningRes = 0.0f;
    float chaosRes = 0.0f;

    // Defense / accuracy
    float evasion = 0.0f;
    float armour = 0.0f;
    float accuracy = 100.0f;

    // Leech
    float leechPercent = 0.0f;
    float leechRateCap = 0.02f; // fraction of maxHP that can be leeched per second
    float pendingLeech = 0.0f;

    MonsterRarity rarity = MonsterRarity::Normal;
    DamageElement contactDamageType = DamageElement::Physical;

    int gold = 0;
};