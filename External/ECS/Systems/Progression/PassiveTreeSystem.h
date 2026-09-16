#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <algorithm>
#include <cmath>
#include "../../Registry/Registry.h"
#include "../../Components/Progression/PassiveTree.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Item/EquipmentSystem.h"
#include "PassiveTreeData.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"

class PassiveTreeSystem {
private:
    // Fills the whole window (unlike the other menus, which stay as small fixed-size
    // panels) since the tree now has enough nodes/branches that a 700x560 box would be
    // too cramped to read or click accurately. kMargin keeps a border so nodes never
    // touch the screen edge; kHeaderH/kFooterH reserve fixed screen-space strips for the
    // title/points text and the selected-node info + Confirm/Cancel buttons.
    static constexpr float kMargin = 30.0f;
    static constexpr float kHeaderH = 70.0f;
    static constexpr float kFooterH = 90.0f;
    static constexpr float kButtonW = 150.0f;
    static constexpr float kButtonH = 36.0f;
    static constexpr float kButtonGap = 24.0f;
    // Base node radii at zoom=1 (the "fit whole tree" default). These scale with m_zoom
    // (see NodeRadiusPx) so node spacing and node size shrink/grow together -- a fixed
    // screen-pixel radius would make nodes overlap once zoomed out past their spacing.
    // Clamped to a min/max pixel range so they never vanish or balloon at the extremes.
    static constexpr float kNodeRadius = 10.0f;
    static constexpr float kStartNodeRadius = 12.0f;
    static constexpr float kMinNodeRadiusPx = 4.0f;
    static constexpr float kMaxNodeRadiusPx = 22.0f;

    // Pan (left-drag) and zoom (wheel) range. minZoom is well below 1.0 so the tree can
    // shrink below its "fit whole tree" size too, not just enlarge past it.
    static constexpr float kMinZoom = 0.5f;
    static constexpr float kMaxZoom = 3.0f;
    static constexpr float kZoomStep = 0.12f; // per wheel notch (delta is usually +-1)
    static constexpr float kDragThreshold = 6.0f; // px of movement before a press counts as a drag, not a click

    struct PanelLayout {
        float panelX = 0.0f;
        float panelY = 0.0f;
        float panelW = 0.0f;
        float panelH = 0.0f;
        float treeAreaX = 0.0f;
        float treeAreaY = 0.0f;
        float treeAreaW = 0.0f;
        float treeAreaH = 0.0f;
        sf::Vector2f baseCenter; // center/scale with zoom=1, pan=0 (the "fit to screen" values)
        float baseScale = 1.0f;
        sf::Vector2f center; // baseCenter + current pan offset
        float scale = 1.0f;  // baseScale * current zoom
        sf::FloatRect confirmBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect cancelBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
    };

    std::shared_ptr<sf::Font> m_font;
    int m_selectedNodeId = 0;

    sf::Vector2f m_panOffset{ 0.0f, 0.0f };
    float m_zoom = 1.0f;
    bool m_wasMouseDown = false;
    bool m_dragCandidate = false;
    bool m_isDragging = false;
    sf::Vector2f m_dragStartMouse;
    sf::Vector2f m_dragStartPan;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    PassiveTreeSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() {
        isOpen = !isOpen;
        if (isOpen) {
            m_panOffset = { 0.0f, 0.0f };
            m_zoom = 1.0f;
        }
    }
    void Close() { isOpen = false; }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, PassiveTreeComponent, EquipmentComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& tree = registry.GetComponent<PassiveTreeComponent>(player);
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        auto& keyInput = InputManager::Instance().GetKeyInput();
        if (keyInput.IsGetKey(sf::Keyboard::Key::Backspace)) {
            TryDeallocate(tree, equipment, stats);
        }

        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        // Mouse wheel zoom, anchored on the cursor (the raw tree-space point under the
        // cursor stays fixed on screen) rather than on the tree center, matching how
        // real PoE's tree zoom feels.
        float wheel = InputManager::Instance().GetMouseWheelDelta();
        if (wheel != 0.0f) {
            PanelLayout preZoom = ComputeLayout(window->getSize());
            sf::Vector2f worldRaw = (mouse - preZoom.center) / preZoom.scale;
            m_zoom = std::clamp(m_zoom + wheel * kZoomStep, kMinZoom, kMaxZoom);
            float newScale = preZoom.baseScale * m_zoom;
            m_panOffset = mouse - worldRaw * newScale - preZoom.baseCenter;
        }

