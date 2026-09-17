#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "../../Registry/Registry.h"
#include "../../Components/Item/Item.h"
#include "../../Components/Item/Inventory.h"
#include "../../Components/Item/Stash.h"
#include "../../Components/Tags/Player/Player.h"
#include "ItemUIHelpers.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"

// Persistent item storage separate from the carried bag (see StashComponent), split into
// StashComponent::kTabCount independent pages (PoE-style stash tabs, switched via tab
// buttons at the top of the left box). Two boxes side by side like VendorSystem's
// own-panel + player-bag layout: left = the active Stash tab's grid, right = the player's
// bag (same 6x4 grid InventorySystem/VendorSystem render). Dragging an item from one box
// and releasing over the other moves it there (first free slot in the target grid, not
// the exact drop position -- same simplification VendorSystem's bag-to-shop sell drag
// already uses). No gold/trading involved, purely storage.
class StashSystem {
private:
    static constexpr float kBoxGap = 24.0f;
    static constexpr float kCellSize = 60.0f;
    static constexpr float kCellGap = 6.0f;

    static constexpr float kStashGridW = StashComponent::kCols * kCellSize + (StashComponent::kCols - 1) * kCellGap;
    static constexpr float kStashGridH = StashComponent::kRows * kCellSize + (StashComponent::kRows - 1) * kCellGap;
    static constexpr float kBagGridW = ItemUIHelpers::kBagGridCols * kCellSize + (ItemUIHelpers::kBagGridCols - 1) * kCellGap;

    static constexpr float kLeftW = kStashGridW + 40.0f;
    static constexpr float kRightW = kBagGridW + 40.0f;
    static constexpr float kPanelW = kLeftW + kBoxGap + kRightW;
    static constexpr float kGridTopY = 96.0f; // leaves room for the tab row above the grid
    static constexpr float kPanelH = kGridTopY + kStashGridH + 30.0f;
    static constexpr float kTabBtnW = 60.0f;
    static constexpr float kTabBtnH = 24.0f;
    static constexpr float kTabRowY = 56.0f;

    std::shared_ptr<sf::Font> m_font;
    int m_activeTab = 0;

    bool m_dragging = false;
    bool m_dragFromStash = false;
    int m_dragIndex = -1;
    ItemComponent m_dragItemCache;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    StashSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Open() { isOpen = true; m_dragging = false; }
    void Close() { isOpen = false; m_dragging = false; }
    void Toggle() { if (isOpen) Close(); else Open(); }

