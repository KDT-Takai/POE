#include <spdlog/spdlog.h>
#include "SceneManager.h"
#include "imgui.h"
#include <Game/GameScene/GameScene/GameScene.h>
#include <Game/TitleScene/TitleScene/TitleScene.h>

void SceneManager::RegisterScene(const std::string& name, SceneFactory factory)
{
    registeredScenes[name] = std::move(factory);
}

SceneManager::SceneManager()
{
    // 全シーンの登録
    RegisterScene<TitleScene>();
    RegisterScene<GameScene>();
    // 初期シーンの設定
    currentSceneName = TitleScene::GetName();
    Scene = registeredScenes.at(currentSceneName)();
}

void SceneManager::ChangeScene(const std::string& SceneName)
{
    if (registeredScenes.count(SceneName)) {
        next = registeredScenes.at(SceneName)();
        changeFlag = true;
        currentSceneName = SceneName;
    }
	spdlog::info("SceneManager: Changing Scene to {}", SceneName);
}

void SceneManager::Update()
{
    Scene->Update();
    if (changeFlag) {
        Scene = std::move(next);
        next.reset();
        changeFlag = false;
    }
}

void SceneManager::Render(sf::RenderTarget& target)
{
    if (Scene) {
        // 現在のシーンのRender関数を呼び出す
        Scene->Render(target);
    }
}

void SceneManager::RenderImGui(const sf::Texture* renderTexture)
{
    if (Scene)
    {
        Scene->RenderImGui(renderTexture);
    }
}