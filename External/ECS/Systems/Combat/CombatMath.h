#pragma once
#include <algorithm>
#include <cmath>
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "Components/Combat/StatusEffects.h"

namespace CombatMath {

    inline float CalcHitChance(float accuracy, float evasion) {
        if (accuracy <= 0.0f) return 0.05f;
        float chance = accuracy / (accuracy + evasion * 0.5f);
        return std::clamp(chance, 0.05f, 1.0f);
    }

    inline bool RollHit(float accuracy, float evasion) {
        float chance = CalcHitChance(accuracy, evasion);
        return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < chance;
    }

    inline bool RollCrit(float critChance) {
        return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < critChance;
    }

    inline float CalcArmourMitigation(float armour, float incomingPhysicalDamage) {
        if (armour <= 0.0f || incomingPhysicalDamage <= 0.0f) return 0.0f;
        float mitigation = armour / (armour + 10.0f * incomingPhysicalDamage);
        return std::clamp(mitigation, 0.0f, 0.9f);
    }

    inline float GetResistance(const CharacterStatsComponent& stats, DamageElement type) {
        float res = 0.0f;
        switch (type) {
        case DamageElement::Fire: res = stats.fireRes; break;
        case DamageElement::Cold: res = stats.iceRes; break;
        case DamageElement::Lightning: res = stats.lightningRes; break;
        case DamageElement::Chaos: res = stats.chaosRes; break;
        default: return 0.0f;
        }
        return std::clamp(res, -1.0f, 0.75f);
    }

    // Applies mitigation/resistance/ES/Life, in the PoE-style defense order. Returns final damage dealt to HP+ES.
    inline float ApplyDamage(CharacterStatsComponent& target, float rawDamage, DamageElement type) {
        float dmg = rawDamage;
        if (dmg <= 0.0f) return 0.0f;

        if (type == DamageElement::Physical) {
            dmg *= (1.0f - CalcArmourMitigation(target.armour, dmg));
        } else if (type != DamageElement::Chaos) {
            dmg *= (1.0f - GetResistance(target, type));
        } else {
            dmg *= (1.0f - GetResistance(target, DamageElement::Chaos));
        }

        if (dmg < 0.0f) dmg = 0.0f;

        if (type != DamageElement::Chaos && target.currentES > 0.0f) {
            float absorbed = (std::min)(target.currentES, dmg);
            target.currentES -= absorbed;
            dmg -= absorbed;
            target.esRegenDelay = 3.0f;
        }

        target.currentHP -= dmg;
        if (target.currentHP < 0.0f) target.currentHP = 0.0f;
        return dmg;
    }

    // Ailment proc chance: guaranteed on crit, otherwise scales with damage vs a fraction of target max life.
    inline bool RollAilmentChance(float hitDamage, float targetMaxHP, bool isCrit) {
        if (isCrit) return true;
        if (targetMaxHP <= 0.0f) return false;
        float threshold = targetMaxHP * 0.25f;
        float chance = std::clamp(hitDamage / threshold, 0.0f, 1.0f);
        return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < chance;
    }

    inline void ApplyAilmentOnHit(StatusEffectsComponent& status, DamageElement type, float hitDamage, float targetMaxHP, bool isCrit) {
        switch (type) {
        case DamageElement::Fire:
            if (RollAilmentChance(hitDamage, targetMaxHP, isCrit)) {
                status.igniteDps = hitDamage * 0.5f;
                status.igniteRemaining = 4.0f;
            }
            break;
        case DamageElement::Cold:
            status.chillRemaining = 2.0f;
            status.chillSlowPercent = 0.3f;
            if (RollAilmentChance(hitDamage, targetMaxHP, isCrit)) {
                status.freezeRemaining = 1.0f;
            }
            break;
        case DamageElement::Lightning:
            if (RollAilmentChance(hitDamage, targetMaxHP, isCrit)) {
                status.shockRemaining = 3.0f;
                status.shockIncreasedDamageTaken = 0.2f;
            }
            break;
        case DamageElement::Physical:
            if (RollAilmentChance(hitDamage, targetMaxHP, isCrit)) {
                status.bleedDps = hitDamage * 0.3f;
                status.bleedRemaining = 5.0f;
            }
            break;
        case DamageElement::Chaos:
            status.poisonStacks.push_back({ hitDamage * 0.3f, 2.0f });
            break;
        }

        if (hitDamage >= targetMaxHP * 0.15f) {
            status.stunRemaining = 0.3f;
        }
    }
}
