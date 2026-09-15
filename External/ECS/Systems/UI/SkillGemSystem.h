#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "../../Registry/Registry.h"
#include "../../Components/PlayerSkill/PlayerSkill.h"
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Skill/SkillGemData.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"

// Lets the player freely reassign any of the 5 skill slots to any gem they have
// ever picked up (SkillGemInventoryComponent::unlockedGemIds). Unlike the passive
// tree, swapping skill gems costs nothing and can be undone anytime (matches PoE).
class SkillGemSystem {
private:
    std::shared_ptr<sf::Font> m_font;
    int m_selectedSlot = 0;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    SkillGemSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() { isOpen = !isOpen; }
    void Close() { isOpen = false; }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, PlayerSkill, SkillGemInventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& skillComp = registry.GetComponent<PlayerSkill>(player);
        auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);

        auto& keyInput = InputManager::Instance().GetKeyInput();

        if (keyInput.IsGetKey(sf::Keyboard::Key::Down)) m_selectedSlot = (m_selectedSlot + 1) % 5;
        if (keyInput.IsGetKey(sf::Keyboard::Key::Up)) m_selectedSlot = (m_selectedSlot - 1 + 5) % 5;

        if (keyInput.IsGetKey(sf::Keyboard::Key::Left)) CycleGem(skillComp, gemInventory, -1);
        if (keyInput.IsGetKey(sf::Keyboard::Key::Right)) CycleGem(skillComp, gemInventory, 1);
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, PlayerSkill, SkillGemInventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& skillComp = registry.GetComponent<PlayerSkill>(player);
        auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::Vector2u winSize = target.getSize();
        float panelW = 640.0f;
        float panelH = 420.0f;
        float panelX = (winSize.x - panelW) / 2.0f;
        float panelY = (winSize.y - panelH) / 2.0f;

        sf::RectangleShape bg({ panelW, panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        auto& binds = KeyBindings::Instance();
        std::string closeKey = KeyToString(binds.Get(GameAction::ToggleSkillGems));
        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "Skill Gems (" + closeKey + " to close) - Up/Down select slot, Left/Right change gem", 15, sf::Color(255, 220, 120));
        const std::string kSlotKeys[5] = {
            KeyToString(binds.Get(GameAction::Skill1)),
            KeyToString(binds.Get(GameAction::Skill2)),
            KeyToString(binds.Get(GameAction::Skill3)),
            KeyToString(binds.Get(GameAction::Skill4)),
            KeyToString(binds.Get(GameAction::Skill5)),
        };
        float rowY = panelY + 55.0f;
        float rowHeight = 30.0f;

        for (int i = 0; i < 5; ++i) {
            float y = rowY + static_cast<float>(i) * rowHeight;
            bool selected = (i == m_selectedSlot);

            if (selected) {
                sf::RectangleShape highlight({ panelW - 40.0f, rowHeight });
                highlight.setPosition({ panelX + 20.0f, y });
                highlight.setFillColor(sf::Color(60, 60, 90, 180));
                target.draw(highlight);
            }

            const SkillData& skill = skillComp.skills[i];
            std::string line = "[" + std::string(kSlotKeys[i]) + "] " +
                (skill.isValid ? skill.name : "-- Empty --");
            if (skill.isValid) {
                line += "  (cd " + FormatFloat(skill.cooldownTime) + "s, " + std::to_string(skill.mpCost) + " mp)";
            }

            DrawText(target, panelX + 26.0f, y + 6.0f, line, 14,
                skill.isValid ? sf::Color(220, 220, 220) : sf::Color(120, 120, 120));
        }

        float listY = rowY + 5 * rowHeight + 20.0f;
        DrawText(target, panelX + 20.0f, listY, "Known gems:", 13, sf::Color(200, 200, 200));

        std::string known;
        for (int id : gemInventory.unlockedGemIds) {
            const GemDefinition* def = SkillGemData::Find(id);
            if (!def) continue;
            if (!known.empty()) known += ", ";
            known += def->skill.name;
        }
        if (known.empty()) known = "(none)";
        DrawText(target, panelX + 20.0f, listY + 20.0f, known, 13, sf::Color(160, 200, 255));

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + panelH - 24.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    // Option list for the selected slot: -1 = Empty, followed by every unlocked gem id.
    std::vector<int> Options(const SkillGemInventoryComponent& gemInventory) const {
        std::vector<int> opts = { -1 };
        opts.insert(opts.end(), gemInventory.unlockedGemIds.begin(), gemInventory.unlockedGemIds.end());
        return opts;
    }

    void CycleGem(PlayerSkill& skillComp, const SkillGemInventoryComponent& gemInventory, int dir) {
        std::vector<int> opts = Options(gemInventory);
        int currentId = skillComp.skills[m_selectedSlot].isValid ? skillComp.skills[m_selectedSlot].gemId : -1;

        int idx = 0;
        for (size_t i = 0; i < opts.size(); ++i) {
            if (opts[i] == currentId) { idx = static_cast<int>(i); break; }
        }

        int count = static_cast<int>(opts.size());
        idx = (idx + dir + count) % count;
        AssignGem(skillComp, opts[idx]);
    }

    void AssignGem(PlayerSkill& skillComp, int gemId) {
        SkillData& slot = skillComp.skills[m_selectedSlot];

        if (gemId < 0) {
            slot = SkillData{};
            lastActionMessage = "Slot " + std::to_string(m_selectedSlot + 1) + ": Empty";
        } else {
            const GemDefinition* def = SkillGemData::Find(gemId);
            if (!def) return;
            slot = def->skill;
            slot.gemId = gemId;
            slot.isValid = true;
            slot.currentCooldown = 0.0f;
            lastActionMessage = "Slot " + std::to_string(m_selectedSlot + 1) + ": " + slot.name;
        }
        messageTimer = 2.0f;
    }

    std::string FormatFloat(float value) const {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f", value);
        return std::string(buf);
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
