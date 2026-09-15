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
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"
#include "ItemUIHelpers.h"

class InventorySystem {
private:
    // The list keeps its original footprint; the footer below it is new space
    // for the selected-item comparison + Equip/Sell/Discard buttons.
    static constexpr float kPanelW = 620.0f;
    static constexpr float kListAreaH = 480.0f;
    static constexpr float kFooterH = 90.0f;
    static constexpr float kPanelH = kListAreaH + kFooterH;
    static constexpr float kLineHeight = 22.0f;
    static constexpr float kListY = 70.0f; // offset from panel top
    static constexpr float kButtonW = 130.0f;
    static constexpr float kButtonH = 36.0f;
    static constexpr float kButtonGap = 20.0f;

    struct PanelLayout {
        float panelX = 0.0f;
        float panelY = 0.0f;
        sf::FloatRect equipBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect sellBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect discardBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
    };

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
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        if (inventory.items.empty()) {
            m_selectedIndex = 0;
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);

            auto& keyInput = InputManager::Instance().GetKeyInput();
            int count = static_cast<int>(inventory.items.size());

            if (keyInput.IsGetKey(sf::Keyboard::Key::Down)) {
                m_selectedIndex = (m_selectedIndex + 1) % count;
            }
            if (keyInput.IsGetKey(sf::Keyboard::Key::Up)) {
                m_selectedIndex = (m_selectedIndex - 1 + count) % count;
            }

