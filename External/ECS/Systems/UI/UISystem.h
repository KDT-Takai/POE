#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
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

    // スキルスロット列のレイアウト計算をGameScene側のクリック判定と共有するための公開
    // ヘルパー(帰還用ポータル生成ボタン、"スキルの横に" という指示でスキル列の右隣に
    // 置く)。Render()内の座標計算と同じ式を使うこと。
    static sf::FloatRect PortalButtonRect(sf::Vector2u winSize) {
        float slotSize = 50.0f;
        float gap = 8.0f;
        float rollGap = 20.0f;
        int skillCount = 5;
        float totalBarWidth = slotSize + rollGap + (slotSize * skillCount) + (gap * (skillCount - 1));
        float startX = (static_cast<float>(winSize.x) - totalBarWidth) / 2.0f;
        float startY = static_cast<float>(winSize.y) - 70.0f;
        float skillStartX = startX + slotSize + rollGap;
        float btnX = skillStartX + static_cast<float>(skillCount) * (slotSize + gap) + rollGap;
        return sf::FloatRect({ btnX, startY }, { slotSize, slotSize });
    }

    // showPortalButton: マップ内(Combatゾーン)でのみtrue、帰還用ポータル生成ボタンを
    // スキルスロットの右隣に描く("マップ上ではスキルの横に帰還用ポータルを出現させる
    // ものを用意"という指示対応)。portalChannelRemaining>0の間は詠唱中の進捗オーバーレイ
    // を重ねる(既存のスキルクールダウン表示と同じDrawCooldownOverlayを流用)。
    void Render(Registry& registry, sf::RenderTarget& target, bool showPortalButton = false,
        float portalChannelRemaining = 0.0f, float portalChannelMax = 1.5f) {
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

        // Energy Shield, PoE2-style: a glowing blue ring wrapped around the life orb's
        // rim (not a separate orb) that sweeps clockwise from the top and shrinks as ES
        // depletes -- ES absorbs non-Chaos damage before HP (see CombatMath::ApplyDamage),
        // so visually "shielding" the life orb reads the same way it does in the real game.
        // Only drawn once the character actually has any (maxES > 0).
        if (stats.maxES > 0.0f) {
            float esRatio = stats.maxES > 0.0f ? std::clamp(stats.currentES / stats.maxES, 0.0f, 1.0f) : 0.0f;
            DrawShieldRing(target, 80.0f, height - 80.0f, 45.0f, 7.0f, sf::Color(90, 190, 255), esRatio);

            std::string esStr = "ES " + std::to_string(static_cast<int>(stats.currentES)) + "/" + std::to_string(static_cast<int>(stats.maxES));
            sf::Text esText(*m_font, esStr, 13);
            sf::FloatRect esBounds = esText.getLocalBounds();
            esText.setOrigin({ esBounds.position.x + esBounds.size.x / 2.0f, esBounds.position.y + esBounds.size.y / 2.0f });
            esText.setPosition({ 80.0f, height - 80.0f - 45.0f - 14.0f });
            esText.setFillColor(sf::Color(150, 220, 255));
            esText.setOutlineColor(sf::Color::Black);
            esText.setOutlineThickness(1.0f);
            target.draw(esText);
        }

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

        if (showPortalButton) {
            sf::FloatRect btnRect = PortalButtonRect(winSize);
            DrawSlotFrame(target, btnRect.position.x, btnRect.position.y, btnRect.size.x, "帰還");
            DrawIcon(target, btnRect.position.x, btnRect.position.y, btnRect.size.x, sf::Color(255, 210, 60));
            if (portalChannelRemaining > 0.0f) {
                DrawCooldownOverlay(target, btnRect.position.x, btnRect.position.y, btnRect.size.x, portalChannelRemaining, portalChannelMax);
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

        sf::Text keyText(*m_font, sf::String::fromUtf8(keyName.begin(), keyName.end()), 12);

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

    // PoE2-style Energy Shield ring: a glowing arc hugging the outside of the life orb's
    // rim, sweeping clockwise from the top (12 o'clock) and shrinking as `ratio` (current/
    // max ES) drops, instead of a plain filled shape -- built from a triangle-strip ring
    // segment since SFML has no built-in circular progress primitive.
    void DrawShieldRing(sf::RenderTarget& target, float cx, float cy, float orbRadius, float thickness, sf::Color color, float ratio) {
        if (ratio <= 0.0f) return;

        float innerR = orbRadius + 2.0f;
        float outerR = innerR + thickness;
        constexpr int kMaxSegments = 64;
        int segments = (std::max)(1, static_cast<int>(kMaxSegments * ratio));

        sf::VertexArray strip(sf::PrimitiveType::TriangleStrip);
        sf::Color glow = color;
        glow.a = 220;
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(kMaxSegments); // 0..ratio
            float angleDeg = -90.0f + t * 360.0f; // start at 12 o'clock, sweep clockwise
            float angleRad = angleDeg * 3.14159265f / 180.0f;
            sf::Vector2f dir(std::cos(angleRad), std::sin(angleRad));
            strip.append(sf::Vertex(sf::Vector2f(cx, cy) + dir * innerR, glow));
            strip.append(sf::Vertex(sf::Vector2f(cx, cy) + dir * outerR, glow));
        }
        target.draw(strip);

        // A brighter thin highlight line along the outer edge, so the ring reads as a
        // glowing shell rather than a flat band.
        sf::VertexArray edge(sf::PrimitiveType::LineStrip);
        sf::Color highlight(220, 240, 255, 240);
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(kMaxSegments);
            float angleDeg = -90.0f + t * 360.0f;
            float angleRad = angleDeg * 3.14159265f / 180.0f;
            sf::Vector2f dir(std::cos(angleRad), std::sin(angleRad));
            edge.append(sf::Vertex(sf::Vector2f(cx, cy) + dir * outerR, highlight));
        }
        target.draw(edge);
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