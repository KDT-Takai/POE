#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <iomanip>
#include <sstream>
#include <cmath> // std::abs用
#include "../../Registry/Registry.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/PlayerSkill/PlayerSkill.h"
#include "../../Components/Tags/Player/Player.h"
#include "System/Resource/ResourceManager/ResourceManager.h"

class UISystem {
private:
    std::shared_ptr<sf::Font> m_font;

public:
    UISystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!m_font) return;
        
        auto view = registry.View<PlayerTag>();
        if (view.empty()) return;

        auto playerEntity = view[0];

        if (!registry.HasComponent<CharacterStatsComponent>(playerEntity)) return;
        auto& stats = registry.GetComponent<CharacterStatsComponent>(playerEntity);

        bool hasSkills = registry.HasComponent<PlayerSkill>(playerEntity);
        PlayerSkill* skillComp = hasSkills ? &registry.GetComponent<PlayerSkill>(playerEntity) : nullptr;

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::Vector2u winSize = target.getSize();
        float width = static_cast<float>(winSize.x);
        float height = static_cast<float>(winSize.y);

        DrawOrb(target, 80.0f, height - 80.0f, 45.0f, sf::Color(180, 0, 0), stats.currentHP, stats.maxHP, "HP");
        DrawOrb(target, width - 80.0f, height - 80.0f, 45.0f, sf::Color(0, 80, 200), stats.currentMP, stats.maxMP, "MP");

        float slotSize = 50.0f;
        float gap = 8.0f;           // スキル間の隙間
        float rollGap = 20.0f;      // ロール(Space)とスキル(Z~)の間の大きめの隙間
        int skillCount = 5;         // Z, X, C, V, F

        float totalBarWidth = slotSize + rollGap + (slotSize * skillCount) + (gap * (skillCount - 1));

        float startX = (width - totalBarWidth) / 2.0f;
        float startY = height - 70.0f;

        float rollX = startX;

        // 枠とキー名
        DrawSlotFrame(target, rollX, startY, slotSize, "Space");

        // アイコン (水色)
        DrawIcon(target, rollX, startY, slotSize, sf::Color(0, 200, 255));

        // クールダウン (Statsの情報を利用)
        if (stats.rollCooldownTimer > 0.0f) {
            DrawCooldownOverlay(target, rollX, startY, slotSize, stats.rollCooldownTimer, stats.rollCooldownMax);
        }

        std::string keyNames[] = { "E", "Q", "R", "V", "F" };

        float skillStartX = rollX + slotSize + rollGap;

        for (int i = 0; i < skillCount; ++i) {
            float x = skillStartX + i * (slotSize + gap);

            DrawSlotFrame(target, x, startY, slotSize, keyNames[i]);

            if (hasSkills && skillComp && skillComp->skills[i].isValid) {
                auto& skill = skillComp->skills[i];

                sf::Color iconColor = sf::Color::Green;
                if (skill.behaviorType == SkillBehaviorType::Dash) iconColor = sf::Color::Cyan;
                else if (skill.behaviorType == SkillBehaviorType::Melee) iconColor = sf::Color(200, 50, 50);

                DrawIcon(target, x, startY, slotSize, iconColor);

                if (skill.currentCooldown > 0.0f) {
                    DrawCooldownOverlay(target, x, startY, slotSize, skill.currentCooldown, skill.cooldownTime);
                }
            }
        }

        DrawStats(target, stats);

        target.setView(oldView);
    }

