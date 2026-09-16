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
class SkillGemSystem {
private:
    static constexpr int kSkillSlotCount = 5;
    static constexpr int kSpiritSlotCount = 5;
    static constexpr int kMaxPossibleSockets = 5;
    static constexpr float kSocketBoxW = 82.0f;
    static constexpr float kSocketBoxH = 16.0f;
    static constexpr float kSocketGap = 4.0f;
    static constexpr float kToggleBtnW = 46.0f;
    static constexpr float kToggleBtnH = 18.0f;

    // Shared row geometry, computed once so Update()'s click hit-testing and Render()'s
    // drawing can never drift apart. Declared this early (not just before ComputeLayout)
    // because it's also used as a parameter type by RenderGemPicker/RenderSupportPicker,
    // and a member function's parameter types (unlike its body) aren't deferred to
    // "complete-class context" -- the type must already be visible at that point.
    struct Layout {
        float uncutGemsRowY;
        float skillRowY;
        float skillRowHeight;  // full row incl. the socket sub-line below the name/stats line
        float skillMainLineH;  // click area for just the name/stats line
        float socketLineOffsetY; // offset within a skill row where the socket line starts
        float spiritLabelY;
        float spiritRowY;
        float spiritRowHeight;
        float pickerHeaderY;
        float pickerRowY;
        float pickerRowHeight;
    };

    std::shared_ptr<sf::Font> m_font;
    int m_selectedSlot = 0;
    // -1 = the bottom picker lists skill/Spirit gems for m_selectedSlot (normal mode).
    // 0-4 = the bottom picker instead lists owned support gems for that socket index of
    // the skill currently equipped in m_selectedSlot (only meaningful when m_selectedSlot
    // is a skill slot).
    int m_selectedSocket = -1;

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

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        Layout L = ComputeLayout();

        for (size_t i = 0; i < gemInventory.pendingUncutGems.size() && i < SkillGemInventoryComponent::kPendingCapacity; ++i) {
            if (UncutGemRect(L, static_cast<int>(i)).contains(mouse)) {
                const PendingUncutGem& pending = gemInventory.pendingUncutGems[i];
                gemIdentifySystem.Open(static_cast<int>(i), pending.level, pending.kind);
                return;
            }
        }

