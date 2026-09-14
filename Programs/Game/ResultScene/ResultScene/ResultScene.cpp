#include "ResultScene.h"
#include <spdlog/spdlog.h>
#include <System/Input/InputManager.h>
#include <System/SceneManager/SceneManager.h>
#include <System/Resource/ResourceManager/ResourceManager.h>
#include <System/Campaign/CampaignManager.h>
#include "../../GameScene/GameScene/GameScene.h"
#include "../../TitleScene/TitleScene/TitleScene.h"

ResultScene::ResultScene() {
    sceneName = "ResultScene";
    m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
}

void ResultScene::Update() {
    auto& keyInput = InputManager::Instance().GetKeyInput();
    auto& campaign = CampaignManager::Instance();

    if (campaign.IsHardcore()) {
        if (keyInput.IsGetKey(sf::Keyboard::Key::Space) || keyInput.IsGetKey(sf::Keyboard::Key::Escape)) {
            CampaignManager::DeleteSaveFile();
            campaign.ResetCampaign();
            SceneManager::Instance().ChangeScene(TitleScene::GetName());
        }
        return;
    }

    if (keyInput.IsGetKey(sf::Keyboard::Key::Space)) {
        campaign.ReturnToLastTown();
        campaign.SaveToDisk();
        SceneManager::Instance().ChangeScene(GameScene::GetName());
    }
    else if (keyInput.IsGetKey(sf::Keyboard::Key::Escape)) {
        campaign.ResetCampaign();
        SceneManager::Instance().ChangeScene(TitleScene::GetName());
    }
}

void ResultScene::Render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());
    target.clear(sf::Color(20, 10, 10));

    if (!m_font) return;

    sf::Vector2u size = target.getSize();

    bool isHardcore = CampaignManager::Instance().IsHardcore();

    sf::Text title(*m_font, isHardcore ? "CHARACTER LOST" : "YOU DIED", 56);
    title.setFillColor(sf::Color(200, 30, 30));
    title.setOutlineColor(sf::Color::Black);
    title.setOutlineThickness(3.0f);
    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setOrigin({ titleBounds.position.x + titleBounds.size.x / 2.0f, titleBounds.position.y + titleBounds.size.y / 2.0f });
    title.setPosition({ size.x / 2.0f, size.y / 2.0f - 60.0f });
    target.draw(title);

    sf::Text hint(*m_font, isHardcore ? "Hardcore: this save has been deleted. Press any key to return to Title." : "Space: Continue from Town   /   Esc: Return to Title", 22);
    hint.setFillColor(sf::Color::White);
    sf::FloatRect hintBounds = hint.getLocalBounds();
    hint.setOrigin({ hintBounds.position.x + hintBounds.size.x / 2.0f, hintBounds.position.y + hintBounds.size.y / 2.0f });
    hint.setPosition({ size.x / 2.0f, size.y / 2.0f + 30.0f });
    target.draw(hint);
}

void ResultScene::RenderImGui(const sf::Texture* renderTexture)
{
}
