#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "../../Registry/Registry.h"
#include "../../Components/PlayerSkill/PlayerSkill.h"
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Skill/SkillGemData.h"
#include "../Skill/SpiritAuraSystem.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"

// Lets the player freely reassign any of the 5 skill slots to any activated gem they
// have ever picked up, plus 2 Spirit slots for Aura-type gems (see SpiritAuraSystem).
// Unlike skill slots, Spirit slots are budget-limited by CharacterStatsComponent::maxSpirit.
class SkillGemSystem {
private:
    static constexpr int kSkillSlotCount = 5;
    static constexpr int kSpiritSlotCount = 2;

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

    // Used by GameScene to resolve click ownership between the non-pausing menus:
    // only steal the click if the cursor is actually over this panel's rect.
    bool IsPointInPanel(sf::Vector2f point) const {
        if (!isOpen) return false;
        return sf::FloatRect({ kPanelX, kPanelY }, { kPanelW, kPanelH }).contains(point);
    }

    // Selection and assignment are entirely mouse-driven: click a slot row to select it,
    // then click a gem in the list below to socket it. The old Up/Down/Left/Right keyboard
    // navigation was removed because this menu doesn't pause the world, so those arrow
    // keys were simultaneously moving the player (see InputSystem's movement handling).
    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, PlayerSkill, SkillGemInventoryComponent, SpiritGemLoadoutComponent, EquipmentComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& skillComp = registry.GetComponent<PlayerSkill>(player);
        auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);
        auto& loadout = registry.GetComponent<SpiritGemLoadoutComponent>(player);
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        Layout L = ComputeLayout();

        for (int i = 0; i < kSkillSlotCount; ++i) {
            sf::FloatRect rect({ kPanelX + 12.0f, L.rowY + static_cast<float>(i) * L.rowHeight }, { kPanelW - 24.0f, L.rowHeight });
            if (rect.contains(mouse)) { m_selectedSlot = i; return; }
        }
        for (int i = 0; i < kSpiritSlotCount; ++i) {
            sf::FloatRect rect({ kPanelX + 12.0f, L.spiritRowY + static_cast<float>(i) * L.rowHeight }, { kPanelW - 24.0f, L.rowHeight });
            if (rect.contains(mouse)) { m_selectedSlot = kSkillSlotCount + i; return; }
        }

        bool spiritSlot = IsSpiritSlot();
        std::vector<int> opts = Options(gemInventory, spiritSlot);
        for (size_t i = 0; i < opts.size(); ++i) {
            sf::FloatRect rect({ kPanelX + 12.0f, L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight }, { kPanelW - 24.0f, L.pickerRowHeight });
            if (rect.contains(mouse)) {
                AssignSelected(skillComp, loadout, equipment, stats, opts[i]);
                return;
            }
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, PlayerSkill, SkillGemInventoryComponent, SpiritGemLoadoutComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& skillComp = registry.GetComponent<PlayerSkill>(player);
        auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);
        auto& loadout = registry.GetComponent<SpiritGemLoadoutComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        float panelW = kPanelW;
        float panelH = kPanelH;
        float panelX = kPanelX;
        float panelY = kPanelY;
        float contentWidth = panelW - 32.0f;

        sf::RectangleShape bg({ panelW, panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        auto& binds = KeyBindings::Instance();
        std::string closeKey = KeyToString(binds.Get(GameAction::ToggleSkillGems));
        DrawText(target, panelX + 12.0f, panelY + 8.0f,
            Truncate("Skill Gems (" + closeKey + " to close) - click a slot, then click a gem", contentWidth, 13), 13, sf::Color(255, 220, 120));

        const std::string kSlotKeys[kSkillSlotCount] = {
            KeyToString(binds.Get(GameAction::Skill1)),
            KeyToString(binds.Get(GameAction::Skill2)),
            KeyToString(binds.Get(GameAction::Skill3)),
            KeyToString(binds.Get(GameAction::Skill4)),
            KeyToString(binds.Get(GameAction::Skill5)),
        };

        Layout L = ComputeLayout();

        for (int i = 0; i < kSkillSlotCount; ++i) {
            float y = L.rowY + static_cast<float>(i) * L.rowHeight;
            bool selected = (i == m_selectedSlot);

            sf::RectangleShape highlight({ panelW - 24.0f, L.rowHeight });
            highlight.setPosition({ panelX + 12.0f, y });
            highlight.setFillColor(selected ? sf::Color(60, 60, 90, 180) : sf::Color(35, 35, 40, 140));
            highlight.setOutlineColor(sf::Color(90, 90, 100));
            highlight.setOutlineThickness(1.0f);
            target.draw(highlight);

            const SkillData& skill = skillComp.skills[i];
            std::string line = "[" + kSlotKeys[i] + "] " + (skill.isValid ? skill.name : "-- Empty --");
            if (skill.isValid) {
                line += "  (cd " + FormatFloat(skill.cooldownTime) + "s, " + std::to_string(skill.mpCost) + " mp";
                if (DealsElementalDamage(skill.behaviorType)) line += ", " + ElementName(skill.element);
                line += ")";
            }

            DrawText(target, panelX + 16.0f, y + L.rowHeight / 2.0f - 7.0f, Truncate(line, contentWidth, 12), 12,
                skill.isValid ? sf::Color(220, 220, 220) : sf::Color(120, 120, 120));
        }

        // Spirit slots, directly below the 5 skill slots.
        DrawText(target, panelX + 12.0f, L.spiritLabelY,
            "Spirit: " + FormatFloat(stats.currentSpirit) + " / " + FormatFloat(stats.maxSpirit) + " reserved",
            12, sf::Color(160, 220, 255));

        for (int i = 0; i < kSpiritSlotCount; ++i) {
            int slotIndex = kSkillSlotCount + i;
            float y = L.spiritRowY + static_cast<float>(i) * L.rowHeight;
            bool selected = (slotIndex == m_selectedSlot);

            sf::RectangleShape highlight({ panelW - 24.0f, L.rowHeight });
            highlight.setPosition({ panelX + 12.0f, y });
            highlight.setFillColor(selected ? sf::Color(60, 90, 90, 180) : sf::Color(35, 40, 40, 140));
            highlight.setOutlineColor(sf::Color(90, 100, 100));
            highlight.setOutlineThickness(1.0f);
            target.draw(highlight);

            int gemId = loadout.auraGemIds[i];
            const GemDefinition* def = gemId >= 0 ? SkillGemData::Find(gemId) : nullptr;
            std::string line = "[Spirit " + std::to_string(i + 1) + "] " + (def ? def->skill.name : "-- Empty --");
            if (def) {
                line += "  (" + FormatFloat(def->skill.spiritCost) + " spirit)";
            }

            DrawText(target, panelX + 16.0f, y + L.rowHeight / 2.0f - 7.0f, Truncate(line, contentWidth, 12), 12,
                def ? sf::Color(180, 230, 230) : sf::Color(120, 120, 120));
        }

        // Gem picker: every unlocked gem matching the selected slot's category (plus
        // "Empty"), one clickable row each. Clicking a row sockets that gem (see Update).
        bool spiritSlot = IsSpiritSlot();
        std::vector<int> opts = Options(gemInventory, spiritSlot);
        int currentId = spiritSlot
            ? loadout.auraGemIds[m_selectedSlot - kSkillSlotCount]
            : (skillComp.skills[m_selectedSlot].isValid ? skillComp.skills[m_selectedSlot].gemId : -1);

        DrawText(target, panelX + 12.0f, L.pickerHeaderY,
            spiritSlot ? "Available Spirit gems (click to socket):" : "Available skill gems (click to socket):",
            13, sf::Color(200, 200, 200));

        for (size_t i = 0; i < opts.size(); ++i) {
            int gemId = opts[i];
            float y = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight;
            bool current = (gemId == currentId);

            sf::RectangleShape highlight({ panelW - 24.0f, L.pickerRowHeight });
            highlight.setPosition({ panelX + 12.0f, y });
            highlight.setFillColor(current ? sf::Color(70, 100, 70, 180) : sf::Color(30, 30, 34, 120));
            target.draw(highlight);

            std::string label;
            sf::Color color = sf::Color(200, 200, 200);
            if (gemId < 0) {
                label = "-- Empty --";
            } else {
                const GemDefinition* def = SkillGemData::Find(gemId);
                if (!def) continue;
                label = def->skill.name;
                if (DealsElementalDamage(def->skill.behaviorType)) {
                    label += " (" + ElementName(def->skill.element) + ")";
                }
                color = sf::Color(220, 220, 255);
            }
            DrawText(target, panelX + 16.0f, y + L.pickerRowHeight / 2.0f - 7.0f, Truncate(label, contentWidth, 12), 12, color);
        }

        float messageY = L.pickerRowY + static_cast<float>(opts.size()) * L.pickerRowHeight + 14.0f;
        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 12.0f, messageY, Truncate(lastActionMessage, contentWidth, 12), 12, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    // Shared row geometry, computed once so Update()'s click hit-testing and Render()'s
    // drawing can never drift apart.
    struct Layout {
        float rowY;
        float rowHeight;
        float spiritLabelY;
        float spiritRowY;
        float pickerHeaderY;
        float pickerRowY;
        float pickerRowHeight;
    };

    Layout ComputeLayout() const {
        Layout L;
        L.rowY = kPanelY + 40.0f;
        L.rowHeight = 26.0f;
        L.spiritLabelY = L.rowY + static_cast<float>(kSkillSlotCount) * L.rowHeight + 10.0f;
        L.spiritRowY = L.spiritLabelY + 18.0f;
        L.pickerHeaderY = L.spiritRowY + static_cast<float>(kSpiritSlotCount) * L.rowHeight + 20.0f;
        L.pickerRowY = L.pickerHeaderY + 22.0f;
        L.pickerRowHeight = 22.0f;
        return L;
    }

    // Shared with CharacterSheetSystem's identical constants: the two panels are
    // tab-switched (mutually exclusive, last one toggled wins) so they occupy the
    // same screen slot.
    static constexpr float kPanelW = 460.0f;
    static constexpr float kPanelH = 680.0f;
    static constexpr float kPanelX = 20.0f;
    static constexpr float kPanelY = 20.0f;

    bool IsSpiritSlot() const { return m_selectedSlot >= kSkillSlotCount; }

    // Option list for the selected slot: -1 = Empty, followed by every unlocked gem
    // matching the slot's category (activated skills for skill slots, Aura gems for
    // Spirit slots).
    std::vector<int> Options(const SkillGemInventoryComponent& gemInventory, bool auraOnly) const {
        std::vector<int> opts = { -1 };
        for (int id : gemInventory.unlockedGemIds) {
            const GemDefinition* def = SkillGemData::Find(id);
            if (!def) continue;
            bool isAura = (def->skill.behaviorType == SkillBehaviorType::Aura);
            if (isAura == auraOnly) opts.push_back(id);
        }
        return opts;
    }

    void AssignSelected(PlayerSkill& skillComp, SpiritGemLoadoutComponent& loadout,
        EquipmentComponent& equipment, CharacterStatsComponent& stats, int gemId) {
        if (IsSpiritSlot()) {
            std::string message;
            SpiritAuraSystem::TryAssign(loadout, m_selectedSlot - kSkillSlotCount, gemId, equipment, stats, message);
            lastActionMessage = message;
            messageTimer = 2.0f;
        } else {
            AssignGem(skillComp, gemId);
        }
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

    // True for behavior types that actually deal elemental damage -- Dash/Buff/Aura
    // leave `element` at its unused default (Physical), so showing it there would be
    // misleading (e.g. a Determination aura is not a "Physical" skill).
    bool DealsElementalDamage(SkillBehaviorType type) const {
        switch (type) {
        case SkillBehaviorType::Melee:
        case SkillBehaviorType::Projectile:
        case SkillBehaviorType::AreaEffect:
        case SkillBehaviorType::Spark:
        case SkillBehaviorType::GroundSlam:
        case SkillBehaviorType::LightningWarp:
        case SkillBehaviorType::LightningBall:
            return true;
        default:
            return false;
        }
    }

    std::string ElementName(DamageElement element) const {
        switch (element) {
        case DamageElement::Physical: return "Physical";
        case DamageElement::Fire: return "Fire";
        case DamageElement::Cold: return "Cold";
        case DamageElement::Lightning: return "Lightning";
        case DamageElement::Chaos: return "Chaos";
        default: return "";
        }
    }

    // Clips str to fit within maxWidth pixels at the given font size, appending "...".
    // Skill/gem names can be long, so line width can't be bounded just by tuning layout constants.
    std::string Truncate(const std::string& str, float maxWidth, unsigned int size) const {
        sf::Text probe(*m_font, sf::String::fromUtf8(str.begin(), str.end()), size);
        if (probe.getLocalBounds().size.x <= maxWidth) return str;

        std::string result = str;
        while (!result.empty()) {
            result.pop_back();
            std::string candidate = result + "...";
            sf::Text probe2(*m_font, sf::String::fromUtf8(candidate.begin(), candidate.end()), size);
            if (probe2.getLocalBounds().size.x <= maxWidth) return candidate;
        }
        return "...";
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
