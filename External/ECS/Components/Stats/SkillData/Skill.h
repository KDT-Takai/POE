#pragma once
#include <string>
#include <vector>

// ‘®«
enum class ElementType {
    None,
    Fire,
    Ice,
    Lightning,
    Dark,
    Holy
};

enum class SkillBehaviorType {
    None,
    Melee,      // ‹ßÚUŒ‚
    Projectile, // ’e‚ğ”­Ë
    Dash,       // ˆÚ“®Œn
    Buff,       // ©ŒÈ‹­‰»
    AreaEffect,  // ”ÍˆÍUŒ‚
    Spark,      // —‹
    GroundSlam,
    LightningWarp,
    LightningBall
};

struct SkillData {
    std::string name = "Empty";
    std::string description = "";
    int level = 1;

    SkillBehaviorType behaviorType = SkillBehaviorType::Melee;
    ElementType element = ElementType::None;

    float cooldownTime = 1.0f;      // ƒN[ƒ‹ƒ_ƒEƒ“
    float currentCooldown = 0.0f;   // Œ»İ‚Ì‘Ò‚¿ŠÔ
    float castTime = 0.0f;          // ‰r¥ŠÔ
    int mpCost = 0;                 // Á”ïMP

    float damage = 0.0f;             // ˆĞ—Í
    float duration = 0.0f;          // ‘±ŠÔ
    float range = 0.0f;             // Ë’ö‹——£‚â‘¬“x

    float buffAtk = 0.0f;
    float buffDef = 0.0f;
    float buffSpeed = 0.0f;

    // —LŒø‚©‚Ç‚¤‚©‚Ì”»•Ê
    bool isValid = false;
};