#include "TitleScene.h"
#include <spdlog/spdlog.h>
#include <System/Config/Config.h>
#include <System/CameraManager/CameraManager.h>
#include <System/Resource/ResourceManager/ResourceManager.h>
#include <System/Time/Time.h>
#include <System/SceneManager/SceneManager.h>
#include <System/Campaign/CampaignManager.h>
#include "../../GameScene/GameScene/GameScene.h"

TitleScene::TitleScene() {
    sceneName = "TitleScene";

    //movingCircle.setRadius(30.0f);
    //movingCircle.setFillColor(sf::Color::Green);
    //movingCircle.setPosition({ 100.0f, 100.0f }); // �����ʒu
    //// �ړ����x�̐ݒ� (X������3, Y������2)
    //circleVelocity = { 3.0f, 2.0f };
    // �e�X�g�`��
    auto test = ResourceManager::Instance().getTexture("Assets/Textures/title.png");
    if (test) {
		testSprite = std::make_unique<sf::Sprite>(*test);
        testSprite->setPosition({ 0,0 }); // �ʒu����
    }

    //spdlog::set_level(spdlog::level::trace);
    // ����� trace / debug / info / warn / error / critical �S���o��

    //spdlog::info("Hello");
    //spdlog::warn("Warning!");
    //spdlog::error("Error!");
    //spdlog::debug("Debug message");
    //spdlog::trace("Trace message");
    auto titleFont = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");

    if (titleFont) {
        pressText = std::make_unique<sf::Text>(*titleFont, "", 40);
        pressText->setFillColor(sf::Color::White);
        pressText->setPosition({ 450.f, 500.f });

        modeText = std::make_unique<sf::Text>(*titleFont, "", 26);
        modeText->setFillColor(sf::Color(255, 220, 120));
        modeText->setPosition({ 450.f, 560.f });
    }
    UpdatePromptText();
}

void TitleScene::UpdatePromptText() {
    bool hasSave = CampaignManager::SaveFileExists();
    if (pressText) {
        std::string prompt = hasSave ? "Press Space: Continue   /   Press N: New Game" : "Press Space Key";
        pressText->setString(prompt);
    }
    if (modeText) {
        std::string mode = std::string("New Game Mode: ") + (m_hardcoreSelected ? "HARDCORE" : "Softcore") + "   (H to toggle)";
        modeText->setString(mode);
    }
}

void TitleScene::Update() {
    auto& keyInput = InputManager::Instance().GetKeyInput();

    if (keyInput.IsGetKey(sf::Keyboard::Key::H))
    {
        m_hardcoreSelected = !m_hardcoreSelected;
        UpdatePromptText();
    }
    else if (keyInput.IsGetKey(sf::Keyboard::Key::Space))
    {
        auto& campaign = CampaignManager::Instance();
        if (!campaign.LoadFromDisk()) {
            campaign.ResetCampaign();
            campaign.SetHardcore(m_hardcoreSelected);
        }
		SceneManager::Instance().ChangeScene(GameScene::GetName());
    }
    else if (keyInput.IsGetKey(sf::Keyboard::Key::N) && CampaignManager::SaveFileExists())
    {
        auto& campaign = CampaignManager::Instance();
        campaign.ResetCampaign();
        campaign.SetHardcore(m_hardcoreSelected);
        SceneManager::Instance().ChangeScene(GameScene::GetName());
    }

    //// ���݂̍��W���擾
    //sf::Vector2f pos = movingCircle.getPosition();

    //float timeScale = Time::Instance().GetTimeScale();
    //// �ړ�����
    //movingCircle.move(circleVelocity * timeScale);

    //float r = movingCircle.getRadius();
    //// ���[ or �E�[
    //if (pos.x < 0 || pos.x + r * 2 > WINDOW_WIDTH)
    //{
    //    circleVelocity.x *= -1;

    //    // �߂荞�ݕ␳
    //    pos.x = std::clamp(pos.x, 0.f, WINDOW_WIDTH - r * 2);
    //    movingCircle.setPosition(pos);
    //}
    //// ��[ or ���[
    //if (pos.y < 0 || pos.y + r * 2 > WINDOW_HEIGHT)
    //{
    //    circleVelocity.y *= -1;

    //    // �߂荞�ݕ␳
    //    pos.y = std::clamp(pos.y, 0.f, WINDOW_HEIGHT - r * 2);
    //    movingCircle.setPosition(pos);
    //}
}

void TitleScene::Render(sf::RenderTarget& target) {
    // �J�����̃Y�[���K�p
    target.setView(CameraManager::Instance().GetCurrentView());

	// �摜�X�v���C�g�̕`��
	if (testSprite) {
		target.draw(*testSprite);
	}
    if (pressText) {
        target.draw(*pressText);
    }
    if (modeText) {
        target.draw(*modeText);
    }
    //// ���~�F���[���h���W (100, 100)
    //sf::CircleShape circle2(50.0f);
    //circle2.setFillColor(sf::Color::Blue);
    //// circle2.setPosition({ 100.0f, 100.0f }); // ���[���h���W (100, 100)
    //circle2.setPosition({ WINDOW_WIDTH / 2.0f + 50.0f, WINDOW_HEIGHT / 2.0f + 50.0f });
    //target.draw(circle2);


    //// �Ԃ��~
    //sf::CircleShape circle(50.0f);
    //circle.setFillColor(sf::Color::Red);

    //// �f�t�H���gView�i1280x720�j�̉�ʒ����ɔz�u
    //sf::Vector2u defaultSize = static_cast<sf::Vector2u>(target.getDefaultView().getSize());
    //circle.setPosition({ (float)defaultSize.x / 2.0f - 50.0f, (float)defaultSize.y / 2.0f - 50.0f });

    //target.draw(circle);

    //target.draw(movingCircle);

    // �J�����̃Y�[���������̉��͓K������Ȃ�
    target.setView(target.getDefaultView());

}

void TitleScene::RenderImGui(const sf::Texture* renderTexture)
{
}