        PanelLayout layout = ComputeLayout(window->getSize());

        // Left-drag pans the tree; a press that never moves past kDragThreshold is
        // treated as a plain click instead (select a node / press Confirm or Cancel).
        // MouseInput has no "just released" query, so the release edge is tracked here
        // via the previous frame's held state.
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
                int clickedNode = HitTestNode(layout, mouse);
                if (clickedNode != -1) {
                    m_selectedNodeId = clickedNode;
                } else if (layout.confirmBtn.contains(mouse)) {
                    TryAllocate(tree, equipment, stats);
                } else if (layout.cancelBtn.contains(mouse)) {
                    Close();
                }
            }
            m_dragCandidate = false;
            m_isDragging = false;
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, PassiveTreeComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& tree = registry.GetComponent<PassiveTreeComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        PanelLayout layout = ComputeLayout(target.getSize());
        float panelX = layout.panelX;
        float panelY = layout.panelY;

        sf::RectangleShape bg({ layout.panelW, layout.panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        std::string closeKey = KeyToString(KeyBindings::Instance().Get(GameAction::TogglePassiveTree));
        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "Passive Tree (" + closeKey + " to close) - Click a node to select, Backspace respec", 15, sf::Color(255, 220, 120));
        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            "Available Points: " + std::to_string(stats.passivePoints) +
            "   Orbs of Regret: " + std::to_string(stats.regretOrbs), 14, sf::Color(200, 200, 200));

        // Panning/zooming can otherwise push nodes/lines up over the header text or down
        // past the footer buttons, so the tree itself draws through a view whose viewport
        // clips to the tree area (1 view unit == 1 pixel, so NodeScreenPos's absolute
        // pixel coordinates still land in the right place; only the clipping changes).
        sf::Vector2u winSize = target.getSize();
        sf::View treeView(sf::FloatRect({ layout.treeAreaX, layout.treeAreaY }, { layout.treeAreaW, layout.treeAreaH }));
        treeView.setViewport(sf::FloatRect(
            { layout.treeAreaX / static_cast<float>(winSize.x), layout.treeAreaY / static_cast<float>(winSize.y) },
            { layout.treeAreaW / static_cast<float>(winSize.x), layout.treeAreaH / static_cast<float>(winSize.y) }));
        target.setView(treeView);

        for (const auto& node : PassiveTreeData::Nodes()) {
            for (int neighborId : node.neighbors) {
                if (neighborId < node.id) continue;
                const PassiveNodeDef* neighbor = PassiveTreeData::Find(neighborId);
                if (!neighbor) continue;

                bool bothAllocated = IsAllocated(tree, node.id) && IsAllocated(tree, neighborId);
                sf::Color lineColor = bothAllocated ? sf::Color(120, 200, 255) : sf::Color(80, 80, 90);
                std::array<sf::Vertex, 2> line = { {
                    { NodeScreenPos(layout, node), lineColor },
                    { NodeScreenPos(layout, *neighbor), lineColor }
                } };
                target.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
            }
        }

        for (const auto& node : PassiveTreeData::Nodes()) {
            sf::Vector2f pos = NodeScreenPos(layout, node);
            float radius = NodeRadiusPx(node.id == 0);

            sf::CircleShape circle(radius);
            circle.setOrigin({ radius, radius });
            circle.setPosition(pos);

            bool allocated = IsAllocated(tree, node.id);
            bool selected = (node.id == m_selectedNodeId);

            circle.setFillColor(allocated ? sf::Color(80, 200, 120) : sf::Color(60, 60, 70));
            circle.setOutlineColor(selected ? sf::Color::Yellow : sf::Color(150, 150, 150));
            circle.setOutlineThickness(selected ? 3.0f : 1.5f);
            target.draw(circle);
        }

        target.setView(target.getDefaultView());

        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();

        const PassiveNodeDef* selectedNode = PassiveTreeData::Find(m_selectedNodeId);
        if (selectedNode && m_selectedNodeId != 0) {
            bool isPercent = IsPercentStat(selectedNode->effect.stat);
            std::string info = selectedNode->effect.label + ": +" +
                std::to_string(static_cast<int>(selectedNode->effect.value)) + (isPercent ? "%" : "");
            if (IsAllocated(tree, m_selectedNodeId)) info += " (allocated)";
            DrawText(target, panelX + 20.0f, panelY + layout.panelH - kFooterH + 4.0f, info, 14, sf::Color(220, 220, 220));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + layout.panelH - kFooterH + 22.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        bool canConfirm = m_selectedNodeId != 0 && !IsAllocated(tree, m_selectedNodeId);
        DrawButton(target, layout.confirmBtn, "Confirm", layout.confirmBtn.contains(mouse),
            canConfirm ? sf::Color(60, 130, 70) : sf::Color(60, 60, 65));
        DrawButton(target, layout.cancelBtn, "Cancel", layout.cancelBtn.contains(mouse), sf::Color(130, 60, 60));

        // Hover tooltip so browsing the tree doesn't require clicking every node first;
        // skipped while actively dragging (panning) so it doesn't flicker under the cursor.
        if (!m_isDragging) {
            int hoveredId = HitTestNode(layout, mouse);
            const PassiveNodeDef* hovered = (hoveredId != -1) ? PassiveTreeData::Find(hoveredId) : nullptr;
            if (hovered) {
                DrawNodeTooltip(target, mouse, tree, *hovered);
            }
        }

        target.setView(oldView);
    }

