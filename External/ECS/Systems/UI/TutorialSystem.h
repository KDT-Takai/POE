#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"
#include "System/Campaign/CampaignManager.h"

struct TutorialContext {
    bool isTown = false;
    float playerMovedDistance = 0.0f;
    bool anyNpcPanelOpen = false;
    bool inventoryOpen = false;
    bool skillGemsOpen = false;
    bool atlasOpen = false;
    bool hasActiveMapAttempt = false;
    bool notableEnemiesCleared = false;
    bool passiveTreeOpen = false;
    bool hidePanel = false;
};

class TutorialSystem {
public:
    static constexpr int kStepCount = 10;

    TutorialSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    bool IsActive() const { return CampaignManager::Instance().GetTutorialStep() < kStepCount || m_finishTimer > 0.0f; }

    bool IsPointInPanel(sf::Vector2f point) const {
        return IsActive() && !m_hidden && PanelRect().contains(point);
    }

    void Update(const TutorialContext& ctx, float dt) {
        m_time += dt;
        m_hidden = ctx.hidePanel;
        auto& campaign = CampaignManager::Instance();
        int step = campaign.GetTutorialStep();

        if (m_finishTimer > 0.0f) m_finishTimer -= dt;
        if (step >= kStepCount) return;

        if (!m_hidden && InputManager::Instance().GetMouseInput().IsGetMouse(sf::Mouse::Button::Left) &&
            SkipRect().contains(InputManager::Instance().GetMouseInput().GetMousePointF())) {
            campaign.SetTutorialStep(kStepCount);
            m_completedTimer = 0.0f;
            return;
        }

        if (m_completedTimer > 0.0f) {
            m_completedTimer -= dt;
            if (m_completedTimer <= 0.0f) {
                campaign.SetTutorialStep(step + 1);
                if (step + 1 >= kStepCount) m_finishTimer = kFinishDisplayTime;
            }
            return;
        }

        if (IsStepSatisfied(step, ctx)) m_completedTimer = kCompletedDisplayTime;
    }

    void Render(sf::RenderTarget& target) {
        if (!m_font || m_hidden) return;
        int step = CampaignManager::Instance().GetTutorialStep();

        if (step >= kStepCount) {
            if (m_finishTimer <= 0.0f) return;
            float alpha = std::clamp(m_finishTimer / 0.6f, 0.0f, 1.0f);
            sf::FloatRect rect = PanelRect();
            DrawPanelFrame(target, { rect.position, { rect.size.x, 56.0f } }, alpha);
            DrawText(target, "チュートリアル完了！ 良い冒険を", 18, { rect.position.x + 16.0f, rect.position.y + 16.0f },
                sf::Color(140, 230, 140, static_cast<std::uint8_t>(255 * alpha)));
            return;
        }

        const StepText text = TextForStep(step);
        sf::FloatRect rect = PanelRect();
        DrawPanelFrame(target, rect, 1.0f);

        float x = rect.position.x + 16.0f;
        float y = rect.position.y + 12.0f;
        DrawText(target, "チュートリアル  " + std::to_string(step + 1) + " / " + std::to_string(kStepCount), 13, { x, y }, sf::Color(180, 170, 150));

        sf::RectangleShape progressBack({ rect.size.x - 32.0f, 3.0f });
        progressBack.setPosition({ x, y + 22.0f });
        progressBack.setFillColor(sf::Color(60, 55, 50));
        target.draw(progressBack);
        float progress = static_cast<float>(step + (m_completedTimer > 0.0f ? 1 : 0)) / kStepCount;
        sf::RectangleShape progressFill({ (rect.size.x - 32.0f) * progress, 3.0f });
        progressFill.setPosition({ x, y + 22.0f });
        progressFill.setFillColor(sf::Color(232, 196, 120));
        target.draw(progressFill);

        bool completed = m_completedTimer > 0.0f;
        float pulse = 0.5f + 0.5f * std::sin(m_time * 4.0f);
        sf::Color titleColor = completed ? sf::Color(140, 230, 140)
            : sf::Color(255, 220, 130, static_cast<std::uint8_t>(200 + 55 * pulse));
        DrawText(target, (completed ? "★ " : "◆ ") + text.title, 18, { x, y + 34.0f }, titleColor);
        DrawText(target, text.body, 14, { x, y + 62.0f }, sf::Color(220, 215, 205));

        sf::FloatRect skip = SkipRect();
        bool hoverSkip = skip.contains(InputManager::Instance().GetMouseInput().GetMousePointF());
        DrawText(target, "スキップ", 12, { skip.position.x + 4.0f, skip.position.y + 1.0f },
            hoverSkip ? sf::Color(255, 255, 160) : sf::Color(150, 145, 135));
    }

private:
    struct StepText {
        std::string title;
        std::string body;
    };

