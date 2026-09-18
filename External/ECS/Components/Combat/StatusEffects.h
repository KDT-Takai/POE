#pragma once
#include <vector>

struct PoisonStack {
    float damagePerSecond = 0.0f;
    float remaining = 0.0f;
};

struct StatusEffectsComponent {
    float igniteDps = 0.0f;
    float igniteRemaining = 0.0f;

    float bleedDps = 0.0f;
    float bleedRemaining = 0.0f;

    std::vector<PoisonStack> poisonStacks;

    float chillRemaining = 0.0f;
    float chillSlowPercent = 0.0f;

    float freezeRemaining = 0.0f;

    float shockRemaining = 0.0f;
    float shockIncreasedDamageTaken = 0.0f;

    float stunRemaining = 0.0f;

    // PoE2の"Elemental Ailment Threshold"を参考にした蓄積値。耐性で軽減された後の
    // ダメージだけを積み増し、対象の未発症時は毎フレーム減衰する(CombatMath::
    // DecayAilmentBuildup)ため、耐性が高いほど閾値到達=発症が遅くなる("耐性で遅らせる")。
    // Ignite/Freeze/Shockに使う(Chillは従来通りヒットで即時付与、Bleed/Poison/Stunは
    // 対象外、CombatMath::ApplyAilmentOnHit参照)。
    float igniteBuildup = 0.0f;
    float freezeBuildup = 0.0f;
    float shockBuildup = 0.0f;

    // Self-buff skills (e.g. War Cry gem). Applied directly to live CharacterStatsComponent
    // on activation; reverted via a full EquipmentSystem::RecalculateStats on expiry so a
    // mid-buff stat recalc (level up, re-equip, etc.) can never leave a stale multiplier baked in.
    float buffRemaining = 0.0f;
    float buffAtkMult = 1.0f;
    float buffSpeedMult = 1.0f;

    bool IsHardLocked() const { return freezeRemaining > 0.0f || stunRemaining > 0.0f; }
};
