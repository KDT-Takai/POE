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

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, CharacterStatsComponent, EquipmentComponent>();
        if (players.empty()) return;
        Entity playerEntity = players[0];

        auto& stats = registry.GetComponent<CharacterStatsComponent>(playerEntity);
        auto& equipment = registry.GetComponent<EquipmentComponent>(playerEntity);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::Vector2u winSize = target.getSize();
        float panelW = 620.0f;
        float panelH = 480.0f;
        float panelX = (winSize.x - panelW) / 2.0f;
        float panelY = (winSize.y - panelH) / 2.0f;

        sf::RectangleShape bg({ panelW, panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        DrawText(target, panelX + 20.0f, panelY + 15.0f, "Character Sheet (C to close)", 22, sf::Color(255, 220, 120));

        std::ostringstream statText;
        statText << std::fixed << std::setprecision(1);
        statText << "Lv " << stats.level << "   XP " << stats.currentXP << "/" << stats.xpToNextLevel
            << "   Gold " << stats.gold << "\n";
        if (stats.passivePoints > 0) {
            statText << "Passive Points: " << stats.passivePoints << " (P to spend)\n";
        }
        statText << "\n";
        statText << "Life: " << stats.currentHP << " / " << stats.maxHP << "\n";
        statText << "Mana: " << stats.currentMP << " / " << stats.maxMP << "\n";
        statText << "Energy Shield: " << stats.currentES << " / " << stats.maxES << "\n\n";
        statText << "Str " << stats.str << "   Dex " << stats.dex << "   Int " << stats.intelligence << "\n\n";
        statText << "Attack: " << stats.atk << "\n";
        statText << "Crit Chance: " << (stats.critRate * 100.0f) << "%\n";
        statText << "Crit Multiplier: " << (stats.critDamage * 100.0f) << "%\n";
        statText << "Move Speed: " << stats.moveSpeed << "\n\n";
        statText << "Evasion: " << stats.evasion << "\n";
        statText << "Armour: " << stats.armour << "\n";
        statText << "Accuracy: " << stats.accuracy << "\n\n";
        statText << "Fire Res: " << (stats.fireRes * 100.0f) << "%\n";
        statText << "Cold Res: " << (stats.iceRes * 100.0f) << "%\n";
        statText << "Lightning Res: " << (stats.lightningRes * 100.0f) << "%\n";
        statText << "Chaos Res: " << (stats.chaosRes * 100.0f) << "%\n";
        if (stats.leechPercent > 0.0f) {
            statText << "Leech: " << (stats.leechPercent * 100.0f) << "%\n";
        }

        DrawText(target, panelX + 20.0f, panelY + 50.0f, statText.str(), 15, sf::Color::White);

        float equipX = panelX + 340.0f;
        float equipY = panelY + 50.0f;
        DrawText(target, equipX, panelY + 15.0f, "Equipment", 18, sf::Color(255, 220, 120));

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

            DrawText(target, equipX, equipY + static_cast<float>(i) * 24.0f, line, 14, color);
        }

        target.setView(oldView);
    }

private:
    void DrawText(sf::RenderTarget& target, float x, float y, const std::string& str, unsigned int size, sf::Color color) {
        sf::Text text(*m_font, str, size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }

};
