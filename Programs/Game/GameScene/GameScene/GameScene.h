#pragma once
#include <string>
#include <ECS.h>
#include <System/SceneManager/SceneBase.h>
#include <System/Campaign/CampaignManager.h>
// ��
#include "../../ECS/Systems/Contorol/InputSystem.h"
#include "../../ECS/Systems/Physics/PhysicsSystem.h"
#include "../../ECS/Systems/World/MapRenderSystem.h"
#include "../../ECS/Systems/Skill/SkillSystem.h"
#include "../../ECS/Systems/UI/UISystem.h"
#include "../../ECS/Systems/Skill/SparkVisualSystem.h"
#include "../../ECS/Systems/Physics/ProjectileSystem.h"
#include "../../ECS/Systems/Skill/SparkRenderSystem.h"
#include "../../ECS/Systems/Chara/EnemySpawnSystem.h"
#include "../../ECS/Systems/Chara/EnemyAISystem.h"
#include "../../ECS/Systems/Chara/EnemyRangedAttackSystem.h"
#include "../../ECS/Systems/Chara/EnemyAreaAttackSystem.h"
#include "../../ECS/Systems/Chara/EnemySummonSystem.h"
#include "../../ECS/Systems/Chara/EnemyChargeSystem.h"
#include "../../ECS/Systems/Physics/CollisionSystem.h"
#include "../../ECS/Systems/UI/HealthBarRenderSystem.h"
#include "../../ECS/Systems/Combat/StatusEffectSystem.h"
#include "../../ECS/Systems/Item/ItemPickupSystem.h"
#include "../../ECS/Systems/Chara/BossPhaseSystem.h"
#include "../../ECS/Systems/UI/CharacterSheetSystem.h"
#include "../../ECS/Systems/UI/InventorySystem.h"
#include "../../ECS/Systems/Progression/PassiveTreeSystem.h"
#include "../../ECS/Systems/Progression/AtlasSystem.h"
#include "../../ECS/Systems/Item/VendorSystem.h"
#include "../../ECS/Systems/Item/WaystoneVendorSystem.h"
#include "../../ECS/Systems/UI/StashSystem.h"
#include "../../ECS/Systems/UI/SkillGemSystem.h"
#include "../../ECS/Systems/UI/GemIdentifySystem.h"
#include "../../ECS/Systems/UI/KeyBindSystem.h"
#include "../../ECS/Systems/Chara/MinionSystem.h"
#include "../../ECS/Components/Tags/Boss/Boss.h"
#include "../Zone/TownNpc.h"

class GameScene : public SceneBase {
public:
    static const char* GetName() { return "GameScene"; }

    GameScene();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:
    void AdvanceToNextZone();
    void TryOpenEndgameMapFromHub();
    std::string HeldWaystoneSummary() const;
    void RenderGemDebugTools();
    static sf::Color NpcRingColor(TownNpcKind kind);
    static std::string NpcHudHint(TownNpcKind kind);

    Entity playerEntity = -1;

    ZoneKind m_zoneKind = ZoneKind::Combat;
    bool m_hasPortal = false;
    bool m_playerNearPortal = false;
    sf::Vector2f m_portalPos;

    // Town NPCs (item Vendor / Waystone Vendor / Stash, see ZoneBuilder::PlaceTownNpcs).
    // m_nearNpcIndex is the one within click range this frame (-1 = none); at most one
    // can be "near" meaningfully since kNpcClickRadius is much smaller than their
    // min-separation, so no ambiguity about which one a click means.
    std::vector<TownNpcSpawn> m_townNpcs;
    int m_nearNpcIndex = -1;
    bool m_clickedOnNpc = false;  // このフレームでNPCを左クリックしたか(スキル発動クリックとの競合防止用)
    bool m_hoveringNpc = false;   // クリック可能範囲にカーソルがあるか(輪の描画に使用)
    static constexpr float kVendorClickRadius = 36.0f;

    // Registry
	std::unique_ptr<Registry> registry;
    
    // System
    std::shared_ptr<EditorSystem> editorSystem;
    std::shared_ptr<RenderSystem> renderSystem;

    std::shared_ptr<InputSystem> inputSystem;
    std::shared_ptr<PhysicsSystem> physicsSystem;
    std::shared_ptr<MapRenderSystem> mapRenderSystem;
    std::shared_ptr<SkillSystem> skillSystem;
    std::shared_ptr<UISystem> uiSystem;
	std::shared_ptr<SparkVisualSystem> sparkVisualSystem;
	std::shared_ptr<ProjectileSystem> projectileSystem;
	std::shared_ptr<SparkRenderSystem> sparkRenderSystem;
	std::shared_ptr<EnemySpawnSystem> enemySpawnSystem;
	std::shared_ptr<EnemyAISystem> enemyAISystem;
	std::shared_ptr<EnemyRangedAttackSystem> enemyRangedAttackSystem;
	std::shared_ptr<EnemyAreaAttackSystem> enemyAreaAttackSystem;
	std::shared_ptr<EnemySummonSystem> enemySummonSystem;
	std::shared_ptr<EnemyChargeSystem> enemyChargeSystem;
	std::shared_ptr<CollisionSystem> collisionSystem;
    std::shared_ptr<HealthBarRenderSystem> healthBarRenderSystem;
    std::shared_ptr<StatusEffectSystem> statusEffectSystem;
    std::shared_ptr<ItemPickupSystem> itemPickupSystem;
    std::shared_ptr<BossPhaseSystem> bossPhaseSystem;
    std::shared_ptr<CharacterSheetSystem> characterSheetSystem;
    std::shared_ptr<InventorySystem> inventorySystem;
    std::shared_ptr<PassiveTreeSystem> passiveTreeSystem;
    std::shared_ptr<AtlasSystem> atlasSystem;
    std::shared_ptr<VendorSystem> vendorSystem;
    std::shared_ptr<WaystoneVendorSystem> waystoneVendorSystem;
    std::shared_ptr<StashSystem> stashSystem;
    std::shared_ptr<SkillGemSystem> skillGemSystem;
    std::shared_ptr<GemIdentifySystem> gemIdentifySystem;
    std::shared_ptr<KeyBindSystem> keyBindSystem;
    std::shared_ptr<MinionSystem> minionSystem;
};