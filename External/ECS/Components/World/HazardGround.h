#pragma once
#include <cstdint>

// PoE2風の床属性ギミック。MapComponent::hazardTilesにタイル単位で焼き込まれる地形情報
// (円形エンティティではない、Dirt/Stone等の他のTileTypeと同じ「マスに紐づく属性」として
// 扱う、ユーザー指示の「床はタイルで」に対応)。内側のタイルにいる間だけ効果を受ける
// (HazardGroundSystem)。Burning/Causticは直接DoTダメージ、Shocked/Chilledは既存の
// StatusEffectsComponentのShock/Chill(+耐性で発症を遅らせるElemental Ailment Threshold、
// CombatMath::AccumulateShock/AccumulateFreeze参照)を間接的に発生させる。
enum class HazardGroundKind : uint8_t {
    None = 0,
    Burning = 1, // 燃焼床: Fire DoT + 継続露出でIgniteも発症しうる
    Shocked = 2, // 雷の床: 耐性で遅らせられるShock(被ダメージ増加)蓄積
    Chilled = 3, // 氷の床: 即時Chill(移動速度低下、耐性で減衰) + 耐性で遅らせられるFreeze蓄積
    Caustic = 4, // 混沌ダメの床: Chaos DoT(ESを無視)
};
