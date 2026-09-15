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
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"

class VendorSystem {
private:
    // Selling only happens through this vendor window (talk to the NPC), never from the
    // field inventory screen -- matches real PoE2, where a vendor visit is required to
    // liquidate items rather than being able to sell from anywhere.
    enum class VendorTab { Buy, Sell };

    std::shared_ptr<sf::Font> m_font;
    std::vector<ItemComponent> m_stock;
    bool m_stockGenerated = false;
    int m_selectedIndex = 0;
    VendorTab m_activeTab = VendorTab::Buy;

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
        m_activeTab = VendorTab::Buy;
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
        auto& inventory = registry.GetComponent<InventoryComponent>(player);

        auto& keyInput = InputManager::Instance().GetKeyInput();

        if (keyInput.IsGetKey(KeyBindings::Instance().Get(GameAction::VendorReroll))) {
            TryReroll(stats);
        }

        if (keyInput.IsGetKey(sf::Keyboard::Key::Left) || keyInput.IsGetKey(sf::Keyboard::Key::Right)) {
            m_activeTab = (m_activeTab == VendorTab::Buy) ? VendorTab::Sell : VendorTab::Buy;
            m_selectedIndex = 0;
        }

        int count = (m_activeTab == VendorTab::Buy)
            ? static_cast<int>(m_stock.size())
            : static_cast<int>(inventory.items.size());
        if (count == 0) return;
        m_selectedIndex = std::clamp(m_selectedIndex, 0, count - 1);

        if (keyInput.IsGetKey(sf::Keyboard::Key::Down)) m_selectedIndex = (m_selectedIndex + 1) % count;
        if (keyInput.IsGetKey(sf::Keyboard::Key::Up)) m_selectedIndex = (m_selectedIndex - 1 + count) % count;

        if (keyInput.IsGetKey(sf::Keyboard::Key::Enter)) {
            if (m_activeTab == VendorTab::Buy) TryBuy(inventory, stats);
            else TrySell(inventory, stats);
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, InventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        const auto& playerStats = registry.GetComponent<CharacterStatsComponent>(player);
        const auto& inventory = registry.GetComponent<InventoryComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::Vector2u winSize = target.getSize();
        float panelW = 620.0f;
        // Tall enough to list a full 24-slot bag on the Sell tab without scrolling.
        float panelH = 660.0f;
        float panelX = (winSize.x - panelW) / 2.0f;
        float panelY = (winSize.y - panelH) / 2.0f;

        sf::RectangleShape bg({ panelW, panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        std::string closeKeyName = KeyToString(KeyBindings::Instance().Get(GameAction::VendorToggle));
        std::string rerollKeyName = KeyToString(KeyBindings::Instance().Get(GameAction::VendorReroll));
        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "Vendor (" + closeKeyName + " to close) - Left/Right switch tab, Up/Down select, Enter buy/sell, " + rerollKeyName + " reroll stock",
            14, sf::Color(255, 220, 120));

        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            "Gold: " + std::to_string(playerStats.gold) + "   Reroll cost: " + std::to_string(RerollPrice(playerStats.level)) + "g",
            13, sf::Color(255, 215, 90));

        // Tabs
        bool buyActive = m_activeTab == VendorTab::Buy;
        DrawText(target, panelX + 20.0f, panelY + 58.0f, buyActive ? "[ Buy ]" : "  Buy  ", 14, buyActive ? sf::Color::White : sf::Color(140, 140, 140));
        DrawText(target, panelX + 100.0f, panelY + 58.0f, buyActive ? "  Sell  " : "[ Sell ]", 14, buyActive ? sf::Color(140, 140, 140) : sf::Color::White);

        float listY = panelY + 82.0f;
        float lineHeight = 20.0f;

        if (buyActive) {
            DrawItemList(target, m_stock, panelX, panelW, listY, lineHeight, true, "(sold out - press " + rerollKeyName + " to reroll)");
        } else {
            DrawItemList(target, inventory.items, panelX, panelW, listY, lineHeight, false, "(bag is empty)");
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

    void TrySell(InventoryComponent& inventory, CharacterStatsComponent& stats) {
        if (inventory.items.empty()) return;
        if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(inventory.items.size())) return;

        const ItemComponent& item = inventory.items[m_selectedIndex];
        int goldValue = ItemFactory::SellValue(item);
        stats.gold += goldValue;
        lastActionMessage = "Sold: " + item.baseName + " (+" + std::to_string(goldValue) + "g)";
        messageTimer = 2.0f;

        inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        if (!inventory.items.empty()) {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);
        } else {
            m_selectedIndex = 0;
        }
    }

    // Shared renderer for both the vendor's buy stock and the player's sellable
    // inventory -- same layout, only the price source (buy vs sell value) differs.
    void DrawItemList(sf::RenderTarget& target, const std::vector<ItemComponent>& items, float panelX, float panelW,
        float listY, float lineHeight, bool isBuyList, const std::string& emptyMessage) {
        if (items.empty()) {
            DrawText(target, panelX + 20.0f, listY, emptyMessage, 14, sf::Color(150, 150, 150));
            return;
        }

        for (size_t i = 0; i < items.size(); ++i) {
            const ItemComponent& item = items[i];
            float y = listY + static_cast<float>(i) * lineHeight;

            bool selected = (static_cast<int>(i) == m_selectedIndex);
            if (selected) {
                sf::RectangleShape highlight({ panelW - 40.0f, lineHeight });
                highlight.setPosition({ panelX + 20.0f, y });
                highlight.setFillColor(sf::Color(60, 60, 90, 180));
                target.draw(highlight);
            }

            int price = isBuyList ? BuyPrice(item) : ItemFactory::SellValue(item);
            std::string line = ItemUIHelpers::SlotName(item.slot) + ": " + item.baseName + " (" +
                ItemUIHelpers::RarityName(item.rarity) + ", iLvl " + std::to_string(item.itemLevel) + ", " +
                std::to_string(item.affixes.size()) + " mods) - " + std::to_string(price) + "g";

            DrawText(target, panelX + 26.0f, y + 2.0f, line, 13, ItemUIHelpers::RarityColor(item.rarity));
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
        // std::string -> sf::Text's implicit sf::String ctor is ANSI/locale, not UTF-8.
        sf::Text text(*m_font, sf::String::fromUtf8(str.begin(), str.end()), size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }
};
