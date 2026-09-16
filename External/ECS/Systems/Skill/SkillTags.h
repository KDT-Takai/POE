#pragma once
#include "../../Components/Stats/SkillData/Skill.h"
#include "../../Components/Combat/DamageType.h"

// Bitmask tags describing what a skill IS (not what it does numerically), used to gate
// which support gems are compatible with it (see SupportGemDefinition::requiredTag /
// SkillGemSystem::SupportOptions) and whether it needs a weapon equipped to register
// (Attack tag). Mirrors the spec's "Skill Tags" concept: tags are derived mechanically
// from behaviorType/element rather than hand-picked per gem, same convention as
// SkillGemData's GemAttribute assignment.
enum class SkillTag : unsigned int {
    None = 0,
    Attack = 1u << 0,      // uses a weapon swing/shot, as opposed to a Spell
    Spell = 1u << 1,
    Melee = 1u << 2,
    Projectile = 1u << 3,
    AreaEffect = 1u << 4,
    Duration = 1u << 5,    // has an ongoing effect (buffs, DoT-style skills)
    Movement = 1u << 6,
    Physical = 1u << 7,
    Elemental = 1u << 8,   // Fire/Cold/Lightning
    Chaos = 1u << 9,
};

inline unsigned int operator|(SkillTag a, SkillTag b) {
    return static_cast<unsigned int>(a) | static_cast<unsigned int>(b);
}
inline unsigned int operator|(unsigned int a, SkillTag b) {
    return a | static_cast<unsigned int>(b);
}
inline unsigned int& operator|=(unsigned int& a, SkillTag b) {
    a |= static_cast<unsigned int>(b);
    return a;
}

namespace SkillTags {
    inline unsigned int TagsFor(SkillBehaviorType type, DamageElement element) {
        unsigned int mask = 0;
        switch (type) {
        case SkillBehaviorType::Melee:
            mask |= SkillTag::Attack | SkillTag::Melee;
            break;
        case SkillBehaviorType::GroundSlam:
            mask |= SkillTag::Attack | SkillTag::Melee | SkillTag::AreaEffect;
            break;
        case SkillBehaviorType::Projectile:
            mask |= SkillTag::Attack | SkillTag::Projectile;
            break;
        case SkillBehaviorType::AreaEffect:
            mask |= SkillTag::Spell | SkillTag::AreaEffect;
            break;
        case SkillBehaviorType::Spark:
        case SkillBehaviorType::LightningBall:
            mask |= SkillTag::Spell | SkillTag::Projectile;
            break;
        case SkillBehaviorType::LightningWarp:
            mask |= SkillTag::Spell | SkillTag::AreaEffect;
            break;
        case SkillBehaviorType::Dash:
            mask |= SkillTag::Movement;
            break;
        case SkillBehaviorType::Buff:
            mask |= SkillTag::Duration;
            break;
        default:
            break;
        }

        // Element tags only make sense for damage-dealing behavior types (matches
        // SkillGemSystem::DealsElementalDamage's list) -- Dash/Buff/Aura/None leave
        // `element` at its unused default and shouldn't be tagged by it.
        bool dealsDamage = (mask & (SkillTag::Attack | SkillTag::Spell)) != 0;
        if (dealsDamage) {
            if (element == DamageElement::Physical) mask |= SkillTag::Physical;
            else if (element == DamageElement::Chaos) mask |= SkillTag::Chaos;
            else mask |= SkillTag::Elemental; // Fire/Cold/Lightning
        }

        return mask;
    }

    // True if a skill with `skillTags` may have a support gem requiring `requiredTag`
    // socketed into it. requiredTag == None means the support is universally compatible.
    inline bool IsCompatible(unsigned int skillTags, SkillTag requiredTag) {
        if (requiredTag == SkillTag::None) return true;
        return (skillTags & static_cast<unsigned int>(requiredTag)) != 0;
    }
}
