#include "TitleScene.h"
#include <cmath>
#include <algorithm>
#include <System/Config/Config.h>
#include <System/Resource/ResourceManager/ResourceManager.h>
#include <System/Time/Time.h>
#include <System/SceneManager/SceneManager.h>
#include <System/Campaign/CampaignManager.h>
#include <System/Input/InputManager.h>
#include "../../GameScene/GameScene/GameScene.h"

namespace {
    constexpr int kEmberCount = 90;
    constexpr float kBackgroundScale = 1.06f;
    constexpr float kBobAmplitude = 12.0f;
    constexpr float kFadeInDuration = 1.2f;
    const sf::Color kGold(232, 196, 120);
}

TitleScene::TitleScene() {
    sceneName = "TitleScene";

    auto texture = ResourceManager::Instance().getTexture("Assets/Textures/title.png");
    if (texture) {
        m_background = std::make_unique<sf::Sprite>(*texture);
        sf::Vector2u texSize = texture->getSize();
        m_background->setOrigin({ texSize.x / 2.0f, texSize.y / 2.0f });
        m_background->setScale({ kBackgroundScale, kBackgroundScale });
    }
    m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");

    m_embers.reserve(kEmberCount);
    for (int i = 0; i < kEmberCount; ++i) SpawnEmber(true);
}

void TitleScene::SpawnEmber(bool randomHeight) {
    std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(WINDOW_WIDTH));
    std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(WINDOW_HEIGHT));
    std::uniform_real_distribution<float> speedDist(18.0f, 60.0f);
    std::uniform_real_distribution<float> lifeDist(4.0f, 9.0f);
    std::uniform_real_distribution<float> sizeDist(1.0f, 2.8f);
    std::uniform_real_distribution<float> phaseDist(0.0f, 6.28f);

    Ember ember;
    ember.pos = { xDist(m_rng), randomHeight ? yDist(m_rng) : WINDOW_HEIGHT + 10.0f };
    ember.vel = { 0.0f, -speedDist(m_rng) };
    ember.maxLife = lifeDist(m_rng);
    ember.life = randomHeight ? ember.maxLife * 0.5f : ember.maxLife;
    ember.size = sizeDist(m_rng);
    ember.phase = phaseDist(m_rng);
    m_embers.push_back(ember);
}

void TitleScene::Update() {
    float dt = static_cast<float>(Time::Instance().GetDeltaTime());
    m_elapsed += dt;

    for (auto& ember : m_embers) {
        ember.life -= dt;
        ember.pos.y += ember.vel.y * dt;
        ember.pos.x += std::sin(m_elapsed * 1.3f + ember.phase) * 14.0f * dt;
    }
    m_embers.erase(std::remove_if(m_embers.begin(), m_embers.end(),
        [](const Ember& e) { return e.life <= 0.0f || e.pos.y < -10.0f; }), m_embers.end());
    while (static_cast<int>(m_embers.size()) < kEmberCount) SpawnEmber(false);

    if (m_elapsed < kFadeInDuration * 0.5f) return;

    auto& keyInput = InputManager::Instance().GetKeyInput();
    bool hasSave = CampaignManager::SaveFileExists();

    if (keyInput.IsGetKey(sf::Keyboard::Key::H)) {
        m_hardcoreSelected = !m_hardcoreSelected;
    }
    else if (keyInput.IsGetKey(sf::Keyboard::Key::Space)) {
        auto& campaign = CampaignManager::Instance();
        if (!campaign.LoadFromDisk()) {
            campaign.ResetCampaign();
            campaign.SetHardcore(m_hardcoreSelected);
        }
        SceneManager::Instance().ChangeScene(GameScene::GetName());
    }
    else if (keyInput.IsGetKey(sf::Keyboard::Key::N) && hasSave) {
        auto& campaign = CampaignManager::Instance();
        campaign.ResetCampaign();
        campaign.SetHardcore(m_hardcoreSelected);
        SceneManager::Instance().ChangeScene(GameScene::GetName());
    }
}

void TitleScene::DrawCenteredText(sf::RenderTarget& target, const std::string& utf8, unsigned int size,
    float y, sf::Color fill, float letterSpacing) const {
    sf::Text text(*m_font, sf::String::fromUtf8(utf8.begin(), utf8.end()), size);
    text.setLetterSpacing(letterSpacing);
    text.setFillColor(fill);
    text.setOutlineColor(sf::Color(0, 0, 0, fill.a));
    text.setOutlineThickness(2.0f);
    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
    text.setPosition({ WINDOW_WIDTH / 2.0f, y });
    target.draw(text);
}

void TitleScene::Render(sf::RenderTarget& target) {
    target.setView(sf::View(sf::FloatRect({ 0.0f, 0.0f }, { static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT) })));
    const float width = static_cast<float>(WINDOW_WIDTH);
    const float height = static_cast<float>(WINDOW_HEIGHT);

    if (m_background) {
        float bob = std::sin(m_elapsed * 0.6f) * kBobAmplitude;
        float drift = std::sin(m_elapsed * 0.23f) * kBobAmplitude * 0.6f;
        m_background->setPosition({ width / 2.0f + drift, height / 2.0f + bob });
        target.draw(*m_background);
    }

    sf::VertexArray shade(sf::PrimitiveType::TriangleStrip, 4);
    const float shadeTop = height * 0.62f;
    shade[0] = sf::Vertex{ { 0.0f, shadeTop }, sf::Color(0, 0, 0, 0) };
    shade[1] = sf::Vertex{ { width, shadeTop }, sf::Color(0, 0, 0, 0) };
    shade[2] = sf::Vertex{ { 0.0f, height }, sf::Color(0, 0, 0, 220) };
    shade[3] = sf::Vertex{ { width, height }, sf::Color(0, 0, 0, 220) };
    target.draw(shade);

    sf::CircleShape emberShape;
    for (const auto& ember : m_embers) {
        float lifeRatio = std::clamp(ember.life / ember.maxLife, 0.0f, 1.0f);
        float flicker = 0.6f + 0.4f * std::sin(m_elapsed * 8.0f + ember.phase * 3.0f);
        auto alpha = static_cast<std::uint8_t>(std::clamp(255.0f * lifeRatio * flicker, 0.0f, 255.0f));
        emberShape.setRadius(ember.size);
        emberShape.setOrigin({ ember.size, ember.size });
        emberShape.setPosition(ember.pos);
        emberShape.setFillColor(sf::Color(255, 150 + static_cast<int>(80 * lifeRatio), 60, alpha));
        target.draw(emberShape, sf::BlendAdd);
    }

    if (m_font) {
        const float promptY = height * 0.80f;
        float pulse = 0.5f + 0.5f * std::sin(m_elapsed * 2.4f);
        auto promptAlpha = static_cast<std::uint8_t>(140 + 115 * pulse);
        sf::Color promptColor = kGold;
        promptColor.a = promptAlpha;

        sf::Text measure(*m_font, "PRESS SPACE", 30);
        measure.setLetterSpacing(3.0f);
        float halfWidth = measure.getLocalBounds().size.x / 2.0f;

        const float lineLength = 150.0f;
        const float gap = 28.0f;
        for (int side : { -1, 1 }) {
            float innerX = width / 2.0f + side * (halfWidth + gap);
            float outerX = innerX + side * lineLength;
            sf::VertexArray line(sf::PrimitiveType::Lines, 2);
            line[0] = sf::Vertex{ { innerX, promptY }, promptColor };
            line[1] = sf::Vertex{ { outerX, promptY }, sf::Color(promptColor.r, promptColor.g, promptColor.b, 0) };
            target.draw(line);

            sf::RectangleShape diamond({ 7.0f, 7.0f });
            diamond.setOrigin({ 3.5f, 3.5f });
            diamond.setRotation(sf::degrees(45.0f));
            diamond.setPosition({ innerX - side * 8.0f, promptY });
            diamond.setFillColor(promptColor);
            target.draw(diamond);
        }
        DrawCenteredText(target, "PRESS SPACE", 30, promptY, promptColor, 3.0f);

        sf::Color subColor(210, 200, 185, 220);
        if (CampaignManager::SaveFileExists()) {
            DrawCenteredText(target, "Space  続きから          N  ニューゲーム", 18, promptY + 44.0f, subColor);
        }
        std::string modeLine = std::string("H  ニューゲームのモード : ") + (m_hardcoreSelected ? "ハードコア" : "ソフトコア");
        sf::Color modeColor = m_hardcoreSelected ? sf::Color(235, 90, 80, 230) : sf::Color(170, 165, 155, 200);
        DrawCenteredText(target, modeLine, 16, promptY + 74.0f, modeColor);
    }

    if (m_elapsed < kFadeInDuration) {
        float fade = 1.0f - m_elapsed / kFadeInDuration;
        sf::RectangleShape black({ width, height });
        black.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(255 * fade)));
        target.draw(black);
    }

    target.setView(target.getDefaultView());
}

void TitleScene::RenderImGui(const sf::Texture* renderTexture) {
}
