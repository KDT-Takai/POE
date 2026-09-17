#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include "../../Registry/Registry.h"
#include "../../Components/Progression/Atlas.h"
#include "../../Components/Item/Inventory.h"
#include "../../Components/Tags/Player/Player.h"
#include "AtlasData.h"
#include "../UI/ItemUIHelpers.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include <System/Campaign/CampaignManager.h>

// PoE2-style Atlas screen. Opened from the hub's map device when no map attempt is
// currently active (see GameScene::TryOpenEndgameMapFromHub) -- reuses
// PassiveTreeSystem's full-screen pan/zoom pattern for the node graph (left), with the
// player's bag docked on the right (same grid InventorySystem/StashSystem use) so a
// Waystone can be dragged straight from the bag into the "map device" socket that
// appears once a node is selected. Dropping a Waystone whose tier matches the selected
// node's tier there immediately consumes it and opens the map; anything else bounces
// back with a message (see AI/DECISIONS.md).
class AtlasSystem {
private:
    static constexpr float kMargin = 30.0f;
    static constexpr float kHeaderH = 70.0f;
    static constexpr float kFooterH = 90.0f;
    static constexpr float kButtonW = 150.0f;
    static constexpr float kButtonH = 36.0f;
    static constexpr float kNodeRadius = 9.0f;
    static constexpr float kMinNodeRadiusPx = 3.0f;
    static constexpr float kMaxNodeRadiusPx = 18.0f;

    static constexpr float kMinZoom = 0.5f;
    static constexpr float kMaxZoom = 3.0f;
    static constexpr float kZoomStep = 0.12f;
    static constexpr float kDragThreshold = 6.0f;

    // Safety cap on how far out nodes are actually drawn/hit-tested per frame -- the
    // unlock logic itself has no such cap (AtlasData::IsUnlocked works at any distance),
    // this only bounds the O(radius^2) render/click cost for an extremely deep run.
    static constexpr int kMaxDisplayRadius = 60;

    static constexpr float kBagCellSize = 60.0f;
    static constexpr float kBagCellGap = 6.0f;
    static constexpr float kBagGridTopY = 96.0f; // leaves room for the socket square above it
    static constexpr float kBagGridW = ItemUIHelpers::kBagGridCols * kBagCellSize + (ItemUIHelpers::kBagGridCols - 1) * kBagCellGap;
    static constexpr float kBagPanelW = kBagGridW + 40.0f;
    static constexpr float kBagGap = 20.0f;

    struct PanelLayout {
        float panelX = 0.0f, panelY = 0.0f, panelW = 0.0f, panelH = 0.0f;
        float areaX = 0.0f, areaY = 0.0f, areaW = 0.0f, areaH = 0.0f;
        sf::Vector2f baseCenter;
        float baseScale = 1.0f;
        sf::Vector2f center;
        float scale = 1.0f;
        sf::FloatRect closeBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect bagBox{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect socketRect{ {0.0f, 0.0f}, {0.0f, 0.0f} };
    };

    std::shared_ptr<sf::Font> m_font;
    bool m_hasSelection = false;
    int m_selectedCol = 0;
    int m_selectedRow = 0;

    sf::Vector2f m_panOffset{ 0.0f, 0.0f };
    float m_zoom = 1.0f;
    bool m_wasMouseDown = false;
    bool m_dragCandidate = false;
    bool m_isDragging = false;
    sf::Vector2f m_dragStartMouse;
    sf::Vector2f m_dragStartPan;

    // Bag-item drag (into the Waystone socket, or freely repositioned within the bag --
    // see AI/DECISIONS.md "自由配置"). Separate from the node-pan drag state above:
    // starting on a bag item takes priority over node panning/selection.
    bool m_bagDragging = false;
    int m_dragBagIndex = -1;
    ItemComponent m_dragItemCache;

    bool m_mapStartRequested = false;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    AtlasSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Open() {
        isOpen = true;
        m_panOffset = { 0.0f, 0.0f };
        m_zoom = 1.0f;
        m_hasSelection = false;
        m_bagDragging = false;
    }
    void Close() { isOpen = false; m_bagDragging = false; }

