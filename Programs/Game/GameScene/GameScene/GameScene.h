#pragma once
#include <string>
#include <ECS.h>
#include <System/SceneManager/SceneBase.h>
// ‰¼
#include "../../ECS/Systems/Contorol/InputSystem.h"
#include "../../ECS/Systems/Physics/MovementSystem.h"
#include "../../ECS/Systems/Physics/PhysicsSystem.h"
#include "../../ECS/Systems/World/MapRenderSystem.h"
#include "../../ECS/Systems/Skill/SkillSystem.h"
#include "../../ECS/Systems/UI/UISystem.h"
#include "../../ECS/Systems/Skill/SparkVisualSystem.h"
#include "../../ECS/Systems/Physics/ProjectileSystem.h"
#include "../../ECS/Systems/Skill/SparkRenderSystem.h"
#include "../../ECS/Systems/Chara/EnemySpawnSystem.h"
#include "../../ECS/Systems/Chara/EnemyAISystem.h"
#include "../../ECS/Systems/Physics/CollisionSystem.h"
#include "../../ECS/Systems/UI/HealthBarRenderSystem.h"

class GameScene : public SceneBase {
public:
    static const char* GetName() { return "GameScene"; }

    GameScene();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:
    Entity playerEntity = -1;
    
    // Registry
	std::unique_ptr<Registry> registry;
    
    // System
    std::shared_ptr<EditorSystem> editorSystem;
    std::shared_ptr<RenderSystem> renderSystem;

    std::shared_ptr<InputSystem> inputSystem;
    std::shared_ptr<MovementSystem> movementSystem;
    std::shared_ptr<PhysicsSystem> physicsSystem;
    std::shared_ptr<MapRenderSystem> mapRenderSystem;
    std::shared_ptr<SkillSystem> skillSystem;
    std::shared_ptr<UISystem> uiSystem;
	std::shared_ptr<SparkVisualSystem> sparkVisualSystem;
	std::shared_ptr<ProjectileSystem> projectileSystem;
	std::shared_ptr<SparkRenderSystem> sparkRenderSystem;
	std::shared_ptr<EnemySpawnSystem> enemySpawnSystem;
	std::shared_ptr<EnemyAISystem> enemyAISystem;
	std::shared_ptr<CollisionSystem> collisionSystem;
    std::shared_ptr<HealthBarRenderSystem> healthBarRenderSystem;
};