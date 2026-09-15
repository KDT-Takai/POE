#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include "../../Registry/Registry.h"
#include "../../Components/Item/Item.h"
#include "../../Components/Item/Inventory.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Tags/Player/Player.h"
#include "ItemFactory.h"
#include "../UI/ItemUIHelpers.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"

class VendorSystem {
private:
    std::shared_ptr<sf::Font> m_font;
    std::vector<ItemComponent> m_stock;
    bool m_stockGenerated = false;
    int m_selectedIndex = 0;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    VendorSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Open(int playerLevel) {
        if (!m_stockGenerated) GenerateStock(playerLevel);
        isOpen = true;
        m_selectedIndex = 0;
    }

    void Close() { isOpen = false; }

    void Toggle(int playerLevel) {
        if (isOpen) Close();
        else Open(playerLevel);
    }

    static int BuyPrice(const ItemComponent& item) {
        return ItemFactory::SellValue(item) * 3;
    }

    static int RerollPrice(int playerLevel) {
        return 20 + playerLevel * 4;
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, InventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        auto& keyInput = InputManager::Instance().GetKeyInput();

        if (keyInput.IsGetKey(sf::Keyboard::Key::T)) {
            TryReroll(stats);
        }

        if (m_stock.empty()) return;

        int count = static_cast<int>(m_stock.size());
        m_selectedIndex = std::clamp(m_selectedIndex, 0, count - 1);

        if (keyInput.IsGetKey(sf::Keyboard::Key::Down)) m_selectedIndex = (m_selectedIndex + 1) % count;
        if (keyInput.IsGetKey(sf::Keyboard::Key::Up)) m_selectedIndex = (m_selectedIndex - 1 + count) % count;

        if (keyInput.IsGetKey(sf::Keyboard::Key::Enter)) {
            TryBuy(registry.GetComponent<InventoryComponent>(player), stats);
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

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

        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "Vendor (B to close) - Up/Down select, Enter buy, T reroll stock", 15, sf::Color(255, 220, 120));

        int gold = 0;
        int playerLevel = 1;
        auto players = registry.View<PlayerTag, CharacterStatsComponent>();
        if (!players.empty()) {
            const auto& playerStats = registry.GetComponent<CharacterStatsComponent>(players[0]);
            gold = playerStats.gold;
            playerLevel = playerStats.level;
        }
        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            "Gold: " + std::to_string(gold) + "   Reroll cost: " + std::to_string(RerollPrice(playerLevel)) + "g",
            13, sf::Color(255, 215, 90));

        float listY = panelY + 70.0f;
        float lineHeight = 24.0f;

        if (m_stock.empty()) {
            DrawText(target, panelX + 20.0f, listY, "(sold out - press T to reroll)", 14, sf::Color(150, 150, 150));
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(m_stock.size()) - 1);

            for (size_t i = 0; i < m_stock.size(); ++i) {
                const ItemComponent& item = m_stock[i];
                float y = listY + static_cast<float>(i) * lineHeight;

                bool selected = (static_cast<int>(i) == m_selectedIndex);
                if (selected) {
                    sf::RectangleShape highlight({ panelW - 40.0f, lineHeight });
                    highlight.setPosition({ panelX + 20.0f, y });
                    highlight.setFillColor(sf::Color(60, 60, 90, 180));
                    target.draw(highlight);
                }

                std::string line = ItemUIHelpers::SlotName(item.slot) + ": " + item.baseName + " (" +
                    ItemUIHelpers::RarityName(item.rarity) + ", iLvl " + std::to_string(item.itemLevel) + ", " +
                    std::to_string(item.affixes.size()) + " mods) - " + std::to_string(BuyPrice(item)) + "g";

                DrawText(target, panelX + 26.0f, y + 2.0f, line, 14, ItemUIHelpers::RarityColor(item.rarity));
            }
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + panelH - 20.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    void GenerateStock(int playerLevel) {
        m_stock.clear();
        static std::random_device rd;
        static std::mt19937 rng(rd());
        int itemLevel = std::clamp(playerLevel, 1, 100);

        for (int i = 0; i < 6; ++i) {
            EquipSlot slot = ItemFactory::RollSlot(rng);
            ItemRarity rarity = ItemFactory::RollRarity(rng);
            m_stock.push_back(ItemFactory::GenerateItem(slot, rarity, itemLevel, rng));
        }
        m_stockGenerated = true;
    }

    void TryBuy(InventoryComponent& inventory, CharacterStatsComponent& stats) {
        if (m_stock.empty()) return;
        const ItemComponent& item = m_stock[m_selectedIndex];
        int price = BuyPrice(item);

        if (stats.gold < price) {
            lastActionMessage = "Not enough gold";
            messageTimer = 2.0f;
            return;
        }
        if (inventory.items.size() >= InventoryComponent::kCapacity) {
            lastActionMessage = "Inventory full";
            messageTimer = 2.0f;
            return;
        }

        stats.gold -= price;
        inventory.items.push_back(item);
        lastActionMessage = "Bought: " + item.baseName;
        messageTimer = 2.0f;

        m_stock.erase(m_stock.begin() + m_selectedIndex);
        if (!m_stock.empty()) {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(m_stock.size()) - 1);
        } else {
            m_selectedIndex = 0;
        }
    }

    void TryReroll(CharacterStatsComponent& stats) {
        int price = RerollPrice(stats.level);
        if (stats.gold < price) {
            lastActionMessage = "Not enough gold to reroll";
            messageTimer = 2.0f;
            return;
        }

        stats.gold -= price;
        GenerateStock(stats.level);
        m_selectedIndex = 0;
        lastActionMessage = "Stock rerolled";
        messageTimer = 2.0f;
    }

    void DrawText(sf::RenderTarget& target, float x, float y, const std::string& str, unsigned int size, sf::Color color) {
        sf::Text text(*m_font, str, size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }
};
