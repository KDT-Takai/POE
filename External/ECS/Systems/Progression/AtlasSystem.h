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
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include <System/Campaign/CampaignManager.h>

// PoE2-style Atlas screen. Opened from the hub's map device when no map attempt is
// currently active (see GameScene::TryOpenEndgameMapFromHub) -- reuses
// PassiveTreeSystem's full-screen pan/zoom pattern, but nodes are procedurally derived
// (AtlasData) instead of a fixed list, and the visible window grows outward as the
// player clears nodes further from the start. Selecting an unlocked node and pressing
// "Open Map" consumes one matching-tier Waystone from the bag and starts that map
// (mirrors the old direct-from-hub flow, just routed through node selection first).
class AtlasSystem {
private:
    static constexpr float kMargin = 30.0f;
    static constexpr float kHeaderH = 70.0f;
    static constexpr float kFooterH = 90.0f;
    static constexpr float kButtonW = 150.0f;
    static constexpr float kButtonH = 36.0f;
    static constexpr float kButtonGap = 24.0f;
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

    struct PanelLayout {
        float panelX = 0.0f, panelY = 0.0f, panelW = 0.0f, panelH = 0.0f;
        float areaX = 0.0f, areaY = 0.0f, areaW = 0.0f, areaH = 0.0f;
        sf::Vector2f baseCenter;
        float baseScale = 1.0f;
        sf::Vector2f center;
        float scale = 1.0f;
        sf::FloatRect openBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect closeBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
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
    }
    void Close() { isOpen = false; }

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

        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        sf::Vector2f mouse = mouseInput.GetMousePointF();
        int nodeRadius = VisibleRadius(atlas);

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
                } else if (layout.openBtn.contains(mouse)) {
                    TryOpenSelected(atlas, inventory);
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
            "Atlas - Click a node then Open Map (Esc to close)", 15, sf::Color(255, 220, 120));

        int furthestDist = AtlasData::FurthestCompletedDistance(atlas);
        int furthestTier = AtlasData::TierForCoord(furthestDist, 0);
        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            "Furthest cleared: Tier " + std::to_string(furthestTier) + "   Held Waystones: " + HeldSummary(inventory),
            14, sf::Color(200, 200, 200));

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

        bool canOpen = m_hasSelection && AtlasData::IsUnlocked(atlas, m_selectedCol, m_selectedRow)
            && CountOwnedTier(inventory, AtlasData::TierForCoord(m_selectedCol, m_selectedRow)) > 0;
        DrawButton(target, layout.openBtn, "Open Map", layout.openBtn.contains(mouse), canOpen ? sf::Color(60, 130, 70) : sf::Color(60, 60, 65));
        DrawButton(target, layout.closeBtn, "Close", layout.closeBtn.contains(mouse), sf::Color(130, 60, 60));

        if (!m_isDragging) {
            int hoverCol, hoverRow;
            if (HitTestNode(layout, mouse, nodeRadius, hoverCol, hoverRow)) {
                DrawNodeTooltip(target, mouse, atlas, inventory, hoverCol, hoverRow);
            }
        }

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

        layout.areaX = layout.panelX;
        layout.areaY = layout.panelY + kHeaderH;
        layout.areaW = layout.panelW;
        layout.areaH = (std::max)(50.0f, layout.panelH - kHeaderH - kFooterH);
        layout.baseCenter = { layout.areaX + layout.areaW / 2.0f, layout.areaY + layout.areaH / 2.0f };

        float maxPixelRadius = (std::max)(1.0f, static_cast<float>(nodeRadius) * AtlasData::kSpacing);
        float availableHalfExtent = (std::min)(layout.areaW, layout.areaH) / 2.0f;
        layout.baseScale = (availableHalfExtent * 0.92f) / maxPixelRadius;

        layout.scale = layout.baseScale * m_zoom;
        layout.center = layout.baseCenter + m_panOffset;

        float totalBtnW = kButtonW * 2.0f + kButtonGap;
        float btnX = layout.panelX + (layout.panelW - totalBtnW) / 2.0f;
        float btnY = layout.panelY + layout.panelH - kFooterH + 44.0f;
        layout.openBtn = sf::FloatRect({ btnX, btnY }, { kButtonW, kButtonH });
        layout.closeBtn = sf::FloatRect({ btnX + kButtonW + kButtonGap, btnY }, { kButtonW, kButtonH });
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

    static std::string HeldSummary(const InventoryComponent& inventory) {
        std::array<int, kMaxWaystoneTier> counts{};
        for (const auto& item : inventory.items) {
            if (item.category != ItemCategory::Waystone) continue;
            if (item.waystoneTier < 1 || item.waystoneTier > kMaxWaystoneTier) continue;
            counts[item.waystoneTier - 1]++;
        }
        std::string summary;
        for (int t = kMaxWaystoneTier; t >= 1; --t) {
            if (counts[t - 1] <= 0) continue;
            if (!summary.empty()) summary += " ";
            summary += "T" + std::to_string(t) + "x" + std::to_string(counts[t - 1]);
        }
        return summary.empty() ? "none" : summary;
    }

    void TryOpenSelected(AtlasComponent& atlas, InventoryComponent& inventory) {
        if (!m_hasSelection) return;
        if (!AtlasData::IsUnlocked(atlas, m_selectedCol, m_selectedRow)) {
            lastActionMessage = "Complete an adjacent map first";
            messageTimer = 2.0f;
            return;
        }

        int tier = AtlasData::TierForCoord(m_selectedCol, m_selectedRow);
        int itemIndex = -1;
        for (size_t i = 0; i < inventory.items.size(); ++i) {
            if (inventory.items[i].category == ItemCategory::Waystone && inventory.items[i].waystoneTier == tier) {
                itemIndex = static_cast<int>(i);
                break;
            }
        }
        if (itemIndex < 0) {
            lastActionMessage = "Requires a Tier " + std::to_string(tier) + " Waystone";
            messageTimer = 2.5f;
            return;
        }

        std::vector<WaystoneMod> mods = inventory.items[itemIndex].waystoneMods;
        inventory.items.erase(inventory.items.begin() + itemIndex);
        CampaignManager::Instance().OpenEndgameMap(tier, mods);
        CampaignManager::Instance().SetPendingAtlasNode(m_selectedCol, m_selectedRow);
        m_mapStartRequested = true;
        Close();
    }

    void DrawNodeTooltip(sf::RenderTarget& target, sf::Vector2f mouse, const AtlasComponent& atlas, const InventoryComponent& inventory, int col, int row) {
        int tier = AtlasData::TierForCoord(col, row);
        bool completed = AtlasData::IsCompleted(atlas, col, row);
        bool unlocked = AtlasData::IsUnlocked(atlas, col, row);

        std::string body = "Tier " + std::to_string(tier) + " Map";
        if (completed) body += "\n(Cleared, click Open Map to run again)";
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
