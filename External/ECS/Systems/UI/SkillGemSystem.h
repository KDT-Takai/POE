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
#include "../Skill/SupportGemData.h"
#include "../Skill/SupportGemSystem.h"
#include "../Skill/SkillGemScaling.h"
#include "../Skill/SkillTags.h"
#include "../Skill/SpiritAuraSystem.h"
#include "GemIdentifySystem.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"

// Lets the player freely reassign any of the 5 skill slots (keybound Skill1-5) or 5
// Spirit slots (no keybind, always-on like a passive node, budget-limited by
// CharacterStatsComponent::maxSpirit) to any gem they own (SkillGemInventoryComponent).
// The 10 slots are the shared equip pool the gem system's design calls for: skill and
// Spirit gems can't duplicate within their own category, and each draws from the same
// underlying "owned gems" list (SkillGemInventoryComponent::ownedGems).
// Skill gems additionally have 2-5 support-gem sockets (Jeweller's Orb, spent from this
// screen, expands them); Spirit/Aura gems don't, since their effect is a flat stat bonus
// with nothing for a support gem's damage/cooldown/mana modifiers to act on.
//
// Visual language deliberately leans on PoE2's own conventions ("スキルジェムのUIを
// POE2みたいに" -- colored gem icons instead of plain text rows, a card-style picker
// instead of a thin text list) despite having no hand-drawn gem art: each gem's icon is
// a plain circle tinted by its GemAttribute (Str/Dex/Int, same red/green/blue coding as
// CharacterSheetSystem), and support sockets are tinted by SupportCategory. The picker
// list is also now a clipped, mouse-wheel-scrollable viewport (see m_pickerScroll) --
// previously it just kept drawing rows past the bottom of the panel with no way to reach
// them once there were more than ~12 options, which was the main "追加しにくい" (hard to
// add gems) complaint for the skill-gem list (17 entries).
class SkillGemSystem {
private:
    static constexpr int kSkillSlotCount = 5;
    static constexpr int kSpiritSlotCount = 5;
    static constexpr int kMaxPossibleSockets = 5;
    static constexpr float kIconRadius = 15.0f;
    static constexpr float kSocketBoxW = 88.0f;
    static constexpr float kSocketBoxH = 18.0f;
    static constexpr float kSocketGap = 5.0f;
    static constexpr float kToggleBtnW = 48.0f;
    static constexpr float kToggleBtnH = 20.0f;
    static constexpr float kScrollBarW = 6.0f;

    // Shared row geometry, computed once so Update()'s click hit-testing and Render()'s
    // drawing can never drift apart. Declared this early (not just before ComputeLayout)
    // because it's also used as a parameter type by RenderGemPicker/RenderSupportPicker,
    // and a member function's parameter types (unlike its body) aren't deferred to
    // "complete-class context" -- the type must already be visible at that point.
    struct Layout {
        float uncutGemsRowY;
        float skillRowY;
        float skillRowHeight;  // full row incl. the socket sub-line below the icon/name line
        float skillMainLineH;  // click area for just the icon/name line
        float socketLineOffsetY; // offset within a skill row where the socket line starts
        float spiritLabelY;
        float spiritRowY;
        float spiritRowHeight;
        float pickerHeaderY;
        float pickerRowY;
        float pickerRowHeight;
        float pickerViewportH; // clipped/scrollable picker area height
    };

    std::shared_ptr<sf::Font> m_font;
    int m_selectedSlot = 0;
    // -1 = the bottom picker lists skill/Spirit gems for m_selectedSlot (normal mode).
    // 0-4 = the bottom picker instead lists owned support gems for that socket index of
    // the skill currently equipped in m_selectedSlot (only meaningful when m_selectedSlot
    // is a skill slot).
    int m_selectedSocket = -1;
    float m_pickerScroll = 0.0f;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    SkillGemSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() { isOpen = !isOpen; m_pickerScroll = 0.0f; }
    void Close() { isOpen = false; }

    // Used by GameScene to resolve click ownership between the non-pausing menus:
    // only steal the click if the cursor is actually over this panel's rect.
    bool IsPointInPanel(sf::Vector2f point) const {
        if (!isOpen) return false;
        return sf::FloatRect({ kPanelX, kPanelY }, { kPanelW, kPanelH }).contains(point);
    }

