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

    // PoE2本家同様、ブロックは命中判定(RollHit)後に別枠でロールし、成功時はそのヒットの
    // ダメージを丸ごと無効化する(部分軽減ではない)。耐性と同じ75%上限。
    inline bool RollBlock(float blockChance) {
        if (blockChance <= 0.0f) return false;
        float chance = std::clamp(blockChance, 0.0f, 0.75f);
        return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < chance;
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

        if (RollBlock(target.blockChance)) return 0.0f;

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

    // Ground-effect DoT (Burning/Caustic Ground, see HazardGroundSystem): same resistance/
    // armour + ES-absorption order as ApplyDamage, but skips the block roll (PoE2's damage-
    // over-time effects are never blocked) and there's no ailment proc / crit to roll for a
    // per-frame tick.
    inline float ApplyDamageOverTime(CharacterStatsComponent& target, float rawDamage, DamageElement type) {
        float dmg = rawDamage;
        if (dmg <= 0.0f) return 0.0f;

        if (type == DamageElement::Physical) {
            dmg *= (1.0f - CalcArmourMitigation(target.armour, dmg));
        } else {
            dmg *= (1.0f - GetResistance(target, type));
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
    // Still used by Bleed(Physical)/stun -- Fire/Cold(freeze)/Lightning moved to the
    // buildup-threshold model below (see AccumulateIgnite/AccumulateFreeze/AccumulateShock).
    inline bool RollAilmentChance(float hitDamage, float targetMaxHP, bool isCrit) {
        if (isCrit) return true;
        if (targetMaxHP <= 0.0f) return false;
        float threshold = targetMaxHP * 0.25f;
        float chance = std::clamp(hitDamage / threshold, 0.0f, 1.0f);
        return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < chance;
    }

    // PoE2の"Elemental Ailment Threshold"を参考にしたIgnite/Freeze/Shockの発症モデル。
    // ダメージ(hitDamage、常に耐性適用後の値)を対象の閾値(maxHP*kAilmentThresholdFraction)
    // に達するまで蓄積し、達した時点で発症する。蓄積は未発症の間ずっと減衰し続ける
    // (DecayAilmentBuildup、StatusEffectSystemが毎フレーム呼ぶ)ため、耐性が高いほど
    // 1ヒット/1フレームあたりの蓄積量が減り、閾値到達=発症までが遅くなる
    // ("耐性で発症を遅らせる")。クリティカルは即発症(従来のRollAilmentChance互換)。
    constexpr float kAilmentThresholdFraction = 0.25f;
    constexpr float kAilmentBuildupDecayPerSecond = 0.6f; // 閾値に対する割合/秒

    inline void DecayAilmentBuildup(StatusEffectsComponent& status, float targetMaxHP, float dt) {
        if (targetMaxHP <= 0.0f) return;
        float decay = targetMaxHP * kAilmentThresholdFraction * kAilmentBuildupDecayPerSecond * dt;
        status.igniteBuildup = (std::max)(0.0f, status.igniteBuildup - decay);
        status.freezeBuildup = (std::max)(0.0f, status.freezeBuildup - decay);
        status.shockBuildup = (std::max)(0.0f, status.shockBuildup - decay);
    }

    inline void AccumulateIgnite(StatusEffectsComponent& status, float postResistDamage, float targetMaxHP, bool guaranteed) {
        if (postResistDamage <= 0.0f || targetMaxHP <= 0.0f) return;
        float threshold = targetMaxHP * kAilmentThresholdFraction;
        status.igniteBuildup += postResistDamage;
        if (guaranteed || status.igniteBuildup >= threshold) {
            status.igniteDps = (std::max)(status.igniteDps, postResistDamage * 0.5f);
            status.igniteRemaining = (std::max)(status.igniteRemaining, 4.0f);
            status.igniteBuildup = 0.0f;
        }
    }

    inline void AccumulateFreeze(StatusEffectsComponent& status, float postResistDamage, float targetMaxHP, bool guaranteed) {
        if (postResistDamage <= 0.0f || targetMaxHP <= 0.0f) return;
        float threshold = targetMaxHP * kAilmentThresholdFraction;
        status.freezeBuildup += postResistDamage;
        if (guaranteed || status.freezeBuildup >= threshold) {
            status.freezeRemaining = (std::max)(status.freezeRemaining, 1.0f);
            status.freezeBuildup = 0.0f;
        }
    }

    inline void AccumulateShock(StatusEffectsComponent& status, float postResistDamage, float targetMaxHP, bool guaranteed) {
        if (postResistDamage <= 0.0f || targetMaxHP <= 0.0f) return;
        float threshold = targetMaxHP * kAilmentThresholdFraction;
        status.shockBuildup += postResistDamage;
        if (guaranteed || status.shockBuildup >= threshold) {
            status.shockRemaining = (std::max)(status.shockRemaining, 3.0f);
            status.shockIncreasedDamageTaken = (std::max)(status.shockIncreasedDamageTaken, 0.2f);
            status.shockBuildup = 0.0f;
        }
    }

    inline void ApplyAilmentOnHit(StatusEffectsComponent& status, DamageElement type, float hitDamage, float targetMaxHP, bool isCrit) {
        switch (type) {
        case DamageElement::Fire:
            AccumulateIgnite(status, hitDamage, targetMaxHP, isCrit);
            break;
        case DamageElement::Cold:
            // Chillそのもの(スロウ)は従来通りヒットで即時付与(PoE2でもほぼ確定でかかる)。
            // Freezeだけが耐性で遅らせられる閾値蓄積モデルに乗る。
            status.chillRemaining = 2.0f;
            status.chillSlowPercent = 0.3f;
            AccumulateFreeze(status, hitDamage, targetMaxHP, isCrit);
            break;
        case DamageElement::Lightning:
            AccumulateShock(status, hitDamage, targetMaxHP, isCrit);
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