    // GameScene polls this once per frame after Update(); true means a node's map was
    // just opened this frame (Waystone consumed, CampaignManager::OpenEndgameMap already
    // called) and the caller should transition the same way the old direct-from-hub flow
    // always did (AdvanceToNextZone()).
    bool ConsumeMapStartRequest() {
        bool requested = m_mapStartRequested;
        m_mapStartRequested = false;
        return requested;
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, AtlasComponent, InventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& atlas = registry.GetComponent<AtlasComponent>(player);
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        ItemUIHelpers::NormalizeBagPlacement(inventory.items);

        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        sf::Vector2f mouse = mouseInput.GetMousePointF();
        int nodeRadius = VisibleRadius(atlas);

        if (m_bagDragging) {
            if (!mouseInput.GetMouse(sf::Mouse::Button::Left)) {
                PanelLayout dropLayout = ComputeLayout(window->getSize(), nodeRadius);
                HandleBagDrop(atlas, inventory, mouse, dropLayout);
                m_bagDragging = false;
            }
            return;
        }

        float wheel = InputManager::Instance().GetMouseWheelDelta();
        if (wheel != 0.0f) {
            PanelLayout preZoom = ComputeLayout(window->getSize(), nodeRadius);
            sf::Vector2f worldRaw = (mouse - preZoom.center) / preZoom.scale;
            m_zoom = std::clamp(m_zoom + wheel * kZoomStep, kMinZoom, kMaxZoom);
            float newScale = preZoom.baseScale * m_zoom;
            m_panOffset = mouse - worldRaw * newScale - preZoom.baseCenter;
        }

        PanelLayout layout = ComputeLayout(window->getSize(), nodeRadius);

        bool mouseDown = mouseInput.GetMouse(sf::Mouse::Button::Left);
        bool justPressed = mouseInput.IsGetMouse(sf::Mouse::Button::Left);
        bool justReleased = m_wasMouseDown && !mouseDown;
        m_wasMouseDown = mouseDown;

        if (justPressed) {
            int bagHit = HitTestBagItem(layout, mouse, inventory.items);
            if (bagHit >= 0) {
                m_bagDragging = true;
                m_dragBagIndex = bagHit;
                m_dragItemCache = inventory.items[bagHit];
                return;
            }

            m_dragCandidate = true;
            m_isDragging = false;
            m_dragStartMouse = mouse;
            m_dragStartPan = m_panOffset;
        }

        if (m_dragCandidate && mouseDown) {
            sf::Vector2f delta = mouse - m_dragStartMouse;
            if (!m_isDragging && (delta.x * delta.x + delta.y * delta.y) > kDragThreshold * kDragThreshold) {
                m_isDragging = true;
            }
            if (m_isDragging) {
                m_panOffset = m_dragStartPan + delta;
            }
        }

        if (justReleased) {
            if (m_dragCandidate && !m_isDragging) {
                int hitCol, hitRow;
                if (HitTestNode(layout, mouse, nodeRadius, hitCol, hitRow)) {
                    m_hasSelection = true;
                    m_selectedCol = hitCol;
                    m_selectedRow = hitRow;
                } else if (layout.closeBtn.contains(mouse)) {
                    Close();
                }
            }
            m_dragCandidate = false;
            m_isDragging = false;
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, AtlasComponent, InventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        const auto& atlas = registry.GetComponent<AtlasComponent>(player);
        const auto& inventory = registry.GetComponent<InventoryComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        int nodeRadius = VisibleRadius(atlas);
        PanelLayout layout = ComputeLayout(target.getSize(), nodeRadius);
        float panelX = layout.panelX;
        float panelY = layout.panelY;

        sf::RectangleShape bg({ layout.panelW, layout.panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(10, 12, 18, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "Atlas - Click a node, then drag a Waystone into the socket (Esc to close)", 15, sf::Color(255, 220, 120));

        int furthestDist = AtlasData::FurthestCompletedDistance(atlas);
        int furthestTier = AtlasData::TierForCoord(furthestDist, 0);
        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            "Furthest cleared: Tier " + std::to_string(furthestTier), 14, sf::Color(200, 200, 200));

        // --- Node graph (left) ---
        sf::Vector2u winSize = target.getSize();
        sf::View atlasView(sf::FloatRect({ layout.areaX, layout.areaY }, { layout.areaW, layout.areaH }));
        atlasView.setViewport(sf::FloatRect(
            { layout.areaX / static_cast<float>(winSize.x), layout.areaY / static_cast<float>(winSize.y) },
            { layout.areaW / static_cast<float>(winSize.x), layout.areaH / static_cast<float>(winSize.y) }));
        target.setView(atlasView);

        for (int col = -nodeRadius; col <= nodeRadius; ++col) {
            for (int row = -nodeRadius; row <= nodeRadius; ++row) {
                bool completed = AtlasData::IsCompleted(atlas, col, row);
                bool unlocked = AtlasData::IsUnlocked(atlas, col, row);

                if (unlocked || completed) {
                    DrawNeighborLine(target, layout, atlas, col, row, col + 1, row);
                    DrawNeighborLine(target, layout, atlas, col, row, col, row + 1);
                }

                sf::Vector2f pos = NodeScreenPos(layout, col, row);
                float radius = NodeRadiusPx(col == 0 && row == 0);
                sf::CircleShape circle(radius);
                circle.setOrigin({ radius, radius });
                circle.setPosition(pos);

                bool selected = m_hasSelection && col == m_selectedCol && row == m_selectedRow;
                sf::Color fill = completed ? sf::Color(80, 200, 120) : (unlocked ? sf::Color(90, 130, 200) : sf::Color(50, 50, 58));
                circle.setFillColor(fill);
                circle.setOutlineColor(selected ? sf::Color::Yellow : sf::Color(150, 150, 150));
                circle.setOutlineThickness(selected ? 3.0f : 1.0f);
                target.draw(circle);
            }
        }

        target.setView(target.getDefaultView());

        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();

        if (m_hasSelection) {
            int tier = AtlasData::TierForCoord(m_selectedCol, m_selectedRow);
            bool unlocked = AtlasData::IsUnlocked(atlas, m_selectedCol, m_selectedRow);
            std::string info = "Tier " + std::to_string(tier) + " Map";
            if (!unlocked) info += "  [Complete an adjacent map first]";
            else info += "  (owned: " + std::to_string(CountOwnedTier(inventory, tier)) + ")";
            DrawText(target, panelX + 20.0f, panelY + layout.panelH - kFooterH + 4.0f, info, 14, sf::Color(220, 220, 220));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + layout.panelH - kFooterH + 22.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        DrawButton(target, layout.closeBtn, "Close", layout.closeBtn.contains(mouse), sf::Color(130, 60, 60));

        if (!m_bagDragging) {
            int hoverCol, hoverRow;
            if (HitTestNode(layout, mouse, nodeRadius, hoverCol, hoverRow)) {
                DrawNodeTooltip(target, mouse, atlas, inventory, hoverCol, hoverRow);
            }
        }

        // --- Bag panel + Waystone socket (right) ---
        RenderBagPanel(target, layout, inventory, mouse);

        target.setView(oldView);
    }

private:
    int VisibleRadius(const AtlasComponent& atlas) const {
        int r = AtlasData::FurthestCompletedDistance(atlas) + 2;
        return std::clamp(r, 3, kMaxDisplayRadius);
    }

    PanelLayout ComputeLayout(sf::Vector2u winSize, int nodeRadius) const {
        PanelLayout layout;
        layout.panelX = kMargin;
        layout.panelY = kMargin;
        layout.panelW = (std::max)(100.0f, static_cast<float>(winSize.x) - kMargin * 2.0f);
        layout.panelH = (std::max)(100.0f, static_cast<float>(winSize.y) - kMargin * 2.0f);

        layout.bagBox = sf::FloatRect(
            { layout.panelX + layout.panelW - kBagPanelW, layout.panelY + kHeaderH },
            { kBagPanelW, layout.panelH - kHeaderH });
        layout.socketRect = sf::FloatRect(
            { layout.bagBox.position.x + 20.0f, layout.bagBox.position.y + 10.0f },
            { kBagCellSize, kBagCellSize });

        layout.areaX = layout.panelX;
        layout.areaY = layout.panelY + kHeaderH;
        layout.areaW = (std::max)(50.0f, layout.panelW - kBagPanelW - kBagGap);
        layout.areaH = (std::max)(50.0f, layout.panelH - kHeaderH - kFooterH);
        layout.baseCenter = { layout.areaX + layout.areaW / 2.0f, layout.areaY + layout.areaH / 2.0f };

        float maxPixelRadius = (std::max)(1.0f, static_cast<float>(nodeRadius) * AtlasData::kSpacing);
        float availableHalfExtent = (std::min)(layout.areaW, layout.areaH) / 2.0f;
        layout.baseScale = (availableHalfExtent * 0.92f) / maxPixelRadius;

        layout.scale = layout.baseScale * m_zoom;
        layout.center = layout.baseCenter + m_panOffset;

        float btnX = layout.areaX + (layout.areaW - kButtonW) / 2.0f;
        float btnY = layout.panelY + layout.panelH - kFooterH + 44.0f;
        layout.closeBtn = sf::FloatRect({ btnX, btnY }, { kButtonW, kButtonH });
        return layout;
    }

    sf::Vector2f NodeScreenPos(const PanelLayout& layout, int col, int row) const {
        float x, y;
        AtlasData::NodePos(col, row, x, y);
        return layout.center + sf::Vector2f(x, y) * layout.scale;
    }

    float NodeRadiusPx(bool isStart) const {
        float base = isStart ? kNodeRadius * 1.3f : kNodeRadius;
        return std::clamp(base * m_zoom, kMinNodeRadiusPx, kMaxNodeRadiusPx);
    }

    bool HitTestNode(const PanelLayout& layout, sf::Vector2f mouse, int nodeRadius, int& outCol, int& outRow) const {
        for (int col = -nodeRadius; col <= nodeRadius; ++col) {
            for (int row = -nodeRadius; row <= nodeRadius; ++row) {
                sf::Vector2f pos = NodeScreenPos(layout, col, row);
                float radius = NodeRadiusPx(col == 0 && row == 0) + 6.0f;
                sf::Vector2f d = mouse - pos;
                if (d.x * d.x + d.y * d.y <= radius * radius) { outCol = col; outRow = row; return true; }
            }
        }
        return false;
    }

    void DrawNeighborLine(sf::RenderTarget& target, const PanelLayout& layout, const AtlasComponent& atlas, int col, int row, int col2, int row2) {
        if (!AtlasData::IsUnlocked(atlas, col2, row2) && !AtlasData::IsCompleted(atlas, col2, row2)) return;
        bool bothCompleted = AtlasData::IsCompleted(atlas, col, row) && AtlasData::IsCompleted(atlas, col2, row2);
        sf::Color color = bothCompleted ? sf::Color(120, 200, 255) : sf::Color(80, 80, 90);
        std::array<sf::Vertex, 2> line = { {
            { NodeScreenPos(layout, col, row), color },
            { NodeScreenPos(layout, col2, row2), color }
        } };
        target.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
    }

    static int CountOwnedTier(const InventoryComponent& inventory, int tier) {
        int count = 0;
        for (const auto& item : inventory.items) {
            if (item.category == ItemCategory::Waystone && item.waystoneTier == tier) count++;
        }
        return count;
    }

    // --- Bag panel (right side): same 6x4 grid as InventorySystem/StashSystem, docked so
    // a Waystone can be dragged straight out of it into the socket above.

    sf::FloatRect BagCellRect(const PanelLayout& layout, int col, int row) const {
        float x = layout.bagBox.position.x + 20.0f + static_cast<float>(col) * (kBagCellSize + kBagCellGap);
        float y = layout.bagBox.position.y + kBagGridTopY + static_cast<float>(row) * (kBagCellSize + kBagCellGap);
        return sf::FloatRect({ x, y }, { kBagCellSize, kBagCellSize });
    }

    sf::FloatRect BagItemRect(const PanelLayout& layout, const ItemComponent& item) const {
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item);
        sf::FloatRect topLeft = BagCellRect(layout, item.gridCol, item.gridRow);
        float w = static_cast<float>(sz.x) * kBagCellSize + static_cast<float>(sz.x - 1) * kBagCellGap;
        float h = static_cast<float>(sz.y) * kBagCellSize + static_cast<float>(sz.y - 1) * kBagCellGap;
        return sf::FloatRect(topLeft.position, { w, h });
    }

    int HitTestBagItem(const PanelLayout& layout, sf::Vector2f mouse, const std::vector<ItemComponent>& items) const {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].gridCol < 0 || items[i].gridRow < 0) continue;
            if (BagItemRect(layout, items[i]).contains(mouse)) return static_cast<int>(i);
        }
        return -1;
    }

