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
    // Tree area keeps its old footprint (branches reach up to 350px from center);
    // the footer below it is new space for the selected-node info + Confirm/Cancel.
    static constexpr float kPanelW = 700.0f;
    static constexpr float kTreeAreaH = 560.0f;
    static constexpr float kFooterH = 90.0f;
    static constexpr float kPanelH = kTreeAreaH + kFooterH;
    static constexpr float kButtonW = 150.0f;
    static constexpr float kButtonH = 36.0f;
    static constexpr float kButtonGap = 24.0f;

    struct PanelLayout {
        float panelX = 0.0f;
        float panelY = 0.0f;
        sf::Vector2f center;
        sf::FloatRect confirmBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect cancelBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
    };

    std::shared_ptr<sf::Font> m_font;
    int m_selectedNodeId = 0;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    PassiveTreeSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() { isOpen = !isOpen; }
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
        PanelLayout layout = ComputeLayout(window->getSize());

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (mouseInput.IsGetMouse(sf::Mouse::Button::Left)) {
            sf::Vector2f mouse = mouseInput.GetMousePointF();

            int clickedNode = HitTestNode(layout, mouse);
            if (clickedNode != -1) {
                m_selectedNodeId = clickedNode;
            } else if (layout.confirmBtn.contains(mouse)) {
                TryAllocate(tree, equipment, stats);
            } else if (layout.cancelBtn.contains(mouse)) {
                Close();
            }
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

        sf::RectangleShape bg({ kPanelW, kPanelH });
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

        for (const auto& node : PassiveTreeData::Nodes()) {
            for (int neighborId : node.neighbors) {
                if (neighborId < node.id) continue;
                const PassiveNodeDef* neighbor = PassiveTreeData::Find(neighborId);
                if (!neighbor) continue;

                bool bothAllocated = IsAllocated(tree, node.id) && IsAllocated(tree, neighborId);
                sf::Color lineColor = bothAllocated ? sf::Color(120, 200, 255) : sf::Color(80, 80, 90);
                std::array<sf::Vertex, 2> line = { {
                    { layout.center + sf::Vector2f(node.x, node.y), lineColor },
                    { layout.center + sf::Vector2f(neighbor->x, neighbor->y), lineColor }
                } };
                target.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
            }
        }

        for (const auto& node : PassiveTreeData::Nodes()) {
            sf::Vector2f pos = layout.center + sf::Vector2f(node.x, node.y);
            float radius = (node.id == 0) ? 12.0f : 10.0f;

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

        const PassiveNodeDef* selectedNode = PassiveTreeData::Find(m_selectedNodeId);
        if (selectedNode && m_selectedNodeId != 0) {
            bool isPercent = IsPercentStat(selectedNode->effect.stat);
            std::string info = selectedNode->effect.label + ": +" +
                std::to_string(static_cast<int>(selectedNode->effect.value)) + (isPercent ? "%" : "");
            if (IsAllocated(tree, m_selectedNodeId)) info += " (allocated)";
            DrawText(target, panelX + 20.0f, panelY + kTreeAreaH + 4.0f, info, 14, sf::Color(220, 220, 220));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + kTreeAreaH + 26.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();

        bool canConfirm = m_selectedNodeId != 0 && !IsAllocated(tree, m_selectedNodeId);
        DrawButton(target, layout.confirmBtn, "Confirm", layout.confirmBtn.contains(mouse),
            canConfirm ? sf::Color(60, 130, 70) : sf::Color(60, 60, 65));
        DrawButton(target, layout.cancelBtn, "Cancel", layout.cancelBtn.contains(mouse), sf::Color(130, 60, 60));

        target.setView(oldView);
    }

private:
    PanelLayout ComputeLayout(sf::Vector2u winSize) const {
        PanelLayout layout;
        layout.panelX = (static_cast<float>(winSize.x) - kPanelW) / 2.0f;
        layout.panelY = (static_cast<float>(winSize.y) - kPanelH) / 2.0f;
        layout.center = { layout.panelX + kPanelW / 2.0f, layout.panelY + kTreeAreaH / 2.0f + 15.0f };

        float totalBtnW = kButtonW * 2.0f + kButtonGap;
        float btnX = layout.panelX + (kPanelW - totalBtnW) / 2.0f;
        float btnY = layout.panelY + kTreeAreaH + 54.0f;
        layout.confirmBtn = sf::FloatRect({ btnX, btnY }, { kButtonW, kButtonH });
        layout.cancelBtn = sf::FloatRect({ btnX + kButtonW + kButtonGap, btnY }, { kButtonW, kButtonH });
        return layout;
    }

    int HitTestNode(const PanelLayout& layout, sf::Vector2f mouse) const {
        for (const auto& node : PassiveTreeData::Nodes()) {
            sf::Vector2f pos = layout.center + sf::Vector2f(node.x, node.y);
            float radius = (node.id == 0 ? 12.0f : 10.0f) + 6.0f; // slack for easier clicking
            sf::Vector2f d = mouse - pos;
            if (d.x * d.x + d.y * d.y <= radius * radius) return node.id;
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

        sf::Text text(*m_font, label, 16);
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
            return true;
        default:
            return false;
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