            if (keyInput.IsGetKey(sf::Keyboard::Key::Enter)) {
                EquipSelected(inventory, equipment, stats);
            } else if (keyInput.IsGetKey(sf::Keyboard::Key::X)) {
                SellSelected(inventory, stats);
            } else if (keyInput.IsGetKey(sf::Keyboard::Key::Delete)) {
                DiscardSelected(inventory);
            }
        }

        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;
        PanelLayout layout = ComputeLayout(window->getSize());

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (mouseInput.IsGetMouse(sf::Mouse::Button::Left)) {
            sf::Vector2f mouse = mouseInput.GetMousePointF();

            int clickedRow = HitTestRow(layout, inventory, mouse);
            if (clickedRow >= 0) {
                m_selectedIndex = clickedRow;
            } else if (layout.equipBtn.contains(mouse)) {
                EquipSelected(inventory, equipment, stats);
            } else if (layout.sellBtn.contains(mouse)) {
                SellSelected(inventory, stats);
            } else if (layout.discardBtn.contains(mouse)) {
                DiscardSelected(inventory);
            }
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

        PanelLayout layout = ComputeLayout(target.getSize());
        float panelX = layout.panelX;
        float panelY = layout.panelY;

        sf::RectangleShape bg({ kPanelW, kPanelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        std::string closeKey = KeyToString(KeyBindings::Instance().Get(GameAction::ToggleInventory));
        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "Inventory (" + closeKey + " to close) - Click an item, then Equip/Sell/Discard",
            15, sf::Color(255, 220, 120));

        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            std::to_string(inventory.items.size()) + " / " + std::to_string(InventoryComponent::kCapacity) + " slots used",
            13, sf::Color(180, 180, 180));

        float listY = panelY + kListY;
        float maxListHeight = kListAreaH - kListY - 20.0f;
        int maxVisible = static_cast<int>(maxListHeight / kLineHeight);

        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();

        if (inventory.items.empty()) {
            DrawText(target, panelX + 20.0f, listY, "(empty)", 14, sf::Color(150, 150, 150));
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);

            int scrollOffset = 0;
            if (m_selectedIndex >= maxVisible) scrollOffset = m_selectedIndex - maxVisible + 1;

            for (int i = scrollOffset; i < static_cast<int>(inventory.items.size()) && i < scrollOffset + maxVisible; ++i) {
                const ItemComponent& item = inventory.items[i];
                float y = listY + static_cast<float>(i - scrollOffset) * kLineHeight;

                bool selected = (i == m_selectedIndex);
                sf::FloatRect rowRect({ panelX + 20.0f, y }, { kPanelW - 40.0f, kLineHeight });
                bool hovered = rowRect.contains(mouse);
                if (selected || hovered) {
                    sf::RectangleShape highlight({ kPanelW - 40.0f, kLineHeight });
                    highlight.setPosition({ panelX + 20.0f, y });
                    highlight.setFillColor(selected ? sf::Color(60, 60, 90, 180) : sf::Color(50, 50, 60, 120));
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
            DrawText(target, panelX + 20.0f, panelY + kListAreaH + 4.0f, comparison, 13, sf::Color(200, 200, 200));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + kListAreaH + 26.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        bool hasSelection = !inventory.items.empty();
        DrawButton(target, layout.equipBtn, "Equip", layout.equipBtn.contains(mouse), hasSelection ? sf::Color(60, 110, 150) : sf::Color(60, 60, 65));
        DrawButton(target, layout.sellBtn, "Sell", layout.sellBtn.contains(mouse), hasSelection ? sf::Color(150, 130, 50) : sf::Color(60, 60, 65));
        DrawButton(target, layout.discardBtn, "Discard", layout.discardBtn.contains(mouse), hasSelection ? sf::Color(130, 60, 60) : sf::Color(60, 60, 65));

        target.setView(oldView);
    }

private:
    PanelLayout ComputeLayout(sf::Vector2u winSize) const {
        PanelLayout layout;
        layout.panelX = (static_cast<float>(winSize.x) - kPanelW) / 2.0f;
        layout.panelY = (static_cast<float>(winSize.y) - kPanelH) / 2.0f;

        float totalBtnW = kButtonW * 3.0f + kButtonGap * 2.0f;
        float btnX = layout.panelX + (kPanelW - totalBtnW) / 2.0f;
        float btnY = layout.panelY + kListAreaH + 46.0f;
        layout.equipBtn = sf::FloatRect({ btnX, btnY }, { kButtonW, kButtonH });
        layout.sellBtn = sf::FloatRect({ btnX + kButtonW + kButtonGap, btnY }, { kButtonW, kButtonH });
        layout.discardBtn = sf::FloatRect({ btnX + (kButtonW + kButtonGap) * 2.0f, btnY }, { kButtonW, kButtonH });
        return layout;
    }

    int HitTestRow(const PanelLayout& layout, const InventoryComponent& inventory, sf::Vector2f mouse) const {
        if (inventory.items.empty()) return -1;

        float listY = layout.panelY + kListY;
        float maxListHeight = kListAreaH - kListY - 20.0f;
        int maxVisible = static_cast<int>(maxListHeight / kLineHeight);

        int scrollOffset = 0;
        if (m_selectedIndex >= maxVisible) scrollOffset = m_selectedIndex - maxVisible + 1;

        for (int i = scrollOffset; i < static_cast<int>(inventory.items.size()) && i < scrollOffset + maxVisible; ++i) {
            float y = listY + static_cast<float>(i - scrollOffset) * kLineHeight;
            sf::FloatRect rowRect({ layout.panelX + 20.0f, y }, { kPanelW - 40.0f, kLineHeight });
            if (rowRect.contains(mouse)) return i;
        }
        return -1;
    }

    void DrawButton(sf::RenderTarget& target, const sf::FloatRect& rect, const std::string& label, bool hovered, sf::Color fillColor) {
        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        sf::Color drawColor = hovered
            ? sf::Color((std::min)(255, fillColor.r + 35), (std::min)(255, fillColor.g + 35), (std::min)(255, fillColor.b + 35), fillColor.a)
            : fillColor;
        box.setFillColor(drawColor);
        box.setOutlineColor(sf::Color(200, 200, 200));
        box.setOutlineThickness(1.5f);
        target.draw(box);

        sf::Text text(*m_font, label, 15);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition({
            rect.position.x + (rect.size.x - textBounds.size.x) / 2.0f - textBounds.position.x,
            rect.position.y + (rect.size.y - textBounds.size.y) / 2.0f - textBounds.position.y
            });
        target.draw(text);
    }

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
        ClampSelection(inventory);
    }

    void SellSelected(InventoryComponent& inventory, CharacterStatsComponent& stats) {
        if (inventory.items.empty()) return;
        const ItemComponent& item = inventory.items[m_selectedIndex];
        int goldValue = ItemFactory::SellValue(item);
        stats.gold += goldValue;

        lastActionMessage = "Sold " + item.baseName + " for " + std::to_string(goldValue) + " gold";
        messageTimer = 2.5f;

        inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        ClampSelection(inventory);
    }

    void DiscardSelected(InventoryComponent& inventory) {
        if (inventory.items.empty()) return;
        const ItemComponent& item = inventory.items[m_selectedIndex];

        lastActionMessage = "Discarded: " + item.baseName;
        messageTimer = 2.5f;

        inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        ClampSelection(inventory);
    }

    // std::clamp(x, 0, size-1) is undefined behavior once size reaches 0 (lo > hi);
    // this happens whenever the last item is equipped/sold/discarded.
    void ClampSelection(const InventoryComponent& inventory) {
        if (inventory.items.empty()) {
            m_selectedIndex = 0;
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);
        }
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
