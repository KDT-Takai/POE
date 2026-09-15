#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <sstream>
#include <algorithm>
#include "../../Registry/Registry.h"
#include "../../Components/Item/Item.h"
#include "../../Components/Item/Inventory.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Item/EquipmentSystem.h"
#include "../Item/ItemFactory.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "ItemUIHelpers.h"

class InventorySystem {
private:
    std::shared_ptr<sf::Font> m_font;
    int m_selectedIndex = 0;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    InventorySystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() {
        isOpen = !isOpen;
        m_selectedIndex = 0;
    }

    void Close() { isOpen = false; }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, InventoryComponent, EquipmentComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];

        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        if (inventory.items.empty()) {
            m_selectedIndex = 0;
            return;
        }

        m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);

        auto& keyInput = InputManager::Instance().GetKeyInput();
        int count = static_cast<int>(inventory.items.size());

        if (keyInput.IsGetKey(sf::Keyboard::Key::Down)) {
            m_selectedIndex = (m_selectedIndex + 1) % count;
        }
        if (keyInput.IsGetKey(sf::Keyboard::Key::Up)) {
            m_selectedIndex = (m_selectedIndex - 1 + count) % count;
        }

        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        if (keyInput.IsGetKey(sf::Keyboard::Key::Enter)) {
            EquipSelected(inventory, equipment, stats);
        } else if (keyInput.IsGetKey(sf::Keyboard::Key::X)) {
            SellSelected(inventory, stats);
        } else if (keyInput.IsGetKey(sf::Keyboard::Key::Delete)) {
            DiscardSelected(inventory);
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, InventoryComponent, EquipmentComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];

        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);

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
            "Inventory (I to close) - Up/Down select, Enter equip, X sell, Del discard",
            15, sf::Color(255, 220, 120));

        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            std::to_string(inventory.items.size()) + " / " + std::to_string(InventoryComponent::kCapacity) + " slots used",
            13, sf::Color(180, 180, 180));

        float listY = panelY + 70.0f;
        float lineHeight = 22.0f;
        float maxListHeight = panelH - 90.0f;
        int maxVisible = static_cast<int>(maxListHeight / lineHeight);

        if (inventory.items.empty()) {
            DrawText(target, panelX + 20.0f, listY, "(empty)", 14, sf::Color(150, 150, 150));
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);

            int scrollOffset = 0;
            if (m_selectedIndex >= maxVisible) scrollOffset = m_selectedIndex - maxVisible + 1;

            for (int i = scrollOffset; i < static_cast<int>(inventory.items.size()) && i < scrollOffset + maxVisible; ++i) {
                const ItemComponent& item = inventory.items[i];
                float y = listY + static_cast<float>(i - scrollOffset) * lineHeight;

                bool selected = (i == m_selectedIndex);
                if (selected) {
                    sf::RectangleShape highlight({ panelW - 40.0f, lineHeight });
                    highlight.setPosition({ panelX + 20.0f, y });
                    highlight.setFillColor(sf::Color(60, 60, 90, 180));
                    target.draw(highlight);
                }

                std::string line = ItemUIHelpers::SlotName(item.slot) + ": " + item.baseName + " (" + ItemUIHelpers::RarityName(item.rarity) +
                    ", iLvl " + std::to_string(item.itemLevel) + ", " + std::to_string(item.affixes.size()) +
                    " mods, Score " + std::to_string(static_cast<int>(item.PowerScore())) + ")";

                DrawText(target, panelX + 26.0f, y + 2.0f, line, 14, ItemUIHelpers::RarityColor(item.rarity));
            }

            const ItemComponent& selectedItem = inventory.items[m_selectedIndex];
            size_t slotIdx = static_cast<size_t>(selectedItem.slot);
            std::string comparison;
            if (equipment.slots[slotIdx].has_value()) {
                const ItemComponent& equipped = *equipment.slots[slotIdx];
                comparison = "Equipped in that slot: " + equipped.baseName + " (Score " +
                    std::to_string(static_cast<int>(equipped.PowerScore())) + ")";
            } else {
                comparison = "That slot is currently empty.";
            }
            DrawText(target, panelX + 20.0f, panelY + panelH - 44.0f, comparison, 13, sf::Color(200, 200, 200));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + panelH - 20.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    void EquipSelected(InventoryComponent& inventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        if (inventory.items.empty()) return;
        ItemComponent picked = inventory.items[m_selectedIndex];
        size_t slotIdx = static_cast<size_t>(picked.slot);
        auto& currentSlot = equipment.slots[slotIdx];

        if (currentSlot.has_value()) {
            inventory.items[m_selectedIndex] = *currentSlot;
        } else {
            inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        }
        currentSlot = picked;
        EquipmentSystem::RecalculateStats(stats, equipment);

        lastActionMessage = "Equipped: " + picked.baseName;
        messageTimer = 2.5f;
        m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);
    }

    void SellSelected(InventoryComponent& inventory, CharacterStatsComponent& stats) {
        if (inventory.items.empty()) return;
        const ItemComponent& item = inventory.items[m_selectedIndex];
        int goldValue = ItemFactory::SellValue(item);
        stats.gold += goldValue;

        lastActionMessage = "Sold " + item.baseName + " for " + std::to_string(goldValue) + " gold";
        messageTimer = 2.5f;

        inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);
    }

    void DiscardSelected(InventoryComponent& inventory) {
        if (inventory.items.empty()) return;
        const ItemComponent& item = inventory.items[m_selectedIndex];

        lastActionMessage = "Discarded: " + item.baseName;
        messageTimer = 2.5f;

        inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);
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
