#pragma once
#include <vector>
#include <array>
#include "../../Core/Entity.h"

// One owned gem "individual" (matches PoE2: a dropped gem has its own level, and skill/
// spirit gems have their own support-gem sockets). isSupport distinguishes which catalog
// gemId refers to: SkillGemData (false) or SupportGemData (true). Support gems are not
// leveled (level always 1) and have no sockets of their own. At most one OwnedGemInstance
// exists per gemId (re-identifying/re-dropping a known gem upgrades its level in place
// instead of adding a duplicate entry -- see GemIdentifySystem).
// supportGemIds holds raw SupportGemData ids of the currently-socketed support gems
// (same convention as PlayerSkill.skills[i].gemId / SpiritGemLoadoutComponent.auraGemIds
// referencing raw SkillGemData ids directly, not owned-instance indices) -- ownership is
// still validated by scanning ownedGems for a matching isSupport entry at assign time.
struct OwnedGemInstance {
    int gemId = -1;
    bool isSupport = false;
    int level = 1;
    int maxSockets = 2;
    std::array<int, 5> supportGemIds = { -1, -1, -1, -1, -1 };
};

// Which catalog an Uncut Gem draws its candidates from once identified (Gem Cutting).
enum class GemPickupKind { Skill, Support, Spirit };

// A dropped, not-yet-identified gem (see GemIdentifySystem): only its rolled level and
// kind are known until the player picks which specific gem it becomes.
struct SkillGemPickupComponent {
    int level = 1;
    GemPickupKind kind = GemPickupKind::Skill;
};

// Held but not-yet-identified (spec: Uncut Gems go into the inventory as items, not
// resolved instantly on pickup -- picking one up just queues it here, capacity-limited
// like a real inventory; the player explicitly opens Gem Cutting (GemIdentifySystem)
// from SkillGemSystem's "Uncut Gems" row when ready).
struct PendingUncutGem {
    int level = 1;
    GemPickupKind kind = GemPickupKind::Skill;
};

struct SkillGemInventoryComponent {
    static constexpr size_t kPendingCapacity = 8;
    std::vector<OwnedGemInstance> ownedGems;
    std::vector<PendingUncutGem> pendingUncutGems;
};

// 5 Spirit slots (expanded from a fixed 2), separate from the 5 activated skill slots in
// PlayerSkill -- together they form the 10-slot shared equip pool (SkillGemSystem).
// -1 = empty. Registering a gem into a slot (auraGemIds[i]) does NOT reserve Spirit or
// apply its effect by itself -- active[i] tracks whether the player has separately
// switched that registered gem ON, matching the spec's two-step
// "register -> (separately) toggle ON" flow. See SpiritAuraSystem.
struct SpiritGemLoadoutComponent {
    std::array<int, 5> auraGemIds = { -1, -1, -1, -1, -1 };
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
