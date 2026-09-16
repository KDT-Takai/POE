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

    // Furthest any node sits from the start node, in the same raw units as x/y.
    // Used by PassiveTreeSystem to scale the whole tree to fit the screen.
    static float MaxRadius() {
        float maxSq = 0.0f;
        for (const auto& node : Nodes()) {
            float distSq = node.x * node.x + node.y * node.y;
            if (distSq > maxSq) maxSq = distSq;
        }
        return std::sqrt(maxSq);
    }

private:
    // A tree of steps radiating from the start node. Each depth level is one ring
    // (radius = kRadiusStep * depth); a step with more than one entry in `children`
    // is a fork -- multiple real paths the player must choose between, not just a
    // decorative branch. angleDeg is absolute (not relative to the parent), so each
    // step's screen position is fully explicit.
    struct StepNode {
        float angleDeg;
        AffixStat stat;
        float value;
        std::string label;
        std::vector<StepNode> children = {};
    };

    static constexpr float kRadiusStep = 70.0f;
    static constexpr float kDegToRad = 3.14159265f / 180.0f;

    static void AddChain(std::vector<PassiveNodeDef>& nodes, int parentId, int depth,
        const std::vector<StepNode>& siblings, int& nextId) {
        for (const auto& step : siblings) {
            int id = nextId++;
            float rad = step.angleDeg * kDegToRad;
            float radius = kRadiusStep * static_cast<float>(depth);
            float x = std::cos(rad) * radius;
            float y = std::sin(rad) * radius;

            nodes.push_back({ id, x, y, { parentId }, { step.stat, step.value, 1, false, step.label } });
            for (auto& n : nodes) {
                if (n.id == parentId) {
                    n.neighbors.push_back(id);
                    break;
                }
            }

            AddChain(nodes, id, depth + 1, step.children, nextId);
        }
    }

    static std::vector<PassiveNodeDef> BuildNodes() {
        std::vector<PassiveNodeDef> nodes;
        nodes.push_back({ 0, 0.0f, 0.0f, {}, { AffixStat::FlatLife, 0.0f, 1, false, "Start" } });

        // 0 deg: Offense. Splits into Crit / raw Damage+Accuracy / pure Accuracy arms.
        std::vector<StepNode> offense = { {
            0.0f, AffixStat::IncreasedAttackDamage, 8.0f, "Attack Damage", { {
                0.0f, AffixStat::CritChance, 1.0f, "Critical Chance", {
                    { -25.0f, AffixStat::CritMultiplier, 8.0f, "Critical Multiplier", { {
                        -25.0f, AffixStat::CritChance, 1.5f, "Critical Chance", {
                            { -35.0f, AffixStat::CritMultiplier, 15.0f, "Critical Multiplier" },
                            { -15.0f, AffixStat::IncreasedAttackDamage, 12.0f, "Attack Damage" },
                        }
                    } } },
                    { 0.0f, AffixStat::IncreasedAttackDamage, 10.0f, "Attack Damage", { {
                        0.0f, AffixStat::FlatAccuracy, 25.0f, "Accuracy", {
                            { -10.0f, AffixStat::IncreasedAttackDamage, 12.0f, "Attack Damage" },
                            { 10.0f, AffixStat::CritChance, 1.0f, "Critical Chance" },
                        }
                    } } },
                    { 25.0f, AffixStat::FlatAccuracy, 30.0f, "Accuracy", { {
                        25.0f, AffixStat::FlatAccuracy, 30.0f, "Accuracy", {
                            { 25.0f, AffixStat::IncreasedAttackDamage, 15.0f, "Attack Damage" },
                        }
                    } } },
                }
            } }
        } };

        // 90 deg: Defense. Splits into Armour+Block / Life+ES / Evasion+Block arms.
        std::vector<StepNode> defense = { {
            90.0f, AffixStat::FlatArmour, 10.0f, "Armour", { {
                90.0f, AffixStat::FlatEvasion, 10.0f, "Evasion", {
                    { 65.0f, AffixStat::FlatArmour, 25.0f, "Armour", { {
                        65.0f, AffixStat::FlatLife, 15.0f, "Life", {
                            { 55.0f, AffixStat::FlatArmour, 30.0f, "Armour" },
                            { 75.0f, AffixStat::BlockChance, 3.0f, "Block Chance" },
                        }
                    } } },
                    { 90.0f, AffixStat::FlatLife, 20.0f, "Life", { {
                        90.0f, AffixStat::FlatES, 15.0f, "Energy Shield", {
                            { 80.0f, AffixStat::FlatLife, 25.0f, "Life" },
                            { 100.0f, AffixStat::FlatES, 20.0f, "Energy Shield" },
                        }
                    } } },
                    { 115.0f, AffixStat::FlatEvasion, 25.0f, "Evasion", { {
                        115.0f, AffixStat::BlockChance, 4.0f, "Block Chance", {
                            { 115.0f, AffixStat::FlatEvasion, 30.0f, "Evasion" },
                        }
                    } } },
                }
            } }
        } };

        // 180 deg: Mobility/Mana. Splits into Speed / Mana / hybrid Life-Mana arms.
        std::vector<StepNode> mobility = { {
            180.0f, AffixStat::MoveSpeed, 3.0f, "Movement Speed", { {
                180.0f, AffixStat::FlatMana, 15.0f, "Mana", {
                    { 155.0f, AffixStat::MoveSpeed, 5.0f, "Movement Speed", { {
                        155.0f, AffixStat::MoveSpeed, 5.0f, "Movement Speed", {
                            { 145.0f, AffixStat::MoveSpeed, 8.0f, "Movement Speed" },
                            { 165.0f, AffixStat::FlatLife, 15.0f, "Life" },
                        }
                    } } },
                    { 180.0f, AffixStat::FlatMana, 30.0f, "Mana", { {
                        180.0f, AffixStat::FlatMana, 30.0f, "Mana", {
                            { 170.0f, AffixStat::FlatMana, 40.0f, "Mana" },
                            { 190.0f, AffixStat::FlatES, 15.0f, "Energy Shield" },
                        }
                    } } },
                    { 205.0f, AffixStat::FlatLife, 15.0f, "Life", { {
                        205.0f, AffixStat::FlatMana, 20.0f, "Mana", {
                            { 205.0f, AffixStat::FlatLife, 20.0f, "Life" },
                        }
                    } } },
                }
            } }
        } };

        // 270 deg: Resistances. Splits into Fire/Cold / Lightning+ES / pure Chaos arms.
        std::vector<StepNode> resist = { {
            270.0f, AffixStat::FireRes, 6.0f, "Fire Resistance", { {
                270.0f, AffixStat::ColdRes, 6.0f, "Cold Resistance", {
                    { 245.0f, AffixStat::FireRes, 8.0f, "Fire Resistance", { {
                        245.0f, AffixStat::ColdRes, 8.0f, "Cold Resistance", {
                            { 235.0f, AffixStat::FireRes, 10.0f, "Fire Resistance" },
                            { 255.0f, AffixStat::ChaosRes, 6.0f, "Chaos Resistance" },
                        }
                    } } },
                    { 270.0f, AffixStat::LightningRes, 8.0f, "Lightning Resistance", { {
                        270.0f, AffixStat::FlatES, 20.0f, "Energy Shield", {
                            { 260.0f, AffixStat::LightningRes, 10.0f, "Lightning Resistance" },
                            { 280.0f, AffixStat::ChaosRes, 6.0f, "Chaos Resistance" },
                        }
                    } } },
                    { 295.0f, AffixStat::ChaosRes, 6.0f, "Chaos Resistance", { {
                        295.0f, AffixStat::ChaosRes, 6.0f, "Chaos Resistance", {
                            { 295.0f, AffixStat::ChaosRes, 8.0f, "Chaos Resistance" },
                        }
                    } } },
                }
            } }
        } };

        int nextId = 1;
        AddChain(nodes, 0, 1, offense, nextId);
        AddChain(nodes, 0, 1, defense, nextId);
        AddChain(nodes, 0, 1, mobility, nextId);
        AddChain(nodes, 0, 1, resist, nextId);
        return nodes;
    }
};
