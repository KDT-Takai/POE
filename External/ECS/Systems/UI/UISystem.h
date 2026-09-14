#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <iomanip>
#include <sstream>
#include <cmath>
#include "../../Registry/Registry.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/PlayerSkill/PlayerSkill.h"
#include "../../Components/Tags/Player/Player.h"
#include "../../Components/Combat/StatusEffects.h"
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

        if (registry.HasComponent<StatusEffectsComponent>(playerEntity)) {
            auto& fx = registry.GetComponent<StatusEffectsComponent>(playerEntity);
            DrawDebuffIcons(target, 160.0f, height - 140.0f, fx);
        }

        float slotSize = 50.0f;
        float gap = 8.0f;           // �X�L���Ԃ̌���
        float rollGap = 20.0f;      // ���[��(Space)�ƃX�L��(Z~)�̊Ԃ̑傫�߂̌���
        int skillCount = 5;         // Z, X, C, V, F

        float totalBarWidth = slotSize + rollGap + (slotSize * skillCount) + (gap * (skillCount - 1));

        float startX = (width - totalBarWidth) / 2.0f;
        float startY = height - 70.0f;

        float rollX = startX;

        // �g�ƃL�[��
        DrawSlotFrame(target, rollX, startY, slotSize, "Space");

        // �A�C�R�� (���F)
        DrawIcon(target, rollX, startY, slotSize, sf::Color(0, 200, 255));

        // �N�[���_�E�� (Stats�̏��𗘗p)
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
    // �g�ƃL�[����`�悷��w���p�[
    void DrawSlotFrame(sf::RenderTarget& target, float x, float y, float size, const std::string& keyName) {
        // �w�i�g
        sf::RectangleShape slot(sf::Vector2f(size, size));
        slot.setPosition({ x, y });
        slot.setFillColor(sf::Color(20, 20, 20));
        slot.setOutlineColor(sf::Color(100, 100, 100));
        slot.setOutlineThickness(2.0f);
        target.draw(slot);

        sf::Text keyText(*m_font, keyName, 12);

        // ���������v�Z
        sf::FloatRect bounds = keyText.getLocalBounds();
        keyText.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
        keyText.setPosition({ x + size / 2.0f, y - 12.0f }); // �g�̏�����

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
        if (max <= 0.0f) max = 1.0f; // �[�����Z�h�~
        float ratio = current / max;
        float cdHeight = size * ratio;

        sf::RectangleShape cdOverlay(sf::Vector2f(size, cdHeight));
        cdOverlay.setPosition({ x, y });
        cdOverlay.setFillColor(sf::Color(0, 0, 0, 180));
        target.draw(cdOverlay);

        // ���l�e�L�X�g
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << current;

        sf::Text timeText(*m_font, ss.str(), 14);
        sf::FloatRect bounds = timeText.getLocalBounds();

        // �g�̒����ɔz�u
        timeText.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
        timeText.setPosition({ x + size / 2.0f, y + size / 2.0f });

        timeText.setFillColor(sf::Color::White);
        timeText.setOutlineColor(sf::Color::Black);
        timeText.setOutlineThickness(1.0f);
        target.draw(timeText);
    }

    // HP/MP�I�[�u�`��
    void DrawOrb(sf::RenderTarget& target, float cx, float cy, float r, sf::Color color, float current, float max, std::string label) {
        // �O�g
        sf::CircleShape border(r);
        border.setOrigin({ r, r });
        border.setPosition({ cx, cy });
        border.setFillColor(sf::Color(10, 10, 10));
        border.setOutlineThickness(4.0f);
        border.setOutlineColor(sf::Color(50, 50, 50));
        target.draw(border);

        // ���g�i�t�́j
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

        // ���x�� (HP/MP)
        sf::Text text(*m_font, label, 14);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
        text.setPosition({ cx, cy + r + 15.0f });
        text.setFillColor(sf::Color::White);
        target.draw(text);

        // ���l (100/100)
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

    void DrawDebuffIcons(sf::RenderTarget& target, float startX, float y, const StatusEffectsComponent& fx) {
        float size = 28.0f;
        float gap = 6.0f;
        int col = 0;

        auto drawIcon = [&](const std::string& label, sf::Color color, float remaining) {
            float x = startX + col * (size + gap);
            sf::RectangleShape icon(sf::Vector2f(size, size));
            icon.setPosition({ x, y });
            icon.setFillColor(color);
            icon.setOutlineColor(sf::Color::Black);
            icon.setOutlineThickness(1.5f);
            target.draw(icon);

            sf::Text text(*m_font, label, 11);
            sf::FloatRect bounds = text.getLocalBounds();
            text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
            text.setPosition({ x + size / 2.0f, y + size / 2.0f });
            text.setFillColor(sf::Color::White);
            text.setOutlineColor(sf::Color::Black);
            text.setOutlineThickness(1.0f);
            target.draw(text);

            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << remaining;
            sf::Text timeText(*m_font, ss.str(), 10);
            bounds = timeText.getLocalBounds();
            timeText.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f });
            timeText.setPosition({ x + size / 2.0f, y - 8.0f });
            timeText.setFillColor(sf::Color::Yellow);
            timeText.setOutlineColor(sf::Color::Black);
            timeText.setOutlineThickness(1.0f);
            target.draw(timeText);

            ++col;
        };

        if (fx.igniteRemaining > 0.0f) drawIcon("IGN", sf::Color(255, 120, 0), fx.igniteRemaining);
        if (fx.bleedRemaining > 0.0f) drawIcon("BLD", sf::Color(180, 0, 0), fx.bleedRemaining);
        if (!fx.poisonStacks.empty()) {
            float maxRemaining = 0.0f;
            for (auto& stack : fx.poisonStacks) {
                if (stack.remaining > maxRemaining) maxRemaining = stack.remaining;
            }
            drawIcon("PSN", sf::Color(120, 0, 160), maxRemaining);
        }
        if (fx.chillRemaining > 0.0f) drawIcon("CHL", sf::Color(120, 220, 255), fx.chillRemaining);
        if (fx.freezeRemaining > 0.0f) drawIcon("FRZ", sf::Color(0, 120, 255), fx.freezeRemaining);
        if (fx.shockRemaining > 0.0f) drawIcon("SHK", sf::Color(255, 230, 0), fx.shockRemaining);
        if (fx.stunRemaining > 0.0f) drawIcon("STN", sf::Color(200, 200, 200), fx.stunRemaining);
    }

    // �X�e�[�^�X���̕`��
    void DrawStats(sf::RenderTarget& target, CharacterStatsComponent& stats) {
        std::string info =
            "Lv: " + std::to_string(stats.level) +
            "  XP: " + std::to_string(stats.currentXP) + "/" + std::to_string(stats.xpToNextLevel) + "\n" +
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