private:
    // 枠とキー名を描画するヘルパー
    void DrawSlotFrame(sf::RenderTarget& target, float x, float y, float size, const std::string& keyName) {
        // 背景枠
        sf::RectangleShape slot(sf::Vector2f(size, size));
        slot.setPosition({ x, y });
        slot.setFillColor(sf::Color(20, 20, 20));
        slot.setOutlineColor(sf::Color(100, 100, 100));
        slot.setOutlineThickness(2.0f);
        target.draw(slot);

        sf::Text keyText(*m_font, keyName, 12);

        // 中央揃え計算
        sf::FloatRect bounds = keyText.getLocalBounds();
        keyText.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
        keyText.setPosition({ x + size / 2.0f, y - 12.0f }); // 枠の少し上

        keyText.setFillColor(sf::Color::Yellow);
        keyText.setOutlineColor(sf::Color::Black);
        keyText.setOutlineThickness(1.0f);
        target.draw(keyText);
    }

    void DrawIcon(sf::RenderTarget& target, float x, float y, float size, sf::Color color) {
        float padding = 4.0f;
        sf::RectangleShape icon(sf::Vector2f(size - padding, size - padding));
        icon.setPosition({ x + padding / 2.0f, y + padding / 2.0f });
        icon.setFillColor(color);
        target.draw(icon);
    }

    void DrawCooldownOverlay(sf::RenderTarget& target, float x, float y, float size, float current, float max) {
        if (max <= 0.0f) max = 1.0f; // ゼロ除算防止
        float ratio = current / max;
        float cdHeight = size * ratio;

        sf::RectangleShape cdOverlay(sf::Vector2f(size, cdHeight));
        cdOverlay.setPosition({ x, y });
        cdOverlay.setFillColor(sf::Color(0, 0, 0, 180));
        target.draw(cdOverlay);

        // 数値テキスト
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << current;

        sf::Text timeText(*m_font, ss.str(), 14);
        sf::FloatRect bounds = timeText.getLocalBounds();

        // 枠の中央に配置
        timeText.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
        timeText.setPosition({ x + size / 2.0f, y + size / 2.0f });

        timeText.setFillColor(sf::Color::White);
        timeText.setOutlineColor(sf::Color::Black);
        timeText.setOutlineThickness(1.0f);
        target.draw(timeText);
    }

    // HP/MPオーブ描画
    void DrawOrb(sf::RenderTarget& target, float cx, float cy, float r, sf::Color color, float current, float max, std::string label) {
        // 外枠
        sf::CircleShape border(r);
        border.setOrigin({ r, r });
        border.setPosition({ cx, cy });
        border.setFillColor(sf::Color(10, 10, 10));
        border.setOutlineThickness(4.0f);
        border.setOutlineColor(sf::Color(50, 50, 50));
        target.draw(border);

        // 中身（液体）
        if (max > 0) {
            float ratio = current / max;
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;

            sf::CircleShape fluid(r * ratio);
            fluid.setOrigin({ r * ratio, r * ratio });
            fluid.setPosition({ cx, cy + (r - r * ratio) });
            fluid.setFillColor(color);
            target.draw(fluid);
        }

        // ラベル (HP/MP)
        sf::Text text(*m_font, label, 14);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
        text.setPosition({ cx, cy + r + 15.0f });
        text.setFillColor(sf::Color::White);
        target.draw(text);

        // 数値 (100/100)
        std::string valStr = std::to_string((int)current) + "/" + std::to_string((int)max);
        sf::Text valText(*m_font, valStr, 12);
        bounds = valText.getLocalBounds();
        valText.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
        valText.setPosition({ cx, cy });
        valText.setFillColor(sf::Color::White);
        valText.setOutlineColor(sf::Color::Black);
        valText.setOutlineThickness(1.0f);
        target.draw(valText);
    }

    // ステータス情報の描画
    void DrawStats(sf::RenderTarget& target, CharacterStatsComponent& stats) {
        std::string info =
            "Lv: " + std::to_string(stats.level) + "\n" +
            "STR: " + std::to_string(stats.str) + "\n" +
            "DEX: " + std::to_string(stats.dex) + "\n" +
            "INT: " + std::to_string(stats.intelligence);

        sf::Text text(*m_font, info, 14);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ 10.0f, 10.0f });
        target.draw(text);
    }
};