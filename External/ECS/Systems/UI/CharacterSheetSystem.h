#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include "../../Registry/Registry.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Tags/Player/Player.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"
#include "ItemUIHelpers.h"

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

        auto players = registry.View<PlayerTag, CharacterStatsComponent, EquipmentComponent>();
        if (players.empty()) return;
        Entity playerEntity = players[0];

        auto& stats = registry.GetComponent<CharacterStatsComponent>(playerEntity);
        auto& equipment = registry.GetComponent<EquipmentComponent>(playerEntity);

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
        DrawText(target, panelX + 16.0f, panelY + 10.0f, "Character Sheet (" + closeKey + " to close)", 16, sf::Color(255, 220, 120));

        float contentWidth = panelW - 32.0f;

        std::ostringstream statText;
        statText << std::fixed << std::setprecision(1);
        statText << "Lv " << stats.level << "  XP " << stats.currentXP << "/" << stats.xpToNextLevel << "  Gold " << stats.gold << "\n";
        if (stats.passivePoints > 0) {
            statText << "Passive Points: " << stats.passivePoints << " (P to spend)\n";
        }
        statText << "HP " << stats.currentHP << "/" << stats.maxHP << "  MP " << stats.currentMP << "/" << stats.maxMP << "\n";
        statText << "ES " << stats.currentES << "/" << stats.maxES << "\n";
        statText << "Str " << stats.str << "  Dex " << stats.dex << "  Int " << stats.intelligence << "\n";
        statText << "Atk " << stats.atk << "  Crit " << (stats.critRate * 100.0f) << "%  CritMulti " << (stats.critDamage * 100.0f) << "%\n";
        statText << "MoveSpd " << stats.moveSpeed << "  Evasion " << stats.evasion << "  Armour " << stats.armour << "\n";
        statText << "Accuracy " << stats.accuracy << "\n";
        statText << "Res: Fire " << (stats.fireRes * 100.0f) << "% Cold " << (stats.iceRes * 100.0f)
            << "% Light " << (stats.lightningRes * 100.0f) << "% Chaos " << (stats.chaosRes * 100.0f) << "%\n";
        if (stats.leechPercent > 0.0f) {
            statText << "Leech: " << (stats.leechPercent * 100.0f) << "%\n";
        }

        std::string statStr = statText.str();
        sf::Text statTextObj(*m_font, sf::String::fromUtf8(statStr.begin(), statStr.end()), 13);
        statTextObj.setFillColor(sf::Color::White);
        statTextObj.setOutlineColor(sf::Color::Black);
        statTextObj.setOutlineThickness(1.0f);
        statTextObj.setPosition({ panelX + 16.0f, panelY + 34.0f });
        target.draw(statTextObj);

        // Position the equipment section below the stat block using its *measured* height
        // rather than an assumed line-height, since actual font line spacing doesn't match
        // a hand-picked pixel guess and caused the two sections to overlap.
        float equipY = statTextObj.getGlobalBounds().position.y + statTextObj.getGlobalBounds().size.y + 14.0f;

        DrawText(target, panelX + 16.0f, equipY, "Equipment", 15, sf::Color(255, 220, 120));
        equipY += 20.0f;

        for (size_t i = 0; i < equipment.slots.size(); ++i) {
            std::string slotLabel = ItemUIHelpers::SlotName(static_cast<EquipSlot>(i));
            std::string line;
            sf::Color color = sf::Color(150, 150, 150);

            if (equipment.slots[i].has_value()) {
                const ItemComponent& item = *equipment.slots[i];
                line = slotLabel + ": " + item.baseName + " (" + ItemUIHelpers::RarityName(item.rarity) + ", " +
                    std::to_string(item.affixes.size()) + " mods)";
                color = ItemUIHelpers::RarityColor(item.rarity);
            } else {
                line = slotLabel + ": (empty)";
            }

            line = Truncate(line, contentWidth, 13);
            DrawText(target, panelX + 16.0f, equipY + static_cast<float>(i) * 18.0f, line, 13, color);
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

    // Clips str to fit within maxWidth pixels at the given font size, appending "...".
    // Item names are procedurally generated and can be arbitrarily long, so line width
    // can't be bounded just by tuning the layout constants above.
    std::string Truncate(const std::string& str, float maxWidth, unsigned int size) const {
        sf::Text probe(*m_font, sf::String::fromUtf8(str.begin(), str.end()), size);
        if (probe.getLocalBounds().size.x <= maxWidth) return str;

        std::string result = str;
        while (!result.empty()) {
            result.pop_back();
            std::string candidate = result + "...";
            sf::Text probe2(*m_font, sf::String::fromUtf8(candidate.begin(), candidate.end()), size);
            if (probe2.getLocalBounds().size.x <= maxWidth) return candidate;
        }
        return "...";
    }

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