    bool IsPointInPanel(sf::Vector2f point, sf::Vector2u winSize) const {
        if (!isOpen) return false;
        return PanelBounds(winSize).contains(point);
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, InventoryComponent, StashComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        auto& stash = registry.GetComponent<StashComponent>(player);
        ItemUIHelpers::NormalizeBagPlacement(inventory.items);
        for (auto& tab : stash.tabs) ItemUIHelpers::NormalizeBagPlacement(tab, StashComponent::kCols, StashComponent::kRows);
        auto& activeTabItems = stash.tabs[m_activeTab];

        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;
        auto& mouseInput = InputManager::Instance().GetMouseInput();
        sf::Vector2f mouse = mouseInput.GetMousePointF();
        Layout layout = ComputeLayout(window->getSize());

        if (m_dragging) {
            if (!mouseInput.GetMouse(sf::Mouse::Button::Left)) {
                if (!m_dragFromStash && layout.leftBox.contains(mouse)) {
                    MoveItem(inventory.items, activeTabItems, m_dragIndex, StashComponent::kCols, StashComponent::kRows, "Stash");
                } else if (m_dragFromStash && layout.rightBox.contains(mouse)) {
                    MoveItem(activeTabItems, inventory.items, m_dragIndex, ItemUIHelpers::kBagGridCols, ItemUIHelpers::kBagGridRows, "Bag");
                }
                m_dragging = false;
            }
            return;
        }

        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;

        if (layout.closeBtnRect.contains(mouse)) { Close(); return; }

        for (int t = 0; t < StashComponent::kTabCount; ++t) {
            if (TabRect(layout, t).contains(mouse)) { m_activeTab = t; return; }
        }

        int stashHit = HitTestGrid(layout, mouse, activeTabItems, true);
        if (stashHit >= 0) {
            m_dragging = true;
            m_dragFromStash = true;
            m_dragIndex = stashHit;
            m_dragItemCache = activeTabItems[stashHit];
            return;
        }
        int bagHit = HitTestGrid(layout, mouse, inventory.items, false);
        if (bagHit >= 0) {
            m_dragging = true;
            m_dragFromStash = false;
            m_dragIndex = bagHit;
            m_dragItemCache = inventory.items[bagHit];
            return;
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, InventoryComponent, StashComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        const auto& inventory = registry.GetComponent<InventoryComponent>(player);
        const auto& stash = registry.GetComponent<StashComponent>(player);
        const auto& activeTabItems = stash.tabs[m_activeTab];

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        Layout layout = ComputeLayout(target.getSize());

        DrawBox(target, layout.leftBox);
        DrawBox(target, layout.rightBox);

        DrawText(target, layout.leftBox.position.x + 16.0f, layout.leftBox.position.y + 10.0f, "Stash", 14, sf::Color(255, 220, 120));
        DrawText(target, layout.rightBox.position.x + 16.0f, layout.rightBox.position.y + 10.0f, "Your Bag", 14, sf::Color(220, 220, 220));

        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();

        for (int t = 0; t < StashComponent::kTabCount; ++t) {
            sf::FloatRect tabRect = TabRect(layout, t);
            bool active = (t == m_activeTab);
            sf::RectangleShape tabBtn(tabRect.size);
            tabBtn.setPosition(tabRect.position);
            tabBtn.setFillColor(active ? sf::Color(70, 70, 110) : sf::Color(35, 35, 40));
            tabBtn.setOutlineColor(sf::Color(150, 150, 160));
            tabBtn.setOutlineThickness(1.0f);
            target.draw(tabBtn);
            DrawText(target, tabRect.position.x + 14.0f, tabRect.position.y + 5.0f, std::to_string(t + 1), 13, sf::Color::White);
        }

        sf::RectangleShape closeBox(layout.closeBtnRect.size);
        closeBox.setPosition(layout.closeBtnRect.position);
        bool closeHovered = layout.closeBtnRect.contains(mouse);
        closeBox.setFillColor(closeHovered ? sf::Color(140, 50, 50) : sf::Color(90, 35, 35));
        closeBox.setOutlineColor(sf::Color(200, 150, 150));
        closeBox.setOutlineThickness(1.0f);
        target.draw(closeBox);
        DrawText(target, layout.closeBtnRect.position.x + 18.0f, layout.closeBtnRect.position.y + 5.0f, "Close", 13, sf::Color::White);

        const ItemComponent* hovered = nullptr;

        DrawGridCells(target, layout, true, StashComponent::kCols, StashComponent::kRows);
        DrawItems(target, layout, activeTabItems, true, mouse, hovered);

        DrawGridCells(target, layout, false, ItemUIHelpers::kBagGridCols, ItemUIHelpers::kBagGridRows);
        DrawItems(target, layout, inventory.items, false, mouse, hovered);

        if (hovered) DrawItemTooltip(target, mouse, *hovered);

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, layout.leftBox.position.x + 16.0f, layout.leftBox.position.y + layout.leftBox.size.y - 22.0f,
                lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        if (m_dragging) {
            float gw = 70.0f, gh = 36.0f;
            sf::RectangleShape ghost({ gw, gh });
            ghost.setPosition({ mouse.x - gw / 2.0f, mouse.y - gh / 2.0f });
            sf::Color rc = ItemUIHelpers::RarityColor(m_dragItemCache.rarity);
            ghost.setFillColor(sf::Color(rc.r, rc.g, rc.b, 160));
            ghost.setOutlineColor(sf::Color::White);
            ghost.setOutlineThickness(2.0f);
            target.draw(ghost);
            DrawText(target, mouse.x - gw / 2.0f + 4.0f, mouse.y - gh / 2.0f + 4.0f, m_dragItemCache.baseName.substr(0, 8), 11, sf::Color::Black);
        }

        target.setView(oldView);
    }

private:
    struct Layout {
        sf::FloatRect leftBox;
        sf::FloatRect rightBox;
        sf::FloatRect closeBtnRect;
    };

    sf::FloatRect PanelBounds(sf::Vector2u winSize) const {
        float x = (static_cast<float>(winSize.x) - kPanelW) / 2.0f;
        float y = (static_cast<float>(winSize.y) - kPanelH) / 2.0f;
        return sf::FloatRect({ x, y }, { kPanelW, kPanelH });
    }

    Layout ComputeLayout(sf::Vector2u winSize) const {
        sf::FloatRect bounds = PanelBounds(winSize);
        Layout layout;
        layout.leftBox = sf::FloatRect(bounds.position, { kLeftW, kPanelH });
        layout.rightBox = sf::FloatRect({ bounds.position.x + kLeftW + kBoxGap, bounds.position.y }, { kRightW, kPanelH });
        layout.closeBtnRect = sf::FloatRect({ layout.rightBox.position.x + kRightW - 90.0f, bounds.position.y + 10.0f }, { 70.0f, 24.0f });
        return layout;
    }

    sf::FloatRect TabRect(const Layout& layout, int tab) const {
        float x = layout.leftBox.position.x + 20.0f + static_cast<float>(tab) * (kTabBtnW + 6.0f);
        float y = layout.leftBox.position.y + kTabRowY;
        return sf::FloatRect({ x, y }, { kTabBtnW, kTabBtnH });
    }

    void DrawBox(sf::RenderTarget& target, sf::FloatRect box) {
        sf::RectangleShape bg(box.size);
        bg.setPosition(box.position);
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);
    }

