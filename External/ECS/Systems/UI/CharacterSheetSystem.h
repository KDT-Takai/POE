#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
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

        // 本家PoE2の実際のキャラクターパネルは区分ごとに見出し+区切り線があり、耐性は
        // 各元素ごとに色分けされたバーで上限(75%)に対する充填率を表示する。プレーンな
        // テキストの羅列ではなくこの構造・配色を再現する。
        const float indent = panelX + 16.0f;
        const float contentW = kPanelW - 32.0f;
        float cursorY = panelY + 44.0f;

        auto section = [&](const std::string& title) {
            DrawText(target, indent, cursorY, title, 14, sf::Color(255, 210, 120));
            cursorY += 20.0f;
            sf::RectangleShape divider({ contentW, 1.0f });
            divider.setPosition({ indent, cursorY });
            divider.setFillColor(sf::Color(90, 90, 100));
            target.draw(divider);
            cursorY += 8.0f;
        };
        auto line = [&](const std::string& text, sf::Color color) {
            DrawText(target, indent, cursorY, text, 14, color);
            cursorY += 20.0f;
        };
        auto gap = [&](float h) { cursorY += h; };

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);
        auto fmt = [&](auto&& fn) -> std::string { ss.str(""); ss.clear(); fn(); return ss.str(); };

        const sf::Color kWhite = sf::Color::White;
        const sf::Color kStrColor(220, 90, 90);
        const sf::Color kDexColor(120, 200, 90);
        const sf::Color kIntColor(100, 160, 230);
        const sf::Color kFireColor(230, 100, 60);
        const sf::Color kColdColor(100, 190, 230);
        const sf::Color kLightningColor(230, 210, 70);
        const sf::Color kChaosColor(200, 90, 190);
        constexpr float kResCap = 75.0f;

        // 概要
        section("概要");
        line(fmt([&] { ss << "Lv " << stats.level << "  経験値 " << stats.currentXP << "/" << stats.xpToNextLevel; }), kWhite);
        if (stats.passivePoints > 0) {
            line(fmt([&] { ss << "パッシブポイント: " << stats.passivePoints << " (Pキーで割り振り)"; }), sf::Color(160, 220, 255));
        }
        gap(8.0f);

        // 属性 -- 本家に合わせ横並び、パッシブツリーと同じSTR=赤/DEX=緑/INT=青の配色。
        section("属性");
        DrawText(target, indent, cursorY, fmt([&] { ss << "STR " << stats.str; }), 14, kStrColor);
        DrawText(target, indent + 140.0f, cursorY, fmt([&] { ss << "DEX " << stats.dex; }), 14, kDexColor);
        DrawText(target, indent + 280.0f, cursorY, fmt([&] { ss << "INT " << stats.intelligence; }), 14, kIntColor);
        cursorY += 20.0f;
        gap(8.0f);

        // 主要ステータス(Life/Mana/Energy Shield/Spirit) -- 最大値と秒間回復量を併記。
        section("主要ステータス");
        line(fmt([&] { ss << "生命力 " << stats.currentHP << "/" << stats.maxHP << "  (自動回復 " << stats.healthRegen << "/秒)"; }), kWhite);
        line(fmt([&] { ss << "マナ " << stats.currentMP << "/" << stats.maxMP << "  (自動回復 " << stats.manaRegen << "/秒)"; }), kWhite);
        line(fmt([&] { ss << "エナジーシールド " << stats.currentES << "/" << stats.maxES << "  (回復 " << (stats.maxES * 0.33f) << "/秒, 被弾から3秒後)"; }), kWhite);
        line(fmt([&] { ss << "スピリット " << stats.currentSpirit << "/" << stats.maxSpirit; }), kWhite);
        gap(4.0f);

        // 耐性 -- 元素ごとに色分けしたバーで75%上限に対する充填率を表示(本家準拠)。
        auto resistLine = [&](const std::string& label, float value, sf::Color color) {
            line(fmt([&] { ss << label << " " << (value * 100.0f) << "% / " << static_cast<int>(kResCap) << "%"; }), color);
            DrawBar(target, indent, cursorY - 4.0f, contentW, 5.0f, (value * 100.0f) / kResCap, color);
            gap(8.0f);
        };
        resistLine("火耐性", stats.fireRes, kFireColor);
        resistLine("冷気耐性", stats.iceRes, kColdColor);
        resistLine("電気耐性", stats.lightningRes, kLightningColor);
        resistLine("カオス耐性", stats.chaosRes, kChaosColor);

        // 詳細な防御(Armour/Evasion/Block)
        section("詳細な防御");
        line(fmt([&] { ss << "アーマー " << stats.armour; }), kWhite);
        line(fmt([&] { ss << "回避力 " << stats.evasion; }), kWhite);
        line(fmt([&] { ss << "ブロック率 " << (stats.blockChance * 100.0f) << "%"; }), kWhite);
        gap(8.0f);

        // その他
        section("その他");
        line(fmt([&] { ss << "移動速度 " << stats.moveSpeed; }), kWhite);
        line(fmt([&] { ss << "命中率 " << stats.accuracy; }), kWhite);
        if (stats.leechPercent > 0.0f) {
            line(fmt([&] { ss << "ライフリーチ: " << (stats.leechPercent * 100.0f) << "%"; }), kWhite);
        }

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

    void DrawBar(sf::RenderTarget& target, float x, float y, float width, float height, float fraction, sf::Color fillColor) {
        fraction = std::clamp(fraction, 0.0f, 1.0f);
        sf::RectangleShape back({ width, height });
        back.setPosition({ x, y });
        back.setFillColor(sf::Color(40, 40, 45));
        back.setOutlineColor(sf::Color(90, 90, 100));
        back.setOutlineThickness(1.0f);
        target.draw(back);

        if (fraction > 0.0f) {
            sf::RectangleShape fill({ width * fraction, height });
            fill.setPosition({ x, y });
            fill.setFillColor(fillColor);
            target.draw(fill);
        }
    }

};
