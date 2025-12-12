#include <spdlog/spdlog.h>
#include "ScreenManager.h"
#include "imgui.h"
#include "../../Game/GameScene/GameScreen/GameScreen.h"
#include "../../Game/TitleScene/TitleScreen/TitleSceen.h"


void ScreenManager::RegisterScreen(const std::string& name, ScreenFactory factory)
{
    registeredScreens[name] = std::move(factory);
}

ScreenManager::ScreenManager()
{
    // 全シーンの登録
    RegisterScreen<TitleScreen>();
    RegisterScreen<GameScreen>();

    // 初期シーンの設定
    currentScreenName = TitleScreen::GetName();
    screen = registeredScreens.at(currentScreenName)();
}

void ScreenManager::ChangeScreen(const std::string& screenName)
{
    if (registeredScreens.count(screenName)) {
        next = registeredScreens.at(screenName)();
        changeFlag = true;
        currentScreenName = screenName;
    }
	spdlog::info("ScreenManager: Changing screen to {}", screenName);
}

void ScreenManager::Update()
{
    screen->Update();
    if (changeFlag) {
        screen = std::move(next);
        next.reset();
        changeFlag = false;
    }
}

void ScreenManager::Render(sf::RenderTarget& target)
{
    if (screen) {
        // 現在のシーンのRender関数を呼び出す
        screen->Render(target);
    }
}

void ScreenManager::RenderImGui(const sf::Texture* renderTexture)
{
    if (screen)
    {
        screen->RenderImGui(renderTexture);
    }
}