    static constexpr float kCompletedDisplayTime = 0.9f;
    static constexpr float kFinishDisplayTime = 3.0f;
    static constexpr float kMoveDistanceGoal = 250.0f;

    static std::string Key(GameAction action) { return KeyToString(KeyBindings::Instance().Get(action)); }

    static bool IsStepSatisfied(int step, const TutorialContext& ctx) {
        switch (step) {
        case 0: return ctx.playerMovedDistance >= kMoveDistanceGoal;
        case 1: return ctx.anyNpcPanelOpen;
        case 2: return ctx.inventoryOpen;
        case 3: return ctx.skillGemsOpen;
        case 4: return ctx.atlasOpen || ctx.hasActiveMapAttempt;
        case 5: return ctx.hasActiveMapAttempt;
        case 6: return !ctx.isTown;
        case 7: return !ctx.isTown && ctx.notableEnemiesCleared;
        case 8: return ctx.isTown;
        case 9: return ctx.passiveTreeOpen;
        default: return true;
        }
    }

    static StepText TextForStep(int step) {
        switch (step) {
        case 0: return { "移動してみよう",
            Key(GameAction::MoveUp) + Key(GameAction::MoveLeft) + Key(GameAction::MoveDown) + Key(GameAction::MoveRight)
            + " キーで移動。\n" + Key(GameAction::Roll) + " キーで回避ロール。" };
        case 1: return { "町の人に話しかけよう",
            "名前が表示されている人物に近づき、\n左クリックで話しかけよう。" };
        case 2: return { "インベントリを開こう",
            Key(GameAction::ToggleInventory) + " キーでバッグと装備を確認。\nアイテムはドラッグで装備・移動できる。" };
        case 3: return { "スキルジェムを確認しよう",
            Key(GameAction::ToggleSkillGems) + " キーでスキルジェム画面を開く。\n空きスロットにジェムをセットできる。" };
        case 4: return { "マップデバイスを調べよう",
            "町の「マップデバイス」に近づいて\n左クリックでAtlasを開こう。" };
        case 5: return { "ウェイストーンでマップを開こう",
            "Atlasのノードを選び、バッグのウェイストーンを\n現れたソケットへドラッグ&ドロップ。" };
        case 6: return { "マップへ入ろう",
            "デバイスの周りに出たポータルを\n左クリックしてマップへ入場。" };
        case 7: return { "レア敵とボスを倒そう",
            "左クリック/" + Key(GameAction::Skill1) + Key(GameAction::Skill2) + Key(GameAction::Skill3)
            + " 等でスキル攻撃。Tabで全体マップ。\nレア敵とボスを全て倒すと帰還ポータルが出る。" };
        case 8: return { "町へ帰還しよう",
            "帰還用ポータルを左クリックで町へ。\nスキル欄右のボタンでもポータルを詠唱できる。" };
        case 9: return { "パッシブツリーを開こう",
            "レベルアップで得たポイントは\n" + Key(GameAction::TogglePassiveTree) + " キーのパッシブツリーで割り振ろう。" };
        default: return { "", "" };
        }
    }

    static sf::FloatRect PanelRect() { return sf::FloatRect({ 16.0f, 130.0f }, { 370.0f, 112.0f }); }
    static sf::FloatRect SkipRect() {
        sf::FloatRect panel = PanelRect();
        return sf::FloatRect({ panel.position.x + panel.size.x - 64.0f, panel.position.y + 8.0f }, { 56.0f, 18.0f });
    }

    void DrawPanelFrame(sf::RenderTarget& target, sf::FloatRect rect, float alpha) const {
        sf::RectangleShape back(rect.size);
        back.setPosition(rect.position);
        back.setFillColor(sf::Color(12, 10, 10, static_cast<std::uint8_t>(190 * alpha)));
        back.setOutlineColor(sf::Color(150, 120, 70, static_cast<std::uint8_t>(200 * alpha)));
        back.setOutlineThickness(1.5f);
        target.draw(back);

        sf::RectangleShape accent({ 3.0f, rect.size.y });
        accent.setPosition(rect.position);
        accent.setFillColor(sf::Color(232, 196, 120, static_cast<std::uint8_t>(255 * alpha)));
        target.draw(accent);
    }

    void DrawText(sf::RenderTarget& target, const std::string& utf8, unsigned int size, sf::Vector2f pos, sf::Color color) const {
        sf::Text text(*m_font, sf::String::fromUtf8(utf8.begin(), utf8.end()), size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color(0, 0, 0, color.a));
        text.setOutlineThickness(1.0f);
        text.setPosition(pos);
        target.draw(text);
    }

    std::shared_ptr<sf::Font> m_font;
    float m_completedTimer = 0.0f;
    float m_finishTimer = 0.0f;
    float m_time = 0.0f;
    bool m_hidden = false;
};