    sf::FloatRect CellRect(const Layout& layout, bool stash, int col, int row) const {
        const sf::FloatRect& box = stash ? layout.leftBox : layout.rightBox;
        float x = box.position.x + 20.0f + static_cast<float>(col) * (kCellSize + kCellGap);
        float y = box.position.y + kGridTopY + static_cast<float>(row) * (kCellSize + kCellGap);
        return sf::FloatRect({ x, y }, { kCellSize, kCellSize });
    }

    sf::FloatRect ItemRect(const Layout& layout, bool stash, const ItemComponent& item) const {
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item);
        sf::FloatRect topLeft = CellRect(layout, stash, item.gridCol, item.gridRow);
        float w = static_cast<float>(sz.x) * kCellSize + static_cast<float>(sz.x - 1) * kCellGap;
        float h = static_cast<float>(sz.y) * kCellSize + static_cast<float>(sz.y - 1) * kCellGap;
        return sf::FloatRect(topLeft.position, { w, h });
    }

    void DrawGridCells(sf::RenderTarget& target, const Layout& layout, bool stash, int cols, int rows) {
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                sf::FloatRect rect = CellRect(layout, stash, col, row);
                sf::RectangleShape box(rect.size);
                box.setPosition(rect.position);
                box.setFillColor(sf::Color(25, 25, 30));
                box.setOutlineColor(sf::Color(70, 70, 78));
                box.setOutlineThickness(1.0f);
                target.draw(box);
            }
        }
    }

    void DrawItems(sf::RenderTarget& target, const Layout& layout, const std::vector<ItemComponent>& items, bool stash,
        sf::Vector2f mouse, const ItemComponent*& outHovered) {
        for (size_t i = 0; i < items.size(); ++i) {
            const ItemComponent& item = items[i];
            if (item.gridCol < 0 || item.gridRow < 0) continue;
            bool draggingAway = m_dragging && m_dragFromStash == stash && static_cast<int>(i) == m_dragIndex;
            if (draggingAway) continue;

            sf::FloatRect rect = ItemRect(layout, stash, item);
            bool hovered = rect.contains(mouse);
            sf::Color rc = ItemUIHelpers::RarityColor(item.rarity);
            sf::Color fill(rc.r / 4, rc.g / 4, rc.b / 4);

            sf::RectangleShape box(rect.size);
            box.setPosition(rect.position);
            box.setFillColor(hovered ? sf::Color((std::min)(255, fill.r + 25), (std::min)(255, fill.g + 25), (std::min)(255, fill.b + 25)) : fill);
            box.setOutlineColor(hovered ? sf::Color::Yellow : rc);
            box.setOutlineThickness(hovered ? 2.0f : 1.5f);
            target.draw(box);

            sf::Vector2i itemSz = ItemUIHelpers::ItemGridSize(item);
            DrawText(target, rect.position.x + 3.0f, rect.position.y + 3.0f, Truncate(item.baseName, static_cast<size_t>(itemSz.x) * 9), 10, rc);
            DrawText(target, rect.position.x + 3.0f, rect.position.y + rect.size.y - 14.0f, ItemUIHelpers::CompactLevelLabel(item), 9, sf::Color(190, 190, 190));

            if (hovered) outHovered = &item;
        }
    }

    int HitTestGrid(const Layout& layout, sf::Vector2f mouse, const std::vector<ItemComponent>& items, bool stash) const {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].gridCol < 0 || items[i].gridRow < 0) continue;
            if (ItemRect(layout, stash, items[i]).contains(mouse)) return static_cast<int>(i);
        }
        return -1;
    }

    // Moves items[m_dragIndex] from `from` into `to` (first free cell in the target
    // grid's own dimensions); leaves it untouched if the target has no room.
    void MoveItem(std::vector<ItemComponent>& from, std::vector<ItemComponent>& to, int index, int targetCols, int targetRows,
        const std::string& targetName) {
        if (index < 0 || index >= static_cast<int>(from.size())) return;
        ItemComponent item = from[index];
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item);
        int col, row;
        if (!ItemUIHelpers::FindBagFreeSpace(to, sz.x, sz.y, col, row, targetCols, targetRows)) {
            lastActionMessage = targetName + " full";
            messageTimer = 2.0f;
            return;
        }
        item.gridCol = col;
        item.gridRow = row;
        to.push_back(item);
        from.erase(from.begin() + index);
        lastActionMessage = "Moved: " + item.baseName + " -> " + targetName;
        messageTimer = 2.0f;
    }

    std::string Truncate(const std::string& s, size_t maxLen) const {
        if (s.size() <= maxLen) return s;
        return s.substr(0, maxLen);
    }

    void DrawItemTooltip(sf::RenderTarget& target, sf::Vector2f mouse, const ItemComponent& item) {
        struct Line { std::string text; sf::Color color; };
        std::vector<Line> lines;

        lines.push_back({ item.baseName, ItemUIHelpers::RarityColor(item.rarity) });
        lines.push_back({ ItemUIHelpers::ItemTypeLine(item), sf::Color(190, 190, 190) });

        for (const auto& affix : item.affixes) {
            lines.push_back({ ItemUIHelpers::FormatAffixLine(affix), sf::Color(150, 200, 255) });
        }
        for (const auto& mod : item.waystoneMods) {
            lines.push_back({ ItemUIHelpers::FormatWaystoneModLine(mod), sf::Color(220, 160, 160) });
        }

        float lineH = 18.0f;
        float maxWidth = 0.0f;
        for (const auto& line : lines) {
            sf::Text probe(*m_font, sf::String::fromUtf8(line.text.begin(), line.text.end()), 13);
            maxWidth = (std::max)(maxWidth, probe.getLocalBounds().size.x);
        }

        float boxW = maxWidth + 24.0f;
        float boxH = static_cast<float>(lines.size()) * lineH + 16.0f;

        sf::Vector2u winSize = target.getSize();
        float x = mouse.x + 18.0f;
        float y = mouse.y + 10.0f;
        if (x + boxW > static_cast<float>(winSize.x) - 4.0f) x = mouse.x - boxW - 18.0f;
        if (y + boxH > static_cast<float>(winSize.y) - 4.0f) y = static_cast<float>(winSize.y) - boxH - 4.0f;

        sf::RectangleShape box({ boxW, boxH });
        box.setPosition({ x, y });
        box.setFillColor(sf::Color(10, 10, 14, 235));
        box.setOutlineColor(sf::Color(150, 150, 160));
        box.setOutlineThickness(1.5f);
        target.draw(box);

        for (size_t i = 0; i < lines.size(); ++i) {
            DrawText(target, x + 12.0f, y + 8.0f + static_cast<float>(i) * lineH, lines[i].text, 13, lines[i].color);
        }
    }

    void DrawText(sf::RenderTarget& target, float x, float y, const std::string& str, unsigned int size, sf::Color color) {
        sf::Text text(*m_font, sf::String::fromUtf8(str.begin(), str.end()), size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }
};
