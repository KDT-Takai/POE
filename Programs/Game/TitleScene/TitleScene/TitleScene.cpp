#include "TitleScene.h"
#include <spdlog/spdlog.h>
#include <System/Config/Config.h>
#include <System/CameraManager/CameraManager.h>
#include <System/Resource/ResourceManager/ResourceManager.h>
#include <System/Time/Time.h>
#include <System/SceneManager/SceneManager.h>
#include "../../GameScene/GameScene/GameScene.h"

TitleScene::TitleScene() {
    sceneName = "TitleScene";

    //movingCircle.setRadius(30.0f);
    //movingCircle.setFillColor(sf::Color::Green);
    //movingCircle.setPosition({ 100.0f, 100.0f }); // 初期位置
    //// 移動速度の設定 (X方向に3, Y方向に2)
    //circleVelocity = { 3.0f, 2.0f };
    // テスト描画
    auto test = ResourceManager::Instance().getTexture("Assets/Textures/title.png");
    if (test) {
		testSprite = std::make_unique<sf::Sprite>(*test);
        testSprite->setPosition({ 0,0 }); // 位置調整
    }

    //spdlog::set_level(spdlog::level::trace);
    // これで trace / debug / info / warn / error / critical 全部出る

    //spdlog::info("Hello");
    //spdlog::warn("Warning!");
    //spdlog::error("Error!");
    //spdlog::debug("Debug message");
    //spdlog::trace("Trace message");
    auto titleFont = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");

    if (titleFont) {
        pressText = std::make_unique<sf::Text>(*titleFont, "Press Space Key", 40);
        pressText->setFillColor(sf::Color::White);
        pressText->setPosition({ 450.f, 500.f });
    }
    
}

void TitleScene::Update() {
    // タイトルスクリーンに移動
	bool input = InputManager::Instance().GetKeyInput().IsGetKey(sf::Keyboard::Key::Space);
    if (input)
    {
		SceneManager::Instance().ChangeScene(GameScene::GetName());
    }

    //// 現在の座標を取得
    //sf::Vector2f pos = movingCircle.getPosition();

    //float timeScale = Time::Instance().GetTimeScale();
    //// 移動処理
    //movingCircle.move(circleVelocity * timeScale);

    //float r = movingCircle.getRadius();
    //// 左端 or 右端
    //if (pos.x < 0 || pos.x + r * 2 > WINDOW_WIDTH)
    //{
    //    circleVelocity.x *= -1;

    //    // めり込み補正
    //    pos.x = std::clamp(pos.x, 0.f, WINDOW_WIDTH - r * 2);
    //    movingCircle.setPosition(pos);
    //}
    //// 上端 or 下端
    //if (pos.y < 0 || pos.y + r * 2 > WINDOW_HEIGHT)
    //{
    //    circleVelocity.y *= -1;

    //    // めり込み補正
    //    pos.y = std::clamp(pos.y, 0.f, WINDOW_HEIGHT - r * 2);
    //    movingCircle.setPosition(pos);
    //}
}

void TitleScene::Render(sf::RenderTarget& target) {
    // カメラのズーム適用
    target.setView(CameraManager::Instance().GetCurrentView());

	// 画像スプライトの描画
	if (testSprite) {
		target.draw(*testSprite);
	}
    if (pressText) {
        target.draw(*pressText);
    }
    //// 青い円：ワールド座標 (100, 100)
    //sf::CircleShape circle2(50.0f);
    //circle2.setFillColor(sf::Color::Blue);
    //// circle2.setPosition({ 100.0f, 100.0f }); // ワールド座標 (100, 100)
    //circle2.setPosition({ WINDOW_WIDTH / 2.0f + 50.0f, WINDOW_HEIGHT / 2.0f + 50.0f });
    //target.draw(circle2);


    //// 赤い円
    //sf::CircleShape circle(50.0f);
    //circle.setFillColor(sf::Color::Red);

    //// デフォルトView（1280x720）の画面中央に配置
    //sf::Vector2u defaultSize = static_cast<sf::Vector2u>(target.getDefaultView().getSize());
    //circle.setPosition({ (float)defaultSize.x / 2.0f - 50.0f, (float)defaultSize.y / 2.0f - 50.0f });

    //target.draw(circle);

    //target.draw(movingCircle);

    // カメラのズーム解除この下は適応されない
    target.setView(target.getDefaultView());

}

void TitleScene::RenderImGui(const sf::Texture* renderTexture)
{
}