    bool MouseToBagCell(const PanelLayout& layout, sf::Vector2f mouse, int& outCol, int& outRow) const {
        float relX = mouse.x - (layout.bagBox.position.x + 20.0f);
        float relY = mouse.y - (layout.bagBox.position.y + kBagGridTopY);
        if (relX < 0.0f || relY < 0.0f) return false;
        int col = static_cast<int>(relX / (kBagCellSize + kBagCellGap));
        int row = static_cast<int>(relY / (kBagCellSize + kBagCellGap));
        if (col < 0 || col >= ItemUIHelpers::kBagGridCols || row < 0 || row >= ItemUIHelpers::kBagGridRows) return false;
        outCol = col;
        outRow = row;
        return true;
    }

    void HandleBagDrop(AtlasComponent& atlas, InventoryComponent& inventory, sf::Vector2f mouse, const PanelLayout& layout) {
        if (m_dragBagIndex < 0 || m_dragBagIndex >= static_cast<int>(inventory.items.size())) return;

        if (m_hasSelection && layout.socketRect.contains(mouse)) {
            TryPlaceWaystone(atlas, inventory, m_dragBagIndex);
            return;
        }

        int targetCol, targetRow;
        if (MouseToBagCell(layout, mouse, targetCol, targetRow)) {
            ItemUIHelpers::TryMoveWithinGrid(inventory.items, m_dragBagIndex, targetCol, targetRow);
        }
        // Dropped anywhere else (outside both the bag grid and the socket): stays where
        // it was, matching this project's "invalid drop target = no change" convention.
    }

