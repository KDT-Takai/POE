#pragma once
#include "Components/Item/Item.h"
#include <vector>
#include <string>
#include <cmath>

struct PassiveNodeDef {
    int id;
    float x;
    float y;
    std::vector<int> neighbors;
    ItemAffix effect;
};

class PassiveTreeData {
public:
    static const std::vector<PassiveNodeDef>& Nodes() {
        static std::vector<PassiveNodeDef> nodes = BuildNodes();
        return nodes;
    }

    static const PassiveNodeDef* Find(int id) {
        for (const auto& node : Nodes()) {
            if (node.id == id) return &node;
        }
        return nullptr;
    }

private:
    struct BranchStep {
        AffixStat stat;
        float value;
        std::string label;
    };
    struct Branch {
        float angleDeg;
        std::vector<BranchStep> steps;
    };

    static std::vector<PassiveNodeDef> BuildNodes() {
        std::vector<PassiveNodeDef> nodes;
        nodes.push_back({ 0, 0.0f, 0.0f, {}, { AffixStat::FlatLife, 0.0f, 1, false, "Start" } });

        std::vector<Branch> branches = {
            { 0.0f, {
                { AffixStat::IncreasedAttackDamage, 8.0f, "Attack Damage" },
                { AffixStat::CritChance, 1.0f, "Critical Chance" },
                { AffixStat::FlatAccuracy, 20.0f, "Accuracy" },
                { AffixStat::CritMultiplier, 10.0f, "Critical Multiplier" },
                { AffixStat::IncreasedAttackDamage, 15.0f, "Attack Damage" },
            } },
            { 90.0f, {
                { AffixStat::FlatArmour, 10.0f, "Armour" },
                { AffixStat::FlatEvasion, 10.0f, "Evasion" },
                { AffixStat::FlatLife, 15.0f, "Life" },
                { AffixStat::FlatES, 10.0f, "Energy Shield" },
                { AffixStat::FlatArmour, 25.0f, "Armour" },
            } },
            { 180.0f, {
                { AffixStat::MoveSpeed, 3.0f, "Movement Speed" },
                { AffixStat::FlatMana, 15.0f, "Mana" },
                { AffixStat::FlatLife, 10.0f, "Life" },
                { AffixStat::MoveSpeed, 5.0f, "Movement Speed" },
                { AffixStat::FlatMana, 30.0f, "Mana" },
            } },
            { 270.0f, {
                { AffixStat::FireRes, 6.0f, "Fire Resistance" },
                { AffixStat::ColdRes, 6.0f, "Cold Resistance" },
                { AffixStat::LightningRes, 6.0f, "Lightning Resistance" },
                { AffixStat::ChaosRes, 4.0f, "Chaos Resistance" },
                { AffixStat::FlatES, 20.0f, "Energy Shield" },
            } },
        };

        int nextId = 1;
        const float radiusStep = 70.0f;
        const float degToRad = 3.14159265f / 180.0f;

        for (const auto& branch : branches) {
            float rad = branch.angleDeg * degToRad;
            int prevId = 0;
            for (size_t i = 0; i < branch.steps.size(); ++i) {
                int id = nextId++;
                float radius = radiusStep * static_cast<float>(i + 1);
                float x = std::cos(rad) * radius;
                float y = std::sin(rad) * radius;
                const BranchStep& step = branch.steps[i];

                nodes.push_back({ id, x, y, { prevId }, { step.stat, step.value, 1, false, step.label } });
                for (auto& n : nodes) {
                    if (n.id == prevId) {
                        n.neighbors.push_back(id);
                        break;
                    }
                }
                prevId = id;
            }
        }
        return nodes;
    }
};