    // Selection and assignment are entirely mouse-driven: click a slot row to select it,
    // then click a gem in the (scrollable) list below to socket it. The old Up/Down/
    // Left/Right keyboard navigation was removed because this menu doesn't pause the
    // world, so those arrow keys were simultaneously moving the player (see InputSystem's
    // movement handling).
    void Update(Registry& registry, float dt, GemIdentifySystem& gemIdentifySystem) {
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

        Layout L = ComputeLayout();
        sf::Vector2f mouseAny = InputManager::Instance().GetMouseInput().GetMousePointF();

        // Mouse-wheel scroll over the picker viewport -- independent of the left-click
        // gate below, since scrolling shouldn't require holding/clicking anything.
        float wheel = InputManager::Instance().GetMouseWheelDelta();
        if (wheel != 0.0f) {
            sf::FloatRect pickerClip({ kPanelX, L.pickerRowY }, { kPanelW, L.pickerViewportH });
            if (pickerClip.contains(mouseAny)) {
                int optCount = (m_selectedSocket >= 0)
                    ? static_cast<int>(SupportOptions(HostSkillTags(skillComp), gemInventory).size())
                    : static_cast<int>(Options(gemInventory, IsSpiritSlot()).size());
                float contentH = static_cast<float>(optCount) * L.pickerRowHeight;
                float maxScroll = (std::max)(0.0f, contentH - L.pickerViewportH);
                m_pickerScroll = std::clamp(m_pickerScroll - wheel * L.pickerRowHeight, 0.0f, maxScroll);
            }
        }

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        for (size_t i = 0; i < gemInventory.pendingUncutGems.size() && i < SkillGemInventoryComponent::kPendingCapacity; ++i) {
            if (UncutGemRect(L, static_cast<int>(i)).contains(mouse)) {
                const PendingUncutGem& pending = gemInventory.pendingUncutGems[i];
                gemIdentifySystem.Open(static_cast<int>(i), pending.level, pending.kind);
                return;
            }
        }

        for (int i = 0; i < kSkillSlotCount; ++i) {
            sf::FloatRect mainRect({ kPanelX + 12.0f, SkillRowY(L, i) }, { kPanelW - 24.0f, L.skillMainLineH });
            if (mainRect.contains(mouse)) { SelectSlot(i, -1); return; }

            if (skillComp.skills[i].isValid) {
                OwnedGemInstance* inst = FindOwnedMutable(gemInventory, skillComp.skills[i].gemId, false);
                if (inst) {
                    for (int s = 0; s < inst->maxSockets && s < kMaxPossibleSockets; ++s) {
                        if (SocketRect(L, i, s).contains(mouse)) { SelectSlot(i, s); return; }
                    }
                    if (inst->maxSockets < kMaxPossibleSockets && stats.jewellersOrbs > 0
                        && SocketRect(L, i, inst->maxSockets).contains(mouse)) {
                        inst->maxSockets++;
                        stats.jewellersOrbs--;
                        lastActionMessage = "Socket added (" + std::to_string(inst->maxSockets) + "/5)";
                        messageTimer = 2.0f;
                        return;
                    }
                }
            }
        }

        for (int i = 0; i < kSpiritSlotCount; ++i) {
            if (loadout.auraGemIds[i] >= 0 && SpiritToggleRect(L, i).contains(mouse)) {
                std::string message;
                SpiritAuraSystem::TryToggleActive(registry, player, loadout, i, gemInventory, equipment, stats, message);
                lastActionMessage = message;
                messageTimer = 2.0f;
                return;
            }
            sf::FloatRect rect({ kPanelX + 12.0f, SpiritRowY(L, i) }, { kPanelW - 24.0f, L.spiritRowHeight });
            if (rect.contains(mouse)) { SelectSlot(kSkillSlotCount + i, -1); return; }
        }

        sf::FloatRect pickerClip({ kPanelX, L.pickerRowY }, { kPanelW, L.pickerViewportH });
        if (!pickerClip.contains(mouse)) return;

        if (m_selectedSocket >= 0) {
            std::vector<int> opts = SupportOptions(HostSkillTags(skillComp), gemInventory);
            for (size_t i = 0; i < opts.size(); ++i) {
                float rowY = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight - m_pickerScroll;
                sf::FloatRect rect({ kPanelX + 12.0f, rowY }, { kPanelW - 24.0f, L.pickerRowHeight });
                if (rect.contains(mouse)) {
                    AssignSupport(skillComp, gemInventory, stats, opts[i]);
                    return;
                }
            }
        } else {
            bool spiritSlot = IsSpiritSlot();
            std::vector<int> opts = Options(gemInventory, spiritSlot);
            for (size_t i = 0; i < opts.size(); ++i) {
                float rowY = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight - m_pickerScroll;
                sf::FloatRect rect({ kPanelX + 12.0f, rowY }, { kPanelW - 24.0f, L.pickerRowHeight });
                if (rect.contains(mouse)) {
                    AssignSelected(registry, player, skillComp, loadout, gemInventory, equipment, stats, opts[i]);
                    return;
                }
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
        DrawText(target, panelX + 12.0f, panelY + 6.0f,
            Truncate("Skill Gems (" + closeKey + " to close) - click a slot, then a gem", contentWidth, 12), 12, sf::Color(255, 220, 120));

        const std::string kSlotKeys[kSkillSlotCount] = {
            KeyToString(binds.Get(GameAction::Skill1)),
            KeyToString(binds.Get(GameAction::Skill2)),
            KeyToString(binds.Get(GameAction::Skill3)),
            KeyToString(binds.Get(GameAction::Skill4)),
            KeyToString(binds.Get(GameAction::Skill5)),
        };

        Layout L = ComputeLayout();

        for (size_t i = 0; i < gemInventory.pendingUncutGems.size() && i < SkillGemInventoryComponent::kPendingCapacity; ++i) {
            const PendingUncutGem& pending = gemInventory.pendingUncutGems[i];
            sf::FloatRect rect = UncutGemRect(L, static_cast<int>(i));
            DrawButtonSmall(target, rect, KindAbbrev(pending.kind) + std::to_string(pending.level), UncutKindColor(pending.kind));
        }
        if (gemInventory.pendingUncutGems.empty()) {
            DrawText(target, panelX + 12.0f, L.uncutGemsRowY + 2.0f, "Uncut Gems: none", 11, sf::Color(120, 120, 120));
        }

        for (int i = 0; i < kSkillSlotCount; ++i) {
            float y = SkillRowY(L, i);
            bool selected = (i == m_selectedSlot);

            sf::RectangleShape highlight({ panelW - 24.0f, L.skillRowHeight - 2.0f });
            highlight.setPosition({ panelX + 12.0f, y });
            highlight.setFillColor(selected ? sf::Color(60, 60, 90, 180) : sf::Color(35, 35, 40, 140));
            highlight.setOutlineColor(selected ? sf::Color(140, 140, 220) : sf::Color(90, 90, 100));
            highlight.setOutlineThickness(selected ? 2.0f : 1.0f);
            target.draw(highlight);

            const SkillData& skill = skillComp.skills[i];
            const GemDefinition* def = skill.isValid ? SkillGemData::Find(skill.gemId) : nullptr;
            sf::Vector2f iconCenter(panelX + 12.0f + 6.0f + kIconRadius, y + L.skillMainLineH / 2.0f);
            DrawGemIcon(target, iconCenter, kIconRadius, def ? AttributeColor(def->primaryAttribute) : sf::Color(40, 40, 45),
                skill.isValid, kSlotKeys[i]);

            float textX = panelX + 12.0f + 6.0f + kIconRadius * 2.0f + 10.0f;
            float textContentW = contentWidth - (textX - panelX);
            std::string line = skill.isValid ? skill.name : "-- Empty --";
            if (skill.isValid) {
                line += " Lv" + std::to_string(skill.level);
            }
            DrawText(target, textX, y + 2.0f, Truncate(line, textContentW, 13), 13,
                skill.isValid ? sf::Color(230, 230, 230) : sf::Color(120, 120, 120));

            if (skill.isValid) {
                std::string statLine = "cd " + FormatFloat(skill.cooldownTime) + "s, " + std::to_string(skill.mpCost) + " mp";
                if (DealsElementalDamage(skill.behaviorType)) statLine += ", " + ElementName(skill.element);
                DrawText(target, textX, y + 18.0f, Truncate(statLine, textContentW, 11), 11, sf::Color(170, 170, 175));
            }

            if (skill.isValid) {
                OwnedGemInstance* inst = FindOwnedMutable(gemInventory, skill.gemId, false);
                if (inst) {
                    for (int s = 0; s < inst->maxSockets && s < kMaxPossibleSockets; ++s) {
                        bool socketSelected = selected && (s == m_selectedSocket);
                        DrawSocketBox(target, SocketRect(L, i, s), inst->supportGemIds[s], socketSelected);
                    }
                    if (inst->maxSockets < kMaxPossibleSockets) {
                        bool canAfford = stats.jewellersOrbs > 0;
                        sf::FloatRect btn = SocketRect(L, i, inst->maxSockets);
                        DrawButtonSmall(target, btn, "+Jeweller(" + std::to_string(stats.jewellersOrbs) + ")",
                            canAfford ? sf::Color(70, 90, 70) : sf::Color(50, 50, 55));
                    }
                }
            }
        }

        DrawText(target, panelX + 12.0f, L.spiritLabelY,
            "Spirit: " + FormatFloat(stats.currentSpirit) + " / " + FormatFloat(stats.maxSpirit) + " reserved",
            12, sf::Color(160, 220, 255));

        for (int i = 0; i < kSpiritSlotCount; ++i) {
            int slotIndex = kSkillSlotCount + i;
            float y = SpiritRowY(L, i);
            bool selected = (slotIndex == m_selectedSlot);

            sf::RectangleShape highlight({ panelW - 24.0f, L.spiritRowHeight - 2.0f });
            highlight.setPosition({ panelX + 12.0f, y });
            highlight.setFillColor(selected ? sf::Color(60, 90, 90, 180) : sf::Color(35, 40, 40, 140));
            highlight.setOutlineColor(selected ? sf::Color(120, 200, 200) : sf::Color(90, 100, 100));
            highlight.setOutlineThickness(selected ? 2.0f : 1.0f);
            target.draw(highlight);

            int gemId = loadout.auraGemIds[i];
            const GemDefinition* def = gemId >= 0 ? SkillGemData::Find(gemId) : nullptr;
            bool active = def && loadout.active[i];

            sf::Vector2f iconCenter(panelX + 12.0f + 6.0f + kIconRadius, y + L.spiritRowHeight / 2.0f);
            DrawGemIcon(target, iconCenter, kIconRadius, def ? AttributeColor(def->primaryAttribute) : sf::Color(40, 40, 45),
                def != nullptr, "");
            if (active) {
                sf::CircleShape activeRing(kIconRadius + 3.0f);
                activeRing.setOrigin({ kIconRadius + 3.0f, kIconRadius + 3.0f });
                activeRing.setPosition(iconCenter);
                activeRing.setFillColor(sf::Color::Transparent);
                activeRing.setOutlineColor(sf::Color(120, 255, 160));
                activeRing.setOutlineThickness(2.0f);
                target.draw(activeRing);
            }

            float textX = panelX + 12.0f + 6.0f + kIconRadius * 2.0f + 10.0f;
            float textContentW = contentWidth - (textX - panelX) - kToggleBtnW - 8.0f;
            std::string line = def ? def->skill.name : "-- Empty --";
            if (def) {
                int level = SpiritAuraSystem::LevelOf(gemInventory, gemId);
                line += " Lv" + std::to_string(level) + "  (" + FormatFloat(SpiritAuraSystem::SpiritCostOf(gemId, gemInventory)) + " spirit)";
            }
            DrawText(target, textX, y + L.spiritRowHeight / 2.0f - 7.0f, Truncate(line, textContentW, 12), 12,
                def ? sf::Color(180, 230, 230) : sf::Color(120, 120, 120));

            if (def) {
                sf::FloatRect toggleRect = SpiritToggleRect(L, i);
                DrawButtonSmall(target, toggleRect, active ? "ON" : "OFF",
                    active ? sf::Color(60, 130, 70) : sf::Color(70, 60, 60));
            }
        }

        // Picker viewport: clipped to its own rect (via a scoped view, same technique as
        // PassiveTreeSystem/AtlasSystem's tree/node-graph area) so a long option list
        // scrolls within a fixed box instead of spilling past the panel with no way to
        // reach the lower entries.
        std::vector<int> pickerOpts = (m_selectedSocket >= 0)
            ? SupportOptions(HostSkillTags(skillComp), gemInventory)
            : Options(gemInventory, IsSpiritSlot());

        DrawText(target, panelX + 12.0f, L.pickerHeaderY,
            (m_selectedSocket >= 0)
                ? "Support gems for socket " + std::to_string(m_selectedSocket + 1) + " (click to socket):"
                : (IsSpiritSlot() ? "Available Spirit gems (click to socket):" : "Available skill gems (click to socket):"),
            13, sf::Color(200, 200, 200));

        sf::Vector2u winSize = target.getSize();
        sf::View pickerView(sf::FloatRect({ panelX, L.pickerRowY }, { panelW, L.pickerViewportH }));
        pickerView.setViewport(sf::FloatRect(
            { panelX / static_cast<float>(winSize.x), L.pickerRowY / static_cast<float>(winSize.y) },
            { panelW / static_cast<float>(winSize.x), L.pickerViewportH / static_cast<float>(winSize.y) }));
        target.setView(pickerView);

        if (m_selectedSocket >= 0) {
            RenderSupportPicker(target, L, panelX, contentWidth, pickerOpts, skillComp, gemInventory, stats);
        } else {
            RenderGemPicker(target, L, panelX, contentWidth, pickerOpts, IsSpiritSlot(), skillComp, loadout, gemInventory, stats);
        }

        target.setView(target.getDefaultView());

        // Scrollbar affordance -- only shown once content actually overflows the
        // viewport, so it doesn't clutter short lists (Spirit gems, support options).
        float contentH = static_cast<float>(pickerOpts.size()) * L.pickerRowHeight;
        if (contentH > L.pickerViewportH) {
            sf::FloatRect track({ panelX + panelW - kScrollBarW - 4.0f, L.pickerRowY }, { kScrollBarW, L.pickerViewportH });
            sf::RectangleShape trackShape(track.size);
            trackShape.setPosition(track.position);
            trackShape.setFillColor(sf::Color(40, 40, 45));
            target.draw(trackShape);

            float thumbH = (std::max)(16.0f, L.pickerViewportH * (L.pickerViewportH / contentH));
            float maxScroll = contentH - L.pickerViewportH;
            float thumbY = track.position.y + (maxScroll > 0.0f ? (m_pickerScroll / maxScroll) * (L.pickerViewportH - thumbH) : 0.0f);
            sf::RectangleShape thumb({ kScrollBarW, thumbH });
            thumb.setPosition({ track.position.x, thumbY });
            thumb.setFillColor(sf::Color(130, 130, 150));
            target.draw(thumb);
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 12.0f, L.pickerRowY + L.pickerViewportH + 8.0f,
                Truncate(lastActionMessage, contentWidth, 12), 12, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    void SelectSlot(int slot, int socket) {
        m_selectedSlot = slot;
        m_selectedSocket = socket;
        m_pickerScroll = 0.0f;
    }

    void RenderGemPicker(sf::RenderTarget& target, const Layout& L, float panelX, float contentWidth,
        const std::vector<int>& opts, bool spiritSlot, const PlayerSkill& skillComp,
        const SpiritGemLoadoutComponent& loadout, const SkillGemInventoryComponent& gemInventory,
        const CharacterStatsComponent& stats) {
        int currentId = spiritSlot
            ? loadout.auraGemIds[m_selectedSlot - kSkillSlotCount]
            : (skillComp.skills[m_selectedSlot].isValid ? skillComp.skills[m_selectedSlot].gemId : -1);

        for (size_t i = 0; i < opts.size(); ++i) {
            int gemId = opts[i];
            float y = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight - m_pickerScroll;
            bool current = (gemId == currentId);

            std::string name = "-- Empty --";
            std::string reqLine;
            sf::Color iconColor(60, 60, 65);
            sf::Color statusColor(200, 220, 255);
            bool hasIcon = false;

            if (gemId >= 0) {
                const GemDefinition* def = SkillGemData::Find(gemId);
                if (!def) continue;
                hasIcon = true;
                iconColor = AttributeColor(def->primaryAttribute);
                int level = SpiritAuraSystem::LevelOf(gemInventory, gemId);
                name = def->skill.name + " Lv" + std::to_string(level);
                if (DealsElementalDamage(def->skill.behaviorType)) name += " (" + ElementName(def->skill.element) + ")";

                int required = SkillGemScaling::RequiredStat(def->baseRequirement, level);
                int have = SpiritAuraSystem::StatValue(stats, def->primaryAttribute);
                reqLine = "Requires " + std::to_string(required) + " " + SpiritAuraSystem::AttributeName(def->primaryAttribute)
                    + " (have " + std::to_string(have) + ")";
                statusColor = (have < required) ? sf::Color(230, 100, 100) : sf::Color(150, 220, 150);
            }

            DrawPickerCard(target, panelX, contentWidth, y, L.pickerRowHeight, name, reqLine, iconColor, hasIcon, current, statusColor);
        }
    }

    void RenderSupportPicker(sf::RenderTarget& target, const Layout& L, float panelX, float contentWidth,
        const std::vector<int>& opts, const PlayerSkill& skillComp, const SkillGemInventoryComponent& gemInventory,
        const CharacterStatsComponent& stats) {
        int hostGemId = (m_selectedSlot < kSkillSlotCount && skillComp.skills[m_selectedSlot].isValid)
            ? skillComp.skills[m_selectedSlot].gemId : -1;
        const OwnedGemInstance* inst = hostGemId >= 0 ? FindOwnedConst(gemInventory, hostGemId, false) : nullptr;
        int currentId = (inst && m_selectedSocket >= 0 && m_selectedSocket < kMaxPossibleSockets)
            ? inst->supportGemIds[m_selectedSocket] : -1;

        for (size_t i = 0; i < opts.size(); ++i) {
            int gemId = opts[i];
            float y = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight - m_pickerScroll;
            bool current = (gemId == currentId);

            std::string name = "-- Empty --";
            std::string reqLine;
            sf::Color iconColor(60, 60, 65);
            sf::Color statusColor(200, 220, 255);
            bool hasIcon = false;

            if (gemId >= 0) {
                const SupportGemDefinition* def = SupportGemData::Find(gemId);
                if (!def) continue;
                hasIcon = true;
                iconColor = CategoryColor(def->category);
                name = def->name;
                int have = SpiritAuraSystem::StatValue(stats, def->primaryAttribute);
                bool categoryConflict = inst && HasCategoryConflict(*inst, m_selectedSocket, *def);
                reqLine = "Requires " + std::to_string(def->requirement) + " " + SpiritAuraSystem::AttributeName(def->primaryAttribute)
                    + " (have " + std::to_string(have) + ")";
                if (categoryConflict) reqLine += " - category conflict";
                statusColor = (have < def->requirement || categoryConflict) ? sf::Color(230, 100, 100) : sf::Color(150, 220, 150);
            }

            DrawPickerCard(target, panelX, contentWidth, y, L.pickerRowHeight, name, reqLine, iconColor, hasIcon, current, statusColor);
        }
    }

    // One picker option, PoE2-style: a colored gem icon + two-line text block (name on
    // top, requirement/status below) inside a bordered card, instead of the old single
    // line of dense text. `current` highlights whatever's already socketed there.
    void DrawPickerCard(sf::RenderTarget& target, float panelX, float contentWidth, float y, float rowHeight,
        const std::string& name, const std::string& reqLine, sf::Color iconColor, bool hasIcon, bool current, sf::Color statusColor) {
        sf::RectangleShape card({ kPanelW - 24.0f, rowHeight - 4.0f });
        card.setPosition({ panelX + 12.0f, y });
        card.setFillColor(current ? sf::Color(55, 80, 60, 200) : sf::Color(28, 28, 33, 190));
        card.setOutlineColor(current ? sf::Color(140, 220, 150) : sf::Color(80, 80, 90));
        card.setOutlineThickness(current ? 2.0f : 1.0f);
        target.draw(card);

        float iconR = kIconRadius * 0.8f;
        sf::Vector2f iconCenter(panelX + 12.0f + 10.0f + iconR, y + (rowHeight - 4.0f) / 2.0f);
        DrawGemIcon(target, iconCenter, iconR, iconColor, hasIcon, "");

        float textX = panelX + 12.0f + 10.0f + iconR * 2.0f + 10.0f;
        float textW = contentWidth - (textX - panelX) - 8.0f;
        DrawText(target, textX, y + 6.0f, Truncate(name, textW, 13), 13, sf::Color(225, 225, 230));
        if (!reqLine.empty()) {
            DrawText(target, textX, y + rowHeight - 20.0f, Truncate(reqLine, textW, 11), 11, statusColor);
        }
    }

    Layout ComputeLayout() const {
        Layout L;
        L.uncutGemsRowY = kPanelY + 22.0f;
        L.skillRowY = L.uncutGemsRowY + 26.0f;
        L.skillRowHeight = 54.0f;
        L.skillMainLineH = 34.0f;
        L.socketLineOffsetY = 34.0f;
        L.spiritLabelY = L.skillRowY + static_cast<float>(kSkillSlotCount) * L.skillRowHeight + 8.0f;
        L.spiritRowY = L.spiritLabelY + 18.0f;
        L.spiritRowHeight = 36.0f;
        L.pickerHeaderY = L.spiritRowY + static_cast<float>(kSpiritSlotCount) * L.spiritRowHeight + 16.0f;
        L.pickerRowY = L.pickerHeaderY + 22.0f;
        L.pickerRowHeight = 46.0f;
        L.pickerViewportH = (std::max)(60.0f, (kPanelY + kPanelH) - L.pickerRowY - 34.0f);
        return L;
    }

    static constexpr float kUncutChipW = 48.0f;
    static constexpr float kUncutChipH = 18.0f;
    static constexpr float kUncutChipGap = 3.0f;

    sf::FloatRect UncutGemRect(const Layout& L, int index) const {
        float x = kPanelX + 12.0f + static_cast<float>(index) * (kUncutChipW + kUncutChipGap);
        return sf::FloatRect({ x, L.uncutGemsRowY }, { kUncutChipW, kUncutChipH });
    }

    float SkillRowY(const Layout& L, int index) const { return L.skillRowY + static_cast<float>(index) * L.skillRowHeight; }
    float SpiritRowY(const Layout& L, int index) const { return L.spiritRowY + static_cast<float>(index) * L.spiritRowHeight; }

    sf::FloatRect SocketRect(const Layout& L, int rowIndex, int socketIndex) const {
        float x = kPanelX + 16.0f + static_cast<float>(socketIndex) * (kSocketBoxW + kSocketGap);
        float y = SkillRowY(L, rowIndex) + L.socketLineOffsetY;
        return sf::FloatRect({ x, y }, { kSocketBoxW, kSocketBoxH });
    }

    // ON/OFF button pinned to the right edge of a Spirit row (registering a gem there
    // does NOT activate it -- this is the separate toggle, see SpiritAuraSystem).
    sf::FloatRect SpiritToggleRect(const Layout& L, int rowIndex) const {
        float x = kPanelX + (kPanelW - 24.0f) + 12.0f - kToggleBtnW - 4.0f;
        float y = SpiritRowY(L, rowIndex) + (L.spiritRowHeight - kToggleBtnH) / 2.0f;
        return sf::FloatRect({ x, y }, { kToggleBtnW, kToggleBtnH });
    }

    static constexpr float kPanelW = 520.0f;
    static constexpr float kPanelH = 720.0f;
    static constexpr float kPanelX = 20.0f;
    static constexpr float kPanelY = 20.0f;

    bool IsSpiritSlot() const { return m_selectedSlot >= kSkillSlotCount; }

    const OwnedGemInstance* FindOwnedConst(const SkillGemInventoryComponent& inv, int gemId, bool isSupport) const {
        return SkillGemScaling::FindOwnedGem(inv, gemId, isSupport);
    }

    // True if `candidate` shares a SupportCategory with any OTHER socketed support on
    // `inst` (excludeSocket is the socket being written to, not a conflict with itself).
    static bool HasCategoryConflict(const OwnedGemInstance& inst, int excludeSocket, const SupportGemDefinition& candidate) {
        for (int s = 0; s < kMaxPossibleSockets; ++s) {
            if (s == excludeSocket || inst.supportGemIds[s] < 0) continue;
            const SupportGemDefinition* other = SupportGemData::Find(inst.supportGemIds[s]);
            if (other && other->category == candidate.category) return true;
        }
        return false;
    }

    OwnedGemInstance* FindOwnedMutable(SkillGemInventoryComponent& inv, int gemId, bool isSupport) const {
        for (auto& owned : inv.ownedGems) {
            if (owned.isSupport == isSupport && owned.gemId == gemId) return &owned;
        }
        return nullptr;
    }

    // Option list for the selected slot: -1 = Empty, followed by every owned gem
    // matching the slot's category (activated skills for skill slots, Aura gems for
    // Spirit slots).
    std::vector<int> Options(const SkillGemInventoryComponent& gemInventory, bool auraOnly) const {
        std::vector<int> opts = { -1 };
        for (const auto& owned : gemInventory.ownedGems) {
            if (owned.isSupport) continue;
            const GemDefinition* def = SkillGemData::Find(owned.gemId);
            if (!def) continue;
            bool isSpirit = SkillGemData::IsSpiritBehavior(def->skill.behaviorType);
            if (isSpirit == auraOnly) opts.push_back(owned.gemId);
        }
        return opts;
    }

    // Support gems are Uncut-Gem drops like Skill/Spirit gems (see GemIdentifySystem) --
    // only ones the player has already cut are offered here, gated by whether the host
    // skill has the support's required SkillTag (spec: only show compatible supports).
    std::vector<int> SupportOptions(unsigned int hostSkillTags, const SkillGemInventoryComponent& gemInventory) const {
        std::vector<int> opts = { -1 };
        for (const auto& owned : gemInventory.ownedGems) {
            if (!owned.isSupport) continue;
            const SupportGemDefinition* def = SupportGemData::Find(owned.gemId);
            if (def && SkillTags::IsCompatible(hostSkillTags, def->requiredTag)) opts.push_back(def->id);
        }
        return opts;
    }

    static std::string KindAbbrev(GemPickupKind kind) {
        switch (kind) {
        case GemPickupKind::Support: return "Su";
        case GemPickupKind::Spirit: return "Sp";
        default: return "Sk";
        }
    }

    // Uncut Gem chips get a color per kind (Skill/Support/Spirit) so they read as three
    // visually distinct groups at a glance instead of a uniform-colored row.
    static sf::Color UncutKindColor(GemPickupKind kind) {
        switch (kind) {
        case GemPickupKind::Support: return sf::Color(100, 70, 110);
        case GemPickupKind::Spirit: return sf::Color(60, 100, 100);
        default: return sf::Color(120, 85, 50);
        }
    }

    // Same red/green/blue coding CharacterSheetSystem already uses for STR/DEX/INT, reused
    // here so a gem's primary attribute reads consistently across both panels.
    static sf::Color AttributeColor(GemAttribute attr) {
        switch (attr) {
        case GemAttribute::Str: return sf::Color(220, 90, 90);
        case GemAttribute::Dex: return sf::Color(120, 200, 90);
        case GemAttribute::Int: return sf::Color(100, 160, 230);
        default: return sf::Color(160, 160, 160);
        }
    }

    static sf::Color CategoryColor(SupportCategory category) {
        switch (category) {
        case SupportCategory::DamageMult: return sf::Color(210, 120, 60);
        case SupportCategory::Speed: return sf::Color(120, 190, 120);
        case SupportCategory::Utility: return sf::Color(110, 150, 210);
        case SupportCategory::AreaMod: return sf::Color(170, 110, 200);
        case SupportCategory::Duration: return sf::Color(90, 170, 170);
        default: return sf::Color(150, 150, 150);
        }
    }

    unsigned int HostSkillTags(const PlayerSkill& skillComp) const {
        if (m_selectedSlot >= kSkillSlotCount || !skillComp.skills[m_selectedSlot].isValid) return 0;
        const SkillData& skill = skillComp.skills[m_selectedSlot];
        return SkillTags::TagsFor(skill.behaviorType, skill.element);
    }

    void AssignSelected(Registry& registry, Entity player, PlayerSkill& skillComp, SpiritGemLoadoutComponent& loadout, SkillGemInventoryComponent& gemInventory,
        EquipmentComponent& equipment, CharacterStatsComponent& stats, int gemId) {
        if (IsSpiritSlot()) {
            std::string message;
            // Only registers the gem into the slot -- it stays OFF (no Spirit reserved,
            // no effect applied) until separately toggled ON via SpiritToggleRect.
            SpiritAuraSystem::TryRegister(registry, player, loadout, m_selectedSlot - kSkillSlotCount, gemId, gemInventory, equipment, stats, message);
            lastActionMessage = message;
            messageTimer = 2.0f;
        } else {
            AssignGem(skillComp, gemInventory, equipment, stats, gemId);
        }
    }

    void AssignGem(PlayerSkill& skillComp, const SkillGemInventoryComponent& gemInventory,
        const EquipmentComponent& equipment, CharacterStatsComponent& stats, int gemId) {
        if (gemId < 0) {
            skillComp.skills[m_selectedSlot] = SkillData{};
            lastActionMessage = "Slot " + std::to_string(m_selectedSlot + 1) + ": Empty";
            messageTimer = 2.0f;
            return;
        }

        for (int i = 0; i < kSkillSlotCount; ++i) {
            if (i != m_selectedSlot && skillComp.skills[i].isValid && skillComp.skills[i].gemId == gemId) {
                lastActionMessage = "Already equipped in another skill slot";
                messageTimer = 2.0f;
                return;
            }
        }

        const GemDefinition* def = SkillGemData::Find(gemId);
        if (!def) return;
        int level = SpiritAuraSystem::LevelOf(gemInventory, gemId);
        int required = SkillGemScaling::RequiredStat(def->baseRequirement, level);
        int have = SpiritAuraSystem::StatValue(stats, def->primaryAttribute);
        if (have < required) {
            lastActionMessage = "Requires " + std::to_string(required) + " " + SpiritAuraSystem::AttributeName(def->primaryAttribute)
                + " (have " + std::to_string(have) + ")";
            messageTimer = 2.0f;
            return;
        }

        unsigned int tags = SkillTags::TagsFor(def->skill.behaviorType, def->skill.element);
        bool needsWeapon = (tags & static_cast<unsigned int>(SkillTag::Attack)) != 0;
        bool hasWeapon = equipment.slots[static_cast<size_t>(EquipSlot::Weapon)].has_value();
        if (needsWeapon && !hasWeapon) {
            lastActionMessage = "Requires a weapon equipped";
            messageTimer = 2.0f;
            return;
        }

        BuildSkillData(skillComp.skills[m_selectedSlot], gemId, gemInventory);
        lastActionMessage = "Slot " + std::to_string(m_selectedSlot + 1) + ": " + skillComp.skills[m_selectedSlot].name;
        messageTimer = 2.0f;
    }

    void AssignSupport(PlayerSkill& skillComp, SkillGemInventoryComponent& gemInventory, CharacterStatsComponent& stats, int supportGemId) {
        if (m_selectedSlot >= kSkillSlotCount || !skillComp.skills[m_selectedSlot].isValid) return;
        int hostGemId = skillComp.skills[m_selectedSlot].gemId;
        OwnedGemInstance* inst = FindOwnedMutable(gemInventory, hostGemId, false);
        if (!inst || m_selectedSocket < 0 || m_selectedSocket >= inst->maxSockets) return;

        if (supportGemId >= 0) {
            const SupportGemDefinition* def = SupportGemData::Find(supportGemId);
            if (!def) return;

            unsigned int hostTags = SkillTags::TagsFor(skillComp.skills[m_selectedSlot].behaviorType, skillComp.skills[m_selectedSlot].element);
            if (!SkillTags::IsCompatible(hostTags, def->requiredTag)) {
                lastActionMessage = "Not compatible with this skill";
                messageTimer = 2.0f;
                return;
            }

            int have = SpiritAuraSystem::StatValue(stats, def->primaryAttribute);
            if (have < def->requirement) {
                lastActionMessage = "Requires " + std::to_string(def->requirement) + " " + SpiritAuraSystem::AttributeName(def->primaryAttribute)
                    + " (have " + std::to_string(have) + ")";
                messageTimer = 2.0f;
                return;
            }
            for (int s = 0; s < kMaxPossibleSockets; ++s) {
                if (s == m_selectedSocket || inst->supportGemIds[s] < 0) continue;
                if (inst->supportGemIds[s] == supportGemId) {
                    lastActionMessage = "Already socketed in this skill";
                    messageTimer = 2.0f;
                    return;
                }
            }
            if (HasCategoryConflict(*inst, m_selectedSocket, *def)) {
                lastActionMessage = "Support Category conflict";
                messageTimer = 2.0f;
                return;
            }
        }

        inst->supportGemIds[m_selectedSocket] = supportGemId;
        BuildSkillData(skillComp.skills[m_selectedSlot], hostGemId, gemInventory);

        const SupportGemDefinition* assigned = supportGemId >= 0 ? SupportGemData::Find(supportGemId) : nullptr;
        lastActionMessage = "Socket " + std::to_string(m_selectedSocket + 1) + ": " + (assigned ? assigned->name : "Empty");
        messageTimer = 2.0f;
    }

    // Called on initial assignment and again whenever a socket on this same gem changes
    // while it's equipped. See SkillGemScaling::BuildEquippedSkillData (shared with
    // GameScene's save-load restore path, so both derive identical live stats).
    void BuildSkillData(SkillData& out, int gemId, const SkillGemInventoryComponent& gemInventory) const {
        SkillGemScaling::BuildEquippedSkillData(out, gemId, gemInventory);
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

    // PoE2-style colored gem icon: a filled circle tinted by attribute/category (see
    // AttributeColor/CategoryColor) when occupied, a dim outlined circle when empty, with
    // an optional short badge (keybind letter) centered inside it.
    void DrawGemIcon(sf::RenderTarget& target, sf::Vector2f center, float radius, sf::Color color, bool filled, const std::string& badge) {
        sf::CircleShape circle(radius);
        circle.setOrigin({ radius, radius });
        circle.setPosition(center);
        circle.setFillColor(filled ? color : sf::Color(30, 30, 34));
        circle.setOutlineColor(filled ? sf::Color(240, 240, 245) : sf::Color(80, 80, 88));
        circle.setOutlineThickness(filled ? 1.5f : 1.0f);
        target.draw(circle);

        if (!badge.empty()) {
            sf::Text text(*m_font, sf::String::fromUtf8(badge.begin(), badge.end()), 10);
            text.setFillColor(sf::Color(20, 20, 20));
            sf::FloatRect b = text.getLocalBounds();
            text.setPosition({ center.x - b.size.x / 2.0f - b.position.x, center.y - b.size.y / 2.0f - b.position.y - 1.0f });
            target.draw(text);
        }
    }

    void DrawSocketBox(sf::RenderTarget& target, sf::FloatRect rect, int supportGemId, bool selected) {
        const SupportGemDefinition* def = supportGemId >= 0 ? SupportGemData::Find(supportGemId) : nullptr;
        sf::Color tint = def ? CategoryColor(def->category) : sf::Color(30, 30, 34);

        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        box.setFillColor(def ? sf::Color(tint.r / 3, tint.g / 3, tint.b / 3) : sf::Color(30, 30, 34));
        box.setOutlineColor(selected ? sf::Color::Yellow : (def ? tint : sf::Color(90, 90, 100)));
        box.setOutlineThickness(selected ? 2.0f : 1.0f);
        target.draw(box);

        std::string label = "+ Empty";
        if (def) label = def->name;
        DrawText(target, rect.position.x + 3.0f, rect.position.y + 1.0f, Truncate(label, rect.size.x - 6.0f, 9), 9,
            def ? sf::Color(230, 230, 240) : sf::Color(140, 140, 140));
    }

    void DrawButtonSmall(sf::RenderTarget& target, sf::FloatRect rect, const std::string& label, sf::Color fillColor) {
        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        box.setFillColor(fillColor);
        box.setOutlineColor(sf::Color(200, 200, 200));
        box.setOutlineThickness(1.0f);
        target.draw(box);
        DrawText(target, rect.position.x + 3.0f, rect.position.y + 1.0f, Truncate(label, rect.size.x - 6.0f, 9), 9, sf::Color(220, 220, 220));
    }
};