    void TryPlaceWaystone(AtlasComponent& atlas, InventoryComponent& inventory, int bagIndex) {
        if (!AtlasData::IsUnlocked(atlas, m_selectedCol, m_selectedRow)) {
            lastActionMessage = "Complete an adjacent map first";
            messageTimer = 2.0f;
            return;
        }

        const ItemComponent& item = inventory.items[bagIndex];
        int requiredTier = AtlasData::TierForCoord(m_selectedCol, m_selectedRow);
        if (item.category != ItemCategory::Waystone) {
            lastActionMessage = "Only Waystones can go in the map device";
            messageTimer = 2.0f;
            return;
        }
        if (item.waystoneTier != requiredTier) {
            lastActionMessage = "Requires a Tier " + std::to_string(requiredTier) + " Waystone (this is Tier " + std::to_string(item.waystoneTier) + ")";
            messageTimer = 2.5f;
            return;
        }

        std::vector<WaystoneMod> mods = item.waystoneMods;
        inventory.items.erase(inventory.items.begin() + bagIndex);
        CampaignManager::Instance().OpenEndgameMap(requiredTier, mods);
        CampaignManager::Instance().SetPendingAtlasNode(m_selectedCol, m_selectedRow);
        m_mapStartRequested = true;
        Close();
    }