private:
    PanelLayout ComputeLayout(sf::Vector2u winSize) const {
        PanelLayout layout;
        layout.panelX = kMargin;
        layout.panelY = kMargin;
        layout.panelW = (std::max)(100.0f, static_cast<float>(winSize.x) - kMargin * 2.0f);
        layout.panelH = (std::max)(100.0f, static_cast<float>(winSize.y) - kMargin * 2.0f);

        layout.treeAreaX = layout.panelX;
        layout.treeAreaY = layout.panelY + kHeaderH;
        layout.treeAreaW = layout.panelW;
        layout.treeAreaH = (std::max)(50.0f, layout.panelH - kHeaderH - kFooterH);
        layout.baseCenter = { layout.treeAreaX + layout.treeAreaW / 2.0f, layout.treeAreaY + layout.treeAreaH / 2.0f };

        // Fit the tree's full radius inside the smaller of the area's half-width/height,
        // with a little padding so outer nodes don't touch the header/footer/edges.
        float maxRadius = (std::max)(1.0f, PassiveTreeData::MaxRadius());
        float availableHalfExtent = (std::min)(layout.treeAreaW, layout.treeAreaH) / 2.0f;
        layout.baseScale = (availableHalfExtent * 0.92f) / maxRadius;

        layout.scale = layout.baseScale * m_zoom;
        layout.center = layout.baseCenter + m_panOffset;

        float totalBtnW = kButtonW * 2.0f + kButtonGap;
        float btnX = layout.panelX + (layout.panelW - totalBtnW) / 2.0f;
        float btnY = layout.panelY + layout.panelH - kFooterH + 44.0f;
        layout.confirmBtn = sf::FloatRect({ btnX, btnY }, { kButtonW, kButtonH });
        layout.cancelBtn = sf::FloatRect({ btnX + kButtonW + kButtonGap, btnY }, { kButtonW, kButtonH });
        return layout;
    }

    sf::Vector2f NodeScreenPos(const PanelLayout& layout, const PassiveNodeDef& node) const {
        return layout.center + sf::Vector2f(node.x, node.y) * layout.scale;
    }

    float NodeRadiusPx(bool isStart) const {
        float base = isStart ? kStartNodeRadius : kNodeRadius;
        return std::clamp(base * m_zoom, kMinNodeRadiusPx, kMaxNodeRadiusPx);
    }

    int HitTestNode(const PanelLayout& layout, sf::Vector2f mouse) const {
        for (const auto& node : PassiveTreeData::Nodes()) {
            sf::Vector2f pos = NodeScreenPos(layout, node);
            float radius = NodeRadiusPx(node.id == 0) + 6.0f; // slack for easier clicking
            sf::Vector2f d = mouse - pos;
            if (d.x * d.x + d.y * d.y <= radius * radius) return node.id;
        }
        return -1;
    }

    void DrawNodeTooltip(sf::RenderTarget& target, sf::Vector2f mouse, const PassiveTreeComponent& tree, const PassiveNodeDef& node) {
        std::string body;
        if (node.id == 0) {
            body = "Start";
        } else {
            bool isPercent = IsPercentStat(node.effect.stat);
            body = node.effect.label + ": +" + std::to_string(static_cast<int>(node.effect.value)) + (isPercent ? "%" : "");
            if (IsAllocated(tree, node.id)) {
                body += "\n(Allocated)";
            } else {
                bool adjacentAllocated = false;
                for (int neighborId : node.neighbors) {
                    if (IsAllocated(tree, neighborId)) { adjacentAllocated = true; break; }
                }
                body += adjacentAllocated ? "\nClick to allocate" : "\n(Not reachable yet)";
            }
        }

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

    void TryAllocate(PassiveTreeComponent& tree, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        if (m_selectedNodeId == 0) return;
        if (IsAllocated(tree, m_selectedNodeId)) return;

        if (stats.passivePoints <= 0) {
            lastActionMessage = "Not enough passive points";
            messageTimer = 2.0f;
            return;
        }

        const PassiveNodeDef* node = PassiveTreeData::Find(m_selectedNodeId);
        if (!node) return;

        bool adjacentAllocated = false;
        for (int neighborId : node->neighbors) {
            if (IsAllocated(tree, neighborId)) {
                adjacentAllocated = true;
                break;
            }
        }
        if (!adjacentAllocated) {
            lastActionMessage = "Allocate a connected node first";
            messageTimer = 2.0f;
            return;
        }

        tree.allocatedNodeIds.push_back(m_selectedNodeId);
        stats.passivePoints--;
        EquipmentSystem::ApplyAffix(equipment.baseStats, node->effect);
        EquipmentSystem::RecalculateStats(stats, equipment);

        lastActionMessage = "Allocated: " + node->effect.label;
        messageTimer = 2.0f;
    }

    void TryDeallocate(PassiveTreeComponent& tree, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        if (m_selectedNodeId == 0) return;
        if (!IsAllocated(tree, m_selectedNodeId)) return;

        if (stats.regretOrbs <= 0) {
            lastActionMessage = "Need an Orb of Regret to respec";
            messageTimer = 2.0f;
            return;
        }

        if (!CanRemoveWithoutDisconnecting(tree, m_selectedNodeId)) {
            lastActionMessage = "Other allocated nodes depend on this one";
            messageTimer = 2.0f;
            return;
        }

        const PassiveNodeDef* node = PassiveTreeData::Find(m_selectedNodeId);
        if (!node) return;

        auto& ids = tree.allocatedNodeIds;
        ids.erase(std::remove(ids.begin(), ids.end(), m_selectedNodeId), ids.end());
        stats.passivePoints++;
        stats.regretOrbs--;
        EquipmentSystem::RemoveAffix(equipment.baseStats, node->effect);
        EquipmentSystem::RecalculateStats(stats, equipment);

        lastActionMessage = "Respecced: " + node->effect.label;
        messageTimer = 2.0f;
    }

    // Checks that removing `removeId` from the allocated set leaves every remaining
    // allocated node still reachable from the start node (id 0) through allocated nodes.
    bool CanRemoveWithoutDisconnecting(const PassiveTreeComponent& tree, int removeId) const {
        std::vector<int> remaining;
        for (int id : tree.allocatedNodeIds) {
            if (id != removeId) remaining.push_back(id);
        }

        std::vector<int> reachable = { 0 };
        std::vector<int> frontier = { 0 };
        while (!frontier.empty()) {
            int current = frontier.back();
            frontier.pop_back();
            const PassiveNodeDef* node = PassiveTreeData::Find(current);
            if (!node) continue;
            for (int neighborId : node->neighbors) {
                if (neighborId == removeId) continue;
                bool isRemainingAllocated = std::find(remaining.begin(), remaining.end(), neighborId) != remaining.end();
                if (!isRemainingAllocated) continue;
                if (std::find(reachable.begin(), reachable.end(), neighborId) != reachable.end()) continue;
                reachable.push_back(neighborId);
                frontier.push_back(neighborId);
            }
        }

        return reachable.size() == remaining.size() + 1;
    }

    bool IsAllocated(const PassiveTreeComponent& tree, int id) const {
        if (id == 0) return true;
        return std::find(tree.allocatedNodeIds.begin(), tree.allocatedNodeIds.end(), id) != tree.allocatedNodeIds.end();
    }

    bool IsPercentStat(AffixStat stat) const {
        switch (stat) {
        case AffixStat::IncreasedAttackDamage:
        case AffixStat::FireRes:
        case AffixStat::ColdRes:
        case AffixStat::LightningRes:
        case AffixStat::ChaosRes:
        case AffixStat::CritChance:
        case AffixStat::CritMultiplier:
        case AffixStat::MoveSpeed:
        case AffixStat::BlockChance:
            return true;
        default:
            return false;
        }
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