        for (int i = 0; i < kSkillSlotCount; ++i) {
            sf::FloatRect mainRect({ kPanelX + 12.0f, SkillRowY(L, i) }, { kPanelW - 24.0f, L.skillMainLineH });
            if (mainRect.contains(mouse)) { m_selectedSlot = i; m_selectedSocket = -1; return; }

            if (skillComp.skills[i].isValid) {
                OwnedGemInstance* inst = FindOwnedMutable(gemInventory, skillComp.skills[i].gemId, false);
                if (inst) {
                    for (int s = 0; s < inst->maxSockets && s < kMaxPossibleSockets; ++s) {
                        if (SocketRect(L, i, s).contains(mouse)) { m_selectedSlot = i; m_selectedSocket = s; return; }
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
            if (rect.contains(mouse)) { m_selectedSlot = kSkillSlotCount + i; m_selectedSocket = -1; return; }
        }

        if (m_selectedSocket >= 0) {
            std::vector<int> opts = SupportOptions(HostSkillTags(skillComp), gemInventory);
            for (size_t i = 0; i < opts.size(); ++i) {
                sf::FloatRect rect({ kPanelX + 12.0f, L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight }, { kPanelW - 24.0f, L.pickerRowHeight });
                if (rect.contains(mouse)) {
                    AssignSupport(skillComp, gemInventory, stats, opts[i]);
                    return;
                }
            }
        } else {
            bool spiritSlot = IsSpiritSlot();
            std::vector<int> opts = Options(gemInventory, spiritSlot);
            for (size_t i = 0; i < opts.size(); ++i) {
                sf::FloatRect rect({ kPanelX + 12.0f, L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight }, { kPanelW - 24.0f, L.pickerRowHeight });
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
            DrawButtonSmall(target, rect, KindAbbrev(pending.kind) + std::to_string(pending.level), sf::Color(90, 60, 90));
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
            highlight.setOutlineColor(sf::Color(90, 90, 100));
            highlight.setOutlineThickness(1.0f);
            target.draw(highlight);

            const SkillData& skill = skillComp.skills[i];
            std::string line = "[" + kSlotKeys[i] + "] " + (skill.isValid ? skill.name : "-- Empty --");
            if (skill.isValid) {
                line += " Lv" + std::to_string(skill.level) + "  (cd " + FormatFloat(skill.cooldownTime) + "s, " + std::to_string(skill.mpCost) + " mp";
                if (DealsElementalDamage(skill.behaviorType)) line += ", " + ElementName(skill.element);
                line += ")";
            }

            DrawText(target, panelX + 16.0f, y + 2.0f, Truncate(line, contentWidth, 12), 12,
                skill.isValid ? sf::Color(220, 220, 220) : sf::Color(120, 120, 120));

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
            highlight.setOutlineColor(sf::Color(90, 100, 100));
            highlight.setOutlineThickness(1.0f);
            target.draw(highlight);

            int gemId = loadout.auraGemIds[i];
            const GemDefinition* def = gemId >= 0 ? SkillGemData::Find(gemId) : nullptr;
            bool active = def && loadout.active[i];
            std::string line = "[Spirit " + std::to_string(i + 1) + "] " + (def ? def->skill.name : "-- Empty --");
            if (def) {
                int level = SpiritAuraSystem::LevelOf(gemInventory, gemId);
                line += " Lv" + std::to_string(level) + "  (" + FormatFloat(SpiritAuraSystem::SpiritCostOf(gemId, gemInventory)) + " spirit)";
            }

            DrawText(target, panelX + 16.0f, y + 2.0f, Truncate(line, contentWidth - kToggleBtnW - 8.0f, 12), 12,
                def ? sf::Color(180, 230, 230) : sf::Color(120, 120, 120));

            if (def) {
                sf::FloatRect toggleRect = SpiritToggleRect(L, i);
                DrawButtonSmall(target, toggleRect, active ? "ON" : "OFF",
                    active ? sf::Color(60, 130, 70) : sf::Color(70, 60, 60));
            }
        }

        if (m_selectedSocket >= 0) {
            std::vector<int> opts = SupportOptions(HostSkillTags(skillComp), gemInventory);
            RenderSupportPicker(target, L, panelX, contentWidth, opts, skillComp, gemInventory, stats);
        } else {
            bool spiritSlot = IsSpiritSlot();
            std::vector<int> opts = Options(gemInventory, spiritSlot);
            RenderGemPicker(target, L, panelX, contentWidth, opts, spiritSlot, skillComp, loadout, gemInventory, stats);
        }

        target.setView(oldView);
    }

private:
    void RenderGemPicker(sf::RenderTarget& target, const Layout& L, float panelX, float contentWidth,
        const std::vector<int>& opts, bool spiritSlot, const PlayerSkill& skillComp,
        const SpiritGemLoadoutComponent& loadout, const SkillGemInventoryComponent& gemInventory,
        const CharacterStatsComponent& stats) {
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

            sf::RectangleShape highlight({ kPanelW - 24.0f, L.pickerRowHeight });
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
                int level = SpiritAuraSystem::LevelOf(gemInventory, gemId);
                label = def->skill.name + " Lv" + std::to_string(level);
                if (DealsElementalDamage(def->skill.behaviorType)) {
                    label += " (" + ElementName(def->skill.element) + ")";
                }

                int required = SkillGemScaling::RequiredStat(def->baseRequirement, level);
                int have = SpiritAuraSystem::StatValue(stats, def->primaryAttribute);
                label += "  [" + std::to_string(required) + " " + SpiritAuraSystem::AttributeName(def->primaryAttribute) + "]";
                color = (have < required) ? sf::Color(230, 100, 100) : sf::Color(220, 220, 255);
            }
            DrawText(target, panelX + 16.0f, y + L.pickerRowHeight / 2.0f - 7.0f, Truncate(label, contentWidth, 12), 12, color);
        }

        float messageY = L.pickerRowY + static_cast<float>(opts.size()) * L.pickerRowHeight + 14.0f;
        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 12.0f, messageY, Truncate(lastActionMessage, contentWidth, 12), 12, sf::Color(255, 230, 120));
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

        DrawText(target, panelX + 12.0f, L.pickerHeaderY,
            "Support gems for socket " + std::to_string(m_selectedSocket + 1) + " (click to socket):",
            13, sf::Color(200, 200, 200));

        for (size_t i = 0; i < opts.size(); ++i) {
            int gemId = opts[i];
            float y = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight;
            bool current = (gemId == currentId);

            sf::RectangleShape highlight({ kPanelW - 24.0f, L.pickerRowHeight });
            highlight.setPosition({ panelX + 12.0f, y });
            highlight.setFillColor(current ? sf::Color(70, 100, 70, 180) : sf::Color(30, 30, 34, 120));
            target.draw(highlight);

            std::string label;
            sf::Color color = sf::Color(200, 200, 200);
            if (gemId < 0) {
                label = "-- Empty --";
            } else {
                const SupportGemDefinition* def = SupportGemData::Find(gemId);
                if (!def) continue;
                label = def->name;
                int have = SpiritAuraSystem::StatValue(stats, def->primaryAttribute);
                label += "  [" + std::to_string(def->requirement) + " " + SpiritAuraSystem::AttributeName(def->primaryAttribute) + "]";
                bool categoryConflict = inst && HasCategoryConflict(*inst, m_selectedSocket, *def);
                if (categoryConflict) label += " (category conflict)";
                color = (have < def->requirement || categoryConflict) ? sf::Color(230, 100, 100) : sf::Color(220, 220, 255);
            }
            DrawText(target, panelX + 16.0f, y + L.pickerRowHeight / 2.0f - 7.0f, Truncate(label, contentWidth, 12), 12, color);
        }

        float messageY = L.pickerRowY + static_cast<float>(opts.size()) * L.pickerRowHeight + 14.0f;
        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 12.0f, messageY, Truncate(lastActionMessage, contentWidth, 12), 12, sf::Color(255, 230, 120));
        }
    }

    Layout ComputeLayout() const {
        Layout L;
        L.uncutGemsRowY = kPanelY + 22.0f;
        L.skillRowY = L.uncutGemsRowY + 24.0f;
        L.skillRowHeight = 40.0f;
        L.skillMainLineH = 20.0f;
        L.socketLineOffsetY = 20.0f;
        L.spiritLabelY = L.skillRowY + static_cast<float>(kSkillSlotCount) * L.skillRowHeight + 6.0f;
        L.spiritRowY = L.spiritLabelY + 16.0f;
        L.spiritRowHeight = 22.0f;
        L.pickerHeaderY = L.spiritRowY + static_cast<float>(kSpiritSlotCount) * L.spiritRowHeight + 16.0f;
        L.pickerRowY = L.pickerHeaderY + 20.0f;
        L.pickerRowHeight = 20.0f;
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

    // Shared with CharacterSheetSystem's identical constants: the two panels are
    // tab-switched (mutually exclusive, last one toggled wins) so they occupy the
    // same screen slot.
    static constexpr float kPanelW = 460.0f;
    static constexpr float kPanelH = 680.0f;
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

    void DrawSocketBox(sf::RenderTarget& target, sf::FloatRect rect, int supportGemId, bool selected) {
        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        box.setFillColor(supportGemId >= 0 ? sf::Color(45, 55, 70) : sf::Color(30, 30, 34));
        box.setOutlineColor(selected ? sf::Color::Yellow : sf::Color(90, 90, 100));
        box.setOutlineThickness(selected ? 2.0f : 1.0f);
        target.draw(box);

        std::string label = "+ Empty";
        if (supportGemId >= 0) {
            const SupportGemDefinition* def = SupportGemData::Find(supportGemId);
            label = def ? def->name : "?";
        }
        DrawText(target, rect.position.x + 3.0f, rect.position.y + 1.0f, Truncate(label, rect.size.x - 6.0f, 9), 9,
            supportGemId >= 0 ? sf::Color(200, 220, 255) : sf::Color(140, 140, 140));
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