    void RenderBagPanel(sf::RenderTarget& target, const PanelLayout& layout, const InventoryComponent& inventory, sf::Vector2f mouse) {
        sf::RectangleShape bagBg(layout.bagBox.size);
        bagBg.setPosition(layout.bagBox.position);
        bagBg.setFillColor(sf::Color(15, 15, 20, 235));
        bagBg.setOutlineColor(sf::Color(150, 150, 160));
        bagBg.setOutlineThickness(2.0f);
        target.draw(bagBg);

        DrawText(target, layout.bagBox.position.x + 20.0f + kBagCellSize + 12.0f, layout.bagBox.position.y + 20.0f,
            "Your Bag", 14, sf::Color(220, 220, 220));

        // Waystone socket: only meaningful once a node is selected; before that it just
        // shows a hint so the player knows to pick a map first.
        sf::RectangleShape socketBox(layout.socketRect.size);
        socketBox.setPosition(layout.socketRect.position);
        bool socketHovered = m_bagDragging && layout.socketRect.contains(mouse);
        socketBox.setFillColor(m_hasSelection ? (socketHovered ? sf::Color(70, 90, 60) : sf::Color(35, 45, 30)) : sf::Color(25, 25, 30));
        socketBox.setOutlineColor(m_hasSelection ? sf::Color(150, 220, 150) : sf::Color(90, 90, 95));
        socketBox.setOutlineThickness(socketHovered ? 3.0f : 1.5f);
        target.draw(socketBox);
        if (!m_hasSelection) {
            DrawWrappedHint(target, layout.socketRect.position.x + kBagCellSize + 12.0f, layout.socketRect.position.y,
                "Select a map node first", kBagCellSize);
        } else {
            DrawWrappedHint(target, layout.socketRect.position.x + kBagCellSize + 12.0f, layout.socketRect.position.y,
                "Drop a matching Waystone here", kBagCellSize);
        }

        for (int row = 0; row < ItemUIHelpers::kBagGridRows; ++row) {
            for (int col = 0; col < ItemUIHelpers::kBagGridCols; ++col) {
                sf::FloatRect rect = BagCellRect(layout, col, row);
                sf::RectangleShape box(rect.size);
                box.setPosition(rect.position);
                box.setFillColor(sf::Color(25, 25, 30));
                box.setOutlineColor(sf::Color(70, 70, 78));
                box.setOutlineThickness(1.0f);
                target.draw(box);
            }
        }

        const ItemComponent* hoveredItem = nullptr;
        for (size_t i = 0; i < inventory.items.size(); ++i) {
            const ItemComponent& item = inventory.items[i];
            if (item.gridCol < 0 || item.gridRow < 0) continue;
            if (m_bagDragging && static_cast<int>(i) == m_dragBagIndex) continue;

            sf::FloatRect rect = BagItemRect(layout, item);
            bool hovered = rect.contains(mouse);
            sf::Color rc = ItemUIHelpers::RarityColor(item.rarity);
            sf::Color fill(rc.r / 4, rc.g / 4, rc.b / 4);

            sf::RectangleShape box(rect.size);
            box.setPosition(rect.position);
            box.setFillColor(hovered ? sf::Color((std::min)(255, fill.r + 25), (std::min)(255, fill.g + 25), (std::min)(255, fill.b + 25)) : fill);
            box.setOutlineColor(hovered ? sf::Color::Yellow : rc);
            box.setOutlineThickness(hovered ? 2.5f : 1.5f);
            target.draw(box);

            sf::Vector2i itemSz = ItemUIHelpers::ItemGridSize(item);
            DrawText(target, rect.position.x + 3.0f, rect.position.y + 3.0f, Truncate(item.baseName, static_cast<size_t>(itemSz.x) * 9), 10, rc);
            DrawText(target, rect.position.x + 3.0f, rect.position.y + rect.size.y - 14.0f, ItemUIHelpers::CompactLevelLabel(item), 9, sf::Color(190, 190, 190));

            if (hovered && !m_bagDragging) hoveredItem = &item;
        }

        if (hoveredItem) {
            DrawBagItemTooltip(target, mouse, *hoveredItem);
        }

        if (m_bagDragging) {
            float gw = static_cast<float>(ItemUIHelpers::ItemGridSize(m_dragItemCache).x) * kBagCellSize;
            float gh = static_cast<float>(ItemUIHelpers::ItemGridSize(m_dragItemCache).y) * kBagCellSize;
            sf::RectangleShape ghost({ gw, gh });
            ghost.setPosition({ mouse.x - gw / 2.0f, mouse.y - gh / 2.0f });
            sf::Color rc = ItemUIHelpers::RarityColor(m_dragItemCache.rarity);
            ghost.setFillColor(sf::Color(rc.r, rc.g, rc.b, 160));
            ghost.setOutlineColor(sf::Color::White);
            ghost.setOutlineThickness(2.0f);
            target.draw(ghost);
            DrawText(target, mouse.x - gw / 2.0f + 4.0f, mouse.y - gh / 2.0f + 4.0f, Truncate(m_dragItemCache.baseName, 8), 11, sf::Color::Black);
        }
    }

