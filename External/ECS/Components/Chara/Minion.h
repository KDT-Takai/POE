#pragma once
#include "../../Core/Entity.h"

// Marks an entity as friendly to the player: excluded from the "enemies" target pool in
// both directions in CollisionSystem (the player's own projectiles never hit it, and it
// never deals contact damage to the player), since this project's collision code has no
// general faction system -- everything without PlayerInputComponent is otherwise treated
// as hostile. Currently only used by permanent minions (see PermanentMinionComponent).
struct AllyTagComponent {};

// A permanent ally spawned by a SkillBehaviorType::Minion Spirit gem (see MinionSystem).
// ownerEntity/spiritSlotIndex point back at the player's SpiritGemLoadoutComponent slot
// that owns this minion, so MinionSystem can update that slot's minionEntity/
// minionRespawnTimer when this entity dies.
struct PermanentMinionComponent {
    Entity ownerEntity = static_cast<Entity>(-1);
    int spiritSlotIndex = -1;
    int gemId = -1;
    float attackCooldown = 0.0f;
};
