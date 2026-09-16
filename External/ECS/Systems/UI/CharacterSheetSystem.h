#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include "../../Registry/Registry.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Tags/Player/Player.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/KeyBindings/KeyBindings.h"

// PoE2本家のキャラクターシート(Cキー)の構成に合わせている: 概要(Lv/経験値)→属性
// (STR/DEX/INT)→主要ステータス(Life/Mana/Spirit/耐性)→詳細な防御(Energy Shield/
// Armour/Evasion)→その他(移動速度等)。本家同様、装備欄は無し(Iキーのインベントリで
// 確認可能)、攻撃力/クリティカル等の攻撃系ステータスも無し(本家はスキルごとに個別の
// 攻撃力パネルを持つ設計のため、キャラクターシート自体には攻撃ステータスを表示しない)。
class CharacterSheetSystem {
private:
    std::shared_ptr<sf::Font> m_font;

public:
    bool isOpen = false;

    CharacterSheetSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() { isOpen = !isOpen; }

    // Used by GameScene to resolve click ownership between the non-pausing menus:
    // only steal the click if the cursor is actually over this panel's rect.
    bool IsPointInPanel(sf::Vector2f point) const {
        if (!isOpen) return false;
        return sf::FloatRect({ kPanelX, kPanelY }, { kPanelW, kPanelH }).contains(point);
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity playerEntity = players[0];

        auto& stats = registry.GetComponent<CharacterStatsComponent>(playerEntity);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        float panelW = kPanelW;
        float panelH = kPanelH;
        float panelX = kPanelX;
        float panelY = kPanelY;

        sf::RectangleShape bg({ panelW, panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        std::string closeKey = KeyToString(KeyBindings::Instance().Get(GameAction::ToggleCharacterSheet));
        DrawText(target, panelX + 16.0f, panelY + 10.0f, "キャラクターシート (" + closeKey + " で閉じる)", 16, sf::Color(255, 220, 120));

        std::ostringstream statText;
        statText << std::fixed << std::setprecision(1);

        // 概要
        statText << "Lv " << stats.level << "  経験値 " << stats.currentXP << "/" << stats.xpToNextLevel << "\n";
        if (stats.passivePoints > 0) {
            statText << "パッシブポイント: " << stats.passivePoints << " (Pキーで割り振り)\n";
        }
        statText << "\n";

        // 属性
        statText << "STR " << stats.str << "  DEX " << stats.dex << "  INT " << stats.intelligence << "\n\n";

        // 主要ステータス(Life/Mana/Spirit/耐性)
        statText << "生命力 " << stats.currentHP << "/" << stats.maxHP << "\n";
        statText << "マナ " << stats.currentMP << "/" << stats.maxMP << "\n";
        statText << "スピリット " << stats.currentSpirit << "/" << stats.maxSpirit << "\n";
        statText << "耐性: 火 " << (stats.fireRes * 100.0f) << "% 冷気 " << (stats.iceRes * 100.0f)
            << "% 電気 " << (stats.lightningRes * 100.0f) << "% カオス " << (stats.chaosRes * 100.0f) << "%\n\n";

        // 詳細な防御(Energy Shield/Armour/Evasion)
        statText << "エナジーシールド " << stats.currentES << "/" << stats.maxES << "\n";
        statText << "アーマー " << stats.armour << "\n";
        statText << "回避力 " << stats.evasion << "\n\n";

        // その他
        statText << "移動速度 " << stats.moveSpeed << "\n";
        statText << "命中率 " << stats.accuracy << "\n";
        if (stats.leechPercent > 0.0f) {
            statText << "ライフリーチ: " << (stats.leechPercent * 100.0f) << "%\n";
        }

        std::string statStr = statText.str();
        sf::Text statTextObj(*m_font, sf::String::fromUtf8(statStr.begin(), statStr.end()), 14);
        statTextObj.setFillColor(sf::Color::White);
        statTextObj.setOutlineColor(sf::Color::Black);
        statTextObj.setOutlineThickness(1.0f);
        statTextObj.setPosition({ panelX + 16.0f, panelY + 40.0f });
        target.draw(statTextObj);

        target.setView(oldView);
    }

private:
    // Shared with SkillGemSystem's identical constants: the two panels are tab-switched
    // (mutually exclusive, last one toggled wins) so they occupy the same screen slot.
    static constexpr float kPanelW = 460.0f;
    static constexpr float kPanelH = 680.0f;
    static constexpr float kPanelX = 20.0f;
    static constexpr float kPanelY = 20.0f;

    void DrawText(sf::RenderTarget& target, float x, float y, const std::string& str, unsigned int size, sf::Color color) {
        // std::string -> sf::Text's implicit sf::String ctor is ANSI/locale, not UTF-8.
        sf::Text text(*m_font, sf::String::fromUtf8(str.begin(), str.end()), size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }

};
