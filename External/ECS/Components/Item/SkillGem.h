#pragma once
#include <array>
#include <optional>
#include "../../Core/Entity.h"
#include "Item.h"

// 5 Spirit slots (expanded from a fixed 2), separate from the 5 activated skill slots in
// PlayerSkill -- together they form the 10-slot shared equip pool (SkillGemSystem). Each
// slot holds the actual bag item (ItemComponent, category==SkillGem) once equipped --
// std::nullopt means empty, matching EquipmentComponent's own optional-slot pattern for
// gear. Equipping/unequipping physically moves the item between here and
// InventoryComponent::items (see SkillGemSystem), per "スキルジェムもウェイストーンみたいに
// アイテム欄に置く" (AI/DECISIONS.md). items[i] does NOT by itself reserve Spirit or apply
// its effect -- active[i] tracks whether the player has separately switched that equipped
// gem ON, matching the spec's two-step "equip -> (separately) toggle ON" flow. See
// SpiritAuraSystem.
struct SpiritGemLoadoutComponent {
    std::array<std::optional<ItemComponent>, 5> items;
    std::array<bool, 5> active = { false, false, false, false, false };

    // Permanent Minion bookkeeping (SkillBehaviorType::Minion gems only; unused/left at
    // defaults for Aura gems). minionEntity[i] is the live minion entity for slot i while
    // one exists, kInvalidEntity otherwise. minionRespawnTimer[i] counts down while the
    // slot is active but the minion is currently dead (see MinionSystem) -- the slot stays
    // active[i]==true (Spirit stays reserved) the whole time, per spec: minion death does
    // not release Spirit, only turning the skill OFF does.
    static constexpr Entity kInvalidMinion = static_cast<Entity>(-1);
    std::array<Entity, 5> minionEntity = { kInvalidMinion, kInvalidMinion, kInvalidMinion, kInvalidMinion, kInvalidMinion };
    std::array<float, 5> minionRespawnTimer = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
};