    void DrawWrappedHint(sf::RenderTarget& target, float x, float y, const std::string& text, float lineHeightHint) {
        DrawText(target, x, y, text, 11, sf::Color(150, 150, 155));
    }

    std::string Truncate(const std::string& s, size_t maxLen) const {
        if (s.size() <= maxLen) return s;
        return s.substr(0, maxLen);
    }

    void DrawBagItemTooltip(sf::RenderTarget& target, sf::Vector2f mouse, const ItemComponent& item) {
        struct Line { std::string text; sf::Color color; };
        std::vector<Line> lines;
        lines.push_back({ item.baseName, ItemUIHelpers::RarityColor(item.rarity) });
        lines.push_back({ ItemUIHelpers::ItemTypeLine(item), sf::Color(190, 190, 190) });
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

    void DrawNodeTooltip(sf::RenderTarget& target, sf::Vector2f mouse, const AtlasComponent& atlas, const InventoryComponent& inventory, int col, int row) {
        int tier = AtlasData::TierForCoord(col, row);
        bool completed = AtlasData::IsCompleted(atlas, col, row);
        bool unlocked = AtlasData::IsUnlocked(atlas, col, row);

        std::string body = "Tier " + std::to_string(tier) + " Map";
        if (completed) body += "\n(Cleared, can be run again)";
        else if (unlocked) body += "\nOwned Waystones: " + std::to_string(CountOwnedTier(inventory, tier));
        else body += "\n(Complete an adjacent map first)";

        sf::Text text(*m_font, sf::String::fromUtf8(body.begin(), body.end()), 14);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);

        sf::FloatRect bounds = text.getLocalBounds();
        const float padding = 8.0f;
        float boxW = bounds.size.x + padding * 2.0f;
        float boxH = bounds.size.y + bounds.position.y + padding * 2.0f;

        sf::Vector2f pos = mouse + sf::Vector2f(18.0f, -10.0f);
        sf::Vector2u winSize = target.getSize();
        if (pos.x + boxW > static_cast<float>(winSize.x)) pos.x = mouse.x - boxW - 18.0f;
        if (pos.y + boxH > static_cast<float>(winSize.y)) pos.y = static_cast<float>(winSize.y) - boxH - 4.0f;
        if (pos.y < 0.0f) pos.y = 4.0f;

        sf::RectangleShape box({ boxW, boxH });
        box.setPosition(pos);
        box.setFillColor(sf::Color(15, 15, 20, 235));
        box.setOutlineColor(sf::Color(150, 150, 160));
        box.setOutlineThickness(1.5f);
        target.draw(box);

        text.setPosition({ pos.x + padding - bounds.position.x, pos.y + padding - bounds.position.y });
        target.draw(text);
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

        sf::Text text(*m_font, sf::String::fromUtf8(label.begin(), label.end()), 16);
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

    void DrawText(sf::RenderTarget& target, float x, float y, const std::string& str, unsigned int size, sf::Color color) {
        sf::Text text(*m_font, sf::String::fromUtf8(str.begin(), str.end()), size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }
};
