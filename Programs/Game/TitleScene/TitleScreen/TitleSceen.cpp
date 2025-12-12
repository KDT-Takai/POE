#include "TitleSceen.h"
#include <spdlog/spdlog.h>
#include "../../../System/Config/Config.h"
#include "../../../System/CameraManager/CameraManager.h"
#include "../../../System/Resource/ResourceManager/ResourceManager.h"
#include "../../../System/Time/Time.h"


TitleScreen::TitleScreen() {
    screenName = "TitleScreen";

    // --- 動く円の初期化 ---
    movingCircle.setRadius(30.0f);
    movingCircle.setFillColor(sf::Color::Green);
    movingCircle.setPosition({ 100.0f, 100.0f }); // 初期位置

    // 移動速度の設定 (X方向に3, Y方向に2)
    circleVelocity = { 3.0f, 2.0f };

    ResourceManager::Instance().loadAllTextures("Assets/Textures");
    ResourceManager::Instance().loadAllFonts("Assets/Fonts");
    ResourceManager::Instance().loadAllSounds("Assets/Sounds");

    auto test = ResourceManager::Instance().getTexture("Assets/Textures/test.png");
    if (test) {
		testSprite = std::make_unique<sf::Sprite>(*test);
        testSprite->setPosition({ 100.0f, 100.0f }); // 位置調整
    }

    //spdlog::set_level(spdlog::level::trace);
    // これで trace / debug / info / warn / error / critical 全部出る

    //spdlog::info("Hello");
    //spdlog::warn("Warning!");
    //spdlog::error("Error!");
    //spdlog::debug("Debug message");
    //spdlog::trace("Trace message");
    
}

void TitleScreen::Update() {
    if (InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::A)) {
        Time::Instance().SetTargetFPS(60);
    }
    if (InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::S)) {
        Time::Instance().SetTargetFPS(120);
    }
    if (InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::D)) {
        Time::Instance().SetTimeScale(2);
    }
    if (InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::F)) {
        Time::Instance().SetTimeScale(1);
    }

    // 現在の座標を取得
    sf::Vector2f pos = movingCircle.getPosition();

    float timeScale = Time::Instance().GetTimeScale();
    // 移動処理
    movingCircle.move(circleVelocity * timeScale);

    float r = movingCircle.getRadius();
    // 左端 or 右端
    if (pos.x < 0 || pos.x + r * 2 > WINDOW_WIDTH)
    {
        circleVelocity.x *= -1;

        // めり込み補正
        pos.x = std::clamp(pos.x, 0.f, WINDOW_WIDTH - r * 2);
        movingCircle.setPosition(pos);
    }
    // 上端 or 下端
    if (pos.y < 0 || pos.y + r * 2 > WINDOW_HEIGHT)
    {
        circleVelocity.y *= -1;

        // めり込み補正
        pos.y = std::clamp(pos.y, 0.f, WINDOW_HEIGHT - r * 2);
        movingCircle.setPosition(pos);
    }
}

void TitleScreen::Render(sf::RenderTarget& target) {
    // カメラのズーム適用
    target.setView(CameraManager::Instance().GetCurrentView());

	// 画像スプライトの描画
	if (testSprite) {
		target.draw(*testSprite);
	}

    // 青い円：ワールド座標 (100, 100)
    sf::CircleShape circle2(50.0f);
    circle2.setFillColor(sf::Color::Blue);
    // circle2.setPosition({ 100.0f, 100.0f }); // ワールド座標 (100, 100)
    circle2.setPosition({ WINDOW_WIDTH / 2.0f + 50.0f, WINDOW_HEIGHT / 2.0f + 50.0f });
    target.draw(circle2);


    // 赤い円
    sf::CircleShape circle(50.0f);
    circle.setFillColor(sf::Color::Red);

    // デフォルトView（1280x720）の画面中央に配置
    sf::Vector2u defaultSize = static_cast<sf::Vector2u>(target.getDefaultView().getSize());
    circle.setPosition({ (float)defaultSize.x / 2.0f - 50.0f, (float)defaultSize.y / 2.0f - 50.0f });

    target.draw(circle);

    target.draw(movingCircle);

    // カメラのズーム解除この下は適応されない
    target.setView(target.getDefaultView());

}

void TitleScreen::RenderImGui(const sf::Texture* renderTexture)
{
}
