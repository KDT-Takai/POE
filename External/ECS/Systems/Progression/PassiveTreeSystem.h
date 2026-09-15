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

        auto& keyInput = InputManager::Instance().GetKeyInput();
        sf::Vector2f dir(0.0f, 0.0f);
        bool moved = false;
        if (keyInput.IsGetKey(sf::Keyboard::Key::Left))  { dir = { -1.0f, 0.0f }; moved = true; }
        if (keyInput.IsGetKey(sf::Keyboard::Key::Right)) { dir = { 1.0f, 0.0f }; moved = true; }
        if (keyInput.IsGetKey(sf::Keyboard::Key::Up))    { dir = { 0.0f, -1.0f }; moved = true; }
        if (keyInput.IsGetKey(sf::Keyboard::Key::Down))  { dir = { 0.0f, 1.0f }; moved = true; }

        if (moved) {
            MoveSelection(dir);
        }

        if (keyInput.IsGetKey(sf::Keyboard::Key::Enter)) {
            TryAllocate(tree, registry.GetComponent<EquipmentComponent>(player), registry.GetComponent<CharacterStatsComponent>(player));
        }

        if (keyInput.IsGetKey(sf::Keyboard::Key::Backspace)) {
            TryDeallocate(tree, registry.GetComponent<EquipmentComponent>(player), registry.GetComponent<CharacterStatsComponent>(player));
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

        sf::Vector2u winSize = target.getSize();
        float panelW = 700.0f;
        float panelH = 560.0f;
        float panelX = (winSize.x - panelW) / 2.0f;
        float panelY = (winSize.y - panelH) / 2.0f;

        sf::RectangleShape bg({ panelW, panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        std::string closeKey = KeyToString(KeyBindings::Instance().Get(GameAction::TogglePassiveTree));
        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "Passive Tree (" + closeKey + " to close) - Arrows move, Enter allocate, Backspace respec", 15, sf::Color(255, 220, 120));
        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            "Available Points: " + std::to_string(stats.passivePoints) +
            "   Orbs of Regret: " + std::to_string(stats.regretOrbs), 14, sf::Color(200, 200, 200));

        sf::Vector2f center(panelX + panelW / 2.0f, panelY + panelH / 2.0f + 15.0f);

        for (const auto& node : PassiveTreeData::Nodes()) {
            for (int neighborId : node.neighbors) {
                if (neighborId < node.id) continue;
                const PassiveNodeDef* neighbor = PassiveTreeData::Find(neighborId);
                if (!neighbor) continue;

                bool bothAllocated = IsAllocated(tree, node.id) && IsAllocated(tree, neighborId);
                sf::Color lineColor = bothAllocated ? sf::Color(120, 200, 255) : sf::Color(80, 80, 90);
                std::array<sf::Vertex, 2> line = { {
                    { center + sf::Vector2f(node.x, node.y), lineColor },
                    { center + sf::Vector2f(neighbor->x, neighbor->y), lineColor }
                } };
                target.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
            }
        }

        for (const auto& node : PassiveTreeData::Nodes()) {
            sf::Vector2f pos = center + sf::Vector2f(node.x, node.y);
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
            DrawText(target, panelX + 20.0f, panelY + panelH - 50.0f, info, 14, sf::Color(220, 220, 220));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + panelH - 24.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    void MoveSelection(const sf::Vector2f& dir) {
        const PassiveNodeDef* current = PassiveTreeData::Find(m_selectedNodeId);
        if (!current) return;

        int bestId = -1;
        float bestDot = 0.3f;
        for (int neighborId : current->neighbors) {
            const PassiveNodeDef* neighbor = PassiveTreeData::Find(neighborId);
            if (!neighbor) continue;

            sf::Vector2f delta(neighbor->x - current->x, neighbor->y - current->y);
            float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (len < 0.001f) continue;
            delta /= len;

            float dot = delta.x * dir.x + delta.y * dir.y;
            if (dot > bestDot) {
                bestDot = dot;
                bestId = neighborId;
            }
        }
        if (bestId != -1) m_selectedNodeId = bestId;
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
