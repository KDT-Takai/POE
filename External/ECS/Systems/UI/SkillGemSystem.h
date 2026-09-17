#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <cstdio>
#include "../../Registry/Registry.h"
#include "../../Components/PlayerSkill/PlayerSkill.h"
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Item/Inventory.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Skill/SkillGemData.h"
#include "../Skill/SupportGemData.h"
#include "../Skill/SupportGemSystem.h"
#include "../Skill/SkillGemScaling.h"
#include "../Skill/SkillTags.h"
#include "../Skill/SpiritAuraSystem.h"
#include "ItemUIHelpers.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"

// Lets the player freely reassign any of the 5 skill slots (keybound Skill1-5) or 5
// Spirit slots (no keybind, always-on like a passive node, budget-limited by
// CharacterStatsComponent::maxSpirit) to any identified SkillGem-category item sitting in
// their bag (InventoryComponent::items) -- gems live in the item grid exactly like
// Waystones do (see AI/DECISIONS.md "スキルジェムもウェイストーンみたいにアイテム欄に置く").
// Equipping physically moves the item out of the bag into PlayerSkill::equippedItems[i] /
// SpiritGemLoadoutComponent::items[i]; unequipping moves it back (rejected if the bag has
// no room, same as unequipping gear). Uncut Gems are identified separately, via
// right-click in InventorySystem (see GemIdentifySystem) -- this screen only ever shows
// already-identified gems in its picker.
// Skill gems additionally have 2-5 support-gem sockets (Jeweller's Orb, spent from this
// screen, expands them); socketing a support gem likewise consumes the physical support
// item from the bag, and unsocketing hands one back. Spirit/Aura gems don't have sockets,
// since their effect is a flat stat bonus with nothing for a support gem's damage/
// cooldown/mana modifiers to act on.
//
// Visual language leans on PoE2's own conventions (colored gem icons instead of plain
// text rows, a card-style picker instead of a thin text list) despite having no
// hand-drawn gem art: each gem's icon is a plain circle tinted by its GemAttribute (Str/
// Dex/Int, same red/green/blue coding as CharacterSheetSystem), and support sockets are
// tinted by SupportCategory. The picker list is a clipped, mouse-wheel-scrollable
// viewport (see m_pickerScroll) so a long bag doesn't spill options past the panel with no
// way to reach them.
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

    // Read-only reference view of every skill/support gem in the game (whether owned or
    // not), toggled by the "All Skills" button -- separate from the normal slot/picker UI
    // ("すきるいちらんはげーむない" -> the reference needs to live in-game, not just in
    // Docs/スキル一覧.md). Reuses DrawPickerCard's name/req/description card layout.
    bool m_codexOpen = false;
    float m_codexScroll = 0.0f;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    SkillGemSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() { isOpen = !isOpen; m_pickerScroll = 0.0f; m_codexOpen = false; m_codexScroll = 0.0f; }
    void Close() { isOpen = false; }

    // Used by GameScene to resolve click ownership between the non-pausing menus:
    // only steal the click if the cursor is actually over this panel's rect.
    bool IsPointInPanel(sf::Vector2f point) const {
        if (!isOpen) return false;
        return sf::FloatRect({ kPanelX, kPanelY }, { kPanelW, kPanelH }).contains(point);
    }

    // Selection and assignment are entirely mouse-driven: click a slot row to select it,
    // then click a gem in the (scrollable) list below to equip it (moving it out of the
    // bag). The old Up/Down/Left/Right keyboard navigation was removed because this menu
    // doesn't pause the world, so those arrow keys were simultaneously moving the player
    // (see InputSystem's movement handling).
    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, PlayerSkill, SpiritGemLoadoutComponent, InventoryComponent, EquipmentComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& skillComp = registry.GetComponent<PlayerSkill>(player);
        auto& loadout = registry.GetComponent<SpiritGemLoadoutComponent>(player);
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        Layout L = ComputeLayout();
        sf::Vector2f mouseAny = InputManager::Instance().GetMouseInput().GetMousePointF();
        float wheel = InputManager::Instance().GetMouseWheelDelta();

        if (m_codexOpen) {
            if (wheel != 0.0f) {
                sf::FloatRect codexClip({ kPanelX, CodexTopY() }, { kPanelW, CodexViewportH() });
                if (codexClip.contains(mouseAny)) {
                    float contentH = static_cast<float>(BuildCodexEntries(stats).size()) * kCodexRowHeight;
                    float maxScroll = (std::max)(0.0f, contentH - CodexViewportH());
                    m_codexScroll = std::clamp(m_codexScroll - wheel * kCodexRowHeight, 0.0f, maxScroll);
                }
            }
            auto& mouseInputCodex = InputManager::Instance().GetMouseInput();
            if (mouseInputCodex.IsGetMouse(sf::Mouse::Button::Left) && CodexToggleRect().contains(mouseAny)) {
                m_codexOpen = false;
            }
            return;
        }

        // Mouse-wheel scroll over the picker viewport -- independent of the left-click
        // gate below, since scrolling shouldn't require holding/clicking anything.
        if (wheel != 0.0f) {
            sf::FloatRect pickerClip({ kPanelX, L.pickerRowY }, { kPanelW, L.pickerViewportH });
            if (pickerClip.contains(mouseAny)) {
                int optCount = (m_selectedSocket >= 0)
                    ? static_cast<int>(SupportOptions(HostSkillTags(skillComp), inventory).size())
                    : static_cast<int>(Options(inventory, IsSpiritSlot()).size());
                float contentH = static_cast<float>(optCount) * L.pickerRowHeight;
                float maxScroll = (std::max)(0.0f, contentH - L.pickerViewportH);
                m_pickerScroll = std::clamp(m_pickerScroll - wheel * L.pickerRowHeight, 0.0f, maxScroll);
            }
        }

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        if (CodexToggleRect().contains(mouse)) {
            m_codexOpen = true;
            m_codexScroll = 0.0f;
            return;
        }

        for (int i = 0; i < kSkillSlotCount; ++i) {
            sf::FloatRect mainRect({ kPanelX + 12.0f, SkillRowY(L, i) }, { kPanelW - 24.0f, L.skillMainLineH });
            if (mainRect.contains(mouse)) { SelectSlot(i, -1); return; }

            if (skillComp.equippedItems[i]) {
                ItemComponent& item = *skillComp.equippedItems[i];
                for (int s = 0; s < item.skillGemMaxSockets && s < kMaxPossibleSockets; ++s) {
                    if (SocketRect(L, i, s).contains(mouse)) { SelectSlot(i, s); return; }
                }
                if (item.skillGemMaxSockets < kMaxPossibleSockets && stats.jewellersOrbs > 0
                    && SocketRect(L, i, item.skillGemMaxSockets).contains(mouse)) {
                    item.skillGemMaxSockets++;
                    stats.jewellersOrbs--;
                    lastActionMessage = "Socket added (" + std::to_string(item.skillGemMaxSockets) + "/5)";
                    messageTimer = 2.0f;
                    return;
                }
            }
        }

        for (int i = 0; i < kSpiritSlotCount; ++i) {
            if (loadout.items[i] && SpiritToggleRect(L, i).contains(mouse)) {
                std::string message;
                SpiritAuraSystem::TryToggleActive(registry, player, loadout, i, equipment, stats, message);
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
            std::vector<int> opts = SupportOptions(HostSkillTags(skillComp), inventory);
            for (size_t i = 0; i < opts.size(); ++i) {
                float rowY = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight - m_pickerScroll;
                sf::FloatRect rect({ kPanelX + 12.0f, rowY }, { kPanelW - 24.0f, L.pickerRowHeight });
                if (rect.contains(mouse)) {
                    AssignSupport(skillComp, inventory, stats, opts[i]);
                    return;
                }
            }
        } else {
            bool spiritSlot = IsSpiritSlot();
            std::vector<int> opts = Options(inventory, spiritSlot);
            for (size_t i = 0; i < opts.size(); ++i) {
                float rowY = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight - m_pickerScroll;
                sf::FloatRect rect({ kPanelX + 12.0f, rowY }, { kPanelW - 24.0f, L.pickerRowHeight });
                if (rect.contains(mouse)) {
                    AssignSelected(registry, player, skillComp, loadout, inventory, equipment, stats, opts[i]);
                    return;
                }
            }
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, PlayerSkill, SpiritGemLoadoutComponent, InventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& skillComp = registry.GetComponent<PlayerSkill>(player);
        auto& loadout = registry.GetComponent<SpiritGemLoadoutComponent>(player);
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
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
            Truncate("Skill Gems (" + closeKey + " to close) - click a slot, then a gem from your bag", contentWidth - 120.0f, 12), 12, sf::Color(255, 220, 120));

        sf::FloatRect codexBtn = CodexToggleRect();
        DrawButtonSmall(target, codexBtn, m_codexOpen ? "< 戻る" : "スキル一覧",
            m_codexOpen ? sf::Color(70, 70, 90) : sf::Color(60, 85, 60));

        if (m_codexOpen) {
            RenderCodex(target, stats);
            target.setView(oldView);
            return;
        }

        const std::string kSlotKeys[kSkillSlotCount] = {
            KeyToString(binds.Get(GameAction::Skill1)),
            KeyToString(binds.Get(GameAction::Skill2)),
            KeyToString(binds.Get(GameAction::Skill3)),
            KeyToString(binds.Get(GameAction::Skill4)),
            KeyToString(binds.Get(GameAction::Skill5)),
        };

        Layout L = ComputeLayout();

        // Hover tooltip (name + description), shown like a normal game tooltip wherever the
        // cursor rests on a gem -- equipped slot, Spirit slot, or a filled support socket --
        // so the player can see what a skill does without opening the picker or the "All
        // Skills" codex. Populated while drawing below, rendered once at the very end (on
        // top of everything, in default-view screen space) so it isn't drawn under later rows.
        sf::Vector2f mouseNow = InputManager::Instance().GetMouseInput().GetMousePointF();
        bool tipActive = false;
        std::string tipName, tipStat, tipDesc;
        sf::Color tipColor(200, 200, 200);

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
            const std::optional<ItemComponent>& equipped = skillComp.equippedItems[i];
            const GemDefinition* def = equipped ? SkillGemData::Find(equipped->skillGemId) : nullptr;
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

            if (equipped) {
                for (int s = 0; s < equipped->skillGemMaxSockets && s < kMaxPossibleSockets; ++s) {
                    bool socketSelected = selected && (s == m_selectedSocket);
                    sf::FloatRect socketRect = SocketRect(L, i, s);
                    DrawSocketBox(target, socketRect, equipped->skillGemSupportIds[s], socketSelected);

                    int supportId = equipped->skillGemSupportIds[s];
                    if (supportId >= 0 && socketRect.contains(mouseNow)) {
                        const SupportGemDefinition* supportDef = SupportGemData::Find(supportId);
                        if (supportDef) {
                            tipActive = true;
                            tipName = supportDef->name;
                            tipStat = "Support";
                            tipDesc = supportDef->description;
                            tipColor = CategoryColor(supportDef->category);
                        }
                    }
                }
                if (equipped->skillGemMaxSockets < kMaxPossibleSockets) {
                    bool canAfford = stats.jewellersOrbs > 0;
                    sf::FloatRect btn = SocketRect(L, i, equipped->skillGemMaxSockets);
                    DrawButtonSmall(target, btn, "+Jeweller(" + std::to_string(stats.jewellersOrbs) + ")",
                        canAfford ? sf::Color(70, 90, 70) : sf::Color(50, 50, 55));
                }
            }

            sf::FloatRect mainRectForTip({ panelX + 12.0f, y }, { panelW - 24.0f, L.skillMainLineH });
            if (skill.isValid && mainRectForTip.contains(mouseNow)) {
                tipActive = true;
                tipName = skill.name + " Lv" + std::to_string(skill.level);
                std::string statLine = "CD " + FormatFloat(skill.cooldownTime) + "s, MP" + std::to_string(skill.mpCost);
                if (DealsElementalDamage(skill.behaviorType)) statLine += ", " + ElementName(skill.element);
                tipStat = statLine;
                tipDesc = skill.description;
                tipColor = def ? AttributeColor(def->primaryAttribute) : sf::Color(200, 200, 200);
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

            const std::optional<ItemComponent>& item = loadout.items[i];
            const GemDefinition* def = item ? SkillGemData::Find(item->skillGemId) : nullptr;
            bool active = item && loadout.active[i];

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
            if (def && item) {
                line += " Lv" + std::to_string(item->skillGemLevel) + "  (" + FormatFloat(SpiritAuraSystem::SpiritCostOf(*item)) + " spirit)";
            }
            DrawText(target, textX, y + L.spiritRowHeight / 2.0f - 7.0f, Truncate(line, textContentW, 12), 12,
                def ? sf::Color(180, 230, 230) : sf::Color(120, 120, 120));

            if (def) {
                sf::FloatRect toggleRect = SpiritToggleRect(L, i);
                DrawButtonSmall(target, toggleRect, active ? "ON" : "OFF",
                    active ? sf::Color(60, 130, 70) : sf::Color(70, 60, 60));
            }

            sf::FloatRect spiritRectForTip({ panelX + 12.0f, y }, { panelW - 24.0f, L.spiritRowHeight });
            if (def && spiritRectForTip.contains(mouseNow)) {
                tipActive = true;
                tipName = def->skill.name + " Lv" + (item ? std::to_string(item->skillGemLevel) : "1");
                tipStat = "Spirit " + FormatFloat(item ? SpiritAuraSystem::SpiritCostOf(*item) : def->skill.spiritCost);
                tipDesc = def->skill.description;
                tipColor = AttributeColor(def->primaryAttribute);
            }
        }

        // Picker viewport: clipped to its own rect (via a scoped view, same technique as
        // PassiveTreeSystem/AtlasSystem's tree/node-graph area) so a long option list
        // scrolls within a fixed box instead of spilling past the panel with no way to
        // reach the lower entries.
        std::vector<int> pickerOpts = (m_selectedSocket >= 0)
            ? SupportOptions(HostSkillTags(skillComp), inventory)
            : Options(inventory, IsSpiritSlot());

        DrawText(target, panelX + 12.0f, L.pickerHeaderY,
            (m_selectedSocket >= 0)
                ? "Support gems in your bag for socket " + std::to_string(m_selectedSocket + 1) + " (click to socket):"
                : (IsSpiritSlot() ? "Spirit gems in your bag (click to equip):" : "Skill gems in your bag (click to equip):"),
            13, sf::Color(200, 200, 200));

        sf::Vector2u winSize = target.getSize();
        sf::View pickerView(sf::FloatRect({ panelX, L.pickerRowY }, { panelW, L.pickerViewportH }));
        pickerView.setViewport(sf::FloatRect(
            { panelX / static_cast<float>(winSize.x), L.pickerRowY / static_cast<float>(winSize.y) },
            { panelW / static_cast<float>(winSize.x), L.pickerViewportH / static_cast<float>(winSize.y) }));
        target.setView(pickerView);

        if (m_selectedSocket >= 0) {
            RenderSupportPicker(target, L, panelX, contentWidth, pickerOpts, inventory, stats);
        } else {
            RenderGemPicker(target, L, panelX, contentWidth, pickerOpts, inventory, stats);
        }

        target.setView(target.getDefaultView());

        // Scrollbar affordance -- only shown once content actually overflows the
        // viewport, so it doesn't clutter short lists.
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

        if (tipActive) {
            DrawHoverTooltip(target, mouseNow, tipName, tipStat, tipDesc, tipColor);
        }

        target.setView(oldView);
    }

private:
    void SelectSlot(int slot, int socket) {
        m_selectedSlot = slot;
        m_selectedSocket = socket;
        m_pickerScroll = 0.0f;
    }

    // opts holds BAG INDICES into inventory.items (-1 = Empty/unequip) -- equipping
    // physically removes the item from the bag, so an equipped gem never appears in this
    // list (it's simply not in the bag anymore), unlike the old gemId-based picker which
    // needed a separate "current" highlight to show that.
    void RenderGemPicker(sf::RenderTarget& target, const Layout& L, float panelX, float contentWidth,
        const std::vector<int>& opts, const InventoryComponent& inventory, const CharacterStatsComponent& stats) {
        for (size_t i = 0; i < opts.size(); ++i) {
            int bagIndex = opts[i];
            float y = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight - m_pickerScroll;

            std::string name = "-- Empty --";
            std::string reqLine;
            sf::Color iconColor(60, 60, 65);
            sf::Color statusColor(200, 220, 255);
            bool hasIcon = false;

            std::string description;
            if (bagIndex >= 0) {
                const ItemComponent& item = inventory.items[bagIndex];
                const GemDefinition* def = SkillGemData::Find(item.skillGemId);
                if (!def) continue;
                hasIcon = true;
                iconColor = AttributeColor(def->primaryAttribute);
                name = def->skill.name + " Lv" + std::to_string(item.skillGemLevel);
                if (DealsElementalDamage(def->skill.behaviorType)) name += " (" + ElementName(def->skill.element) + ")";
                description = def->skill.description;

                int required = SkillGemScaling::RequiredStat(def->baseRequirement, item.skillGemLevel);
                int have = SpiritAuraSystem::StatValue(stats, def->primaryAttribute);
                reqLine = "Requires " + std::to_string(required) + " " + SpiritAuraSystem::AttributeName(def->primaryAttribute)
                    + " (have " + std::to_string(have) + ")";
                statusColor = (have < required) ? sf::Color(230, 100, 100) : sf::Color(150, 220, 150);
            }

            DrawPickerCard(target, panelX, contentWidth, y, L.pickerRowHeight, name, reqLine, description, iconColor, hasIcon, statusColor);
        }
    }

    void RenderSupportPicker(sf::RenderTarget& target, const Layout& L, float panelX, float contentWidth,
        const std::vector<int>& opts, const InventoryComponent& inventory, const CharacterStatsComponent& stats) {
        for (size_t i = 0; i < opts.size(); ++i) {
            int bagIndex = opts[i];
            float y = L.pickerRowY + static_cast<float>(i) * L.pickerRowHeight - m_pickerScroll;

            std::string name = "-- Empty --";
            std::string reqLine;
            sf::Color iconColor(60, 60, 65);
            sf::Color statusColor(200, 220, 255);
            bool hasIcon = false;

            std::string description;
            if (bagIndex >= 0) {
                const ItemComponent& item = inventory.items[bagIndex];
                const SupportGemDefinition* def = SupportGemData::Find(item.skillGemId);
                if (!def) continue;
                hasIcon = true;
                iconColor = CategoryColor(def->category);
                name = def->name;
                description = def->description;
                int have = SpiritAuraSystem::StatValue(stats, def->primaryAttribute);
                reqLine = "Requires " + std::to_string(def->requirement) + " " + SpiritAuraSystem::AttributeName(def->primaryAttribute)
                    + " (have " + std::to_string(have) + ")";
                statusColor = (have < def->requirement) ? sf::Color(230, 100, 100) : sf::Color(150, 220, 150);
            }

            DrawPickerCard(target, panelX, contentWidth, y, L.pickerRowHeight, name, reqLine, description, iconColor, hasIcon, statusColor);
        }
    }

    // One picker option, PoE2-style: a colored gem icon + three-line text block (name,
    // requirement/status, and a short effect description) inside a bordered card, instead
    // of the old single line of dense text. The description line is what answers "what does
    // this skill actually do" when browsing the picker after pressing G.
    void DrawPickerCard(sf::RenderTarget& target, float panelX, float contentWidth, float y, float rowHeight,
        const std::string& name, const std::string& reqLine, const std::string& description,
        sf::Color iconColor, bool hasIcon, sf::Color statusColor) {
        sf::RectangleShape card({ kPanelW - 24.0f, rowHeight - 4.0f });
        card.setPosition({ panelX + 12.0f, y });
        card.setFillColor(sf::Color(28, 28, 33, 190));
        card.setOutlineColor(sf::Color(80, 80, 90));
        card.setOutlineThickness(1.0f);
        target.draw(card);

        float iconR = kIconRadius * 0.8f;
        sf::Vector2f iconCenter(panelX + 12.0f + 10.0f + iconR, y + (rowHeight - 4.0f) / 2.0f);
        DrawGemIcon(target, iconCenter, iconR, iconColor, hasIcon, "");

        float textX = panelX + 12.0f + 10.0f + iconR * 2.0f + 10.0f;
        float textW = contentWidth - (textX - panelX) - 8.0f;
        DrawText(target, textX, y + 4.0f, Truncate(name, textW, 13), 13, sf::Color(225, 225, 230));
        if (!reqLine.empty()) {
            DrawText(target, textX, y + 21.0f, Truncate(reqLine, textW, 11), 11, statusColor);
        }
        if (!description.empty()) {
            DrawText(target, textX, y + 37.0f, Truncate(description, textW, 10), 10, sf::Color(170, 170, 178));
        }
    }

    static constexpr float kCodexRowHeight = 58.0f;

    sf::FloatRect CodexToggleRect() const {
        return sf::FloatRect({ kPanelX + kPanelW - 130.0f, kPanelY + 8.0f }, { 110.0f, 22.0f });
    }
    float CodexTopY() const { return kPanelY + 42.0f; }
    float CodexViewportH() const { return (kPanelY + kPanelH) - CodexTopY() - 20.0f; }

    struct CodexEntry {
        std::string name;
        std::string reqLine;
        std::string description;
        sf::Color iconColor;
        sf::Color statusColor;
    };

    // Every skill/support gem in the game, owned or not -- the reference catalog behind
    // the "All Skills" button. Built fresh each frame (25 entries, negligible cost) so it
    // never drifts from the live SkillGemData/SupportGemData catalogs.
    std::vector<CodexEntry> BuildCodexEntries(const CharacterStatsComponent& stats) const {
        std::vector<CodexEntry> out;
        for (const auto& def : SkillGemData::Gems()) {
            CodexEntry e;
            e.name = def.skill.name;
            e.description = def.skill.description;
            e.iconColor = AttributeColor(def.primaryAttribute);

            int required = SkillGemScaling::RequiredStat(def.baseRequirement, 1);
            int have = SpiritAuraSystem::StatValue(stats, def.primaryAttribute);
            e.statusColor = (have < required) ? sf::Color(230, 100, 100) : sf::Color(150, 220, 150);

            std::string statLine;
            if (def.skill.behaviorType == SkillBehaviorType::Aura) {
                statLine = "Aura, Spirit " + FormatFloat(def.skill.spiritCost);
            } else if (def.skill.behaviorType == SkillBehaviorType::Minion) {
                statLine = "Minion, Spirit " + FormatFloat(def.skill.spiritCost);
            } else {
                statLine = "CD " + FormatFloat(def.skill.cooldownTime) + "s, MP" + std::to_string(def.skill.mpCost);
                if (def.skill.damage > 0.0f) statLine += ", " + FormatFloat(def.skill.damage) + "% dmg";
                if (DealsElementalDamage(def.skill.behaviorType)) statLine += ", " + ElementName(def.skill.element);
            }
            statLine += " | Req " + std::to_string(required) + " " + SpiritAuraSystem::AttributeName(def.primaryAttribute)
                + " (have " + std::to_string(have) + ")";
            e.reqLine = statLine;
            out.push_back(e);
        }
        for (const auto& def : SupportGemData::Gems()) {
            CodexEntry e;
            e.name = def.name;
            e.description = def.description;
            e.iconColor = CategoryColor(def.category);

            int have = SpiritAuraSystem::StatValue(stats, def.primaryAttribute);
            e.statusColor = (have < def.requirement) ? sf::Color(230, 100, 100) : sf::Color(150, 220, 150);
            e.reqLine = "Support | Req " + std::to_string(def.requirement) + " " + SpiritAuraSystem::AttributeName(def.primaryAttribute)
                + " (have " + std::to_string(have) + ")";
            out.push_back(e);
        }
        return out;
    }

    // Scrollable, clipped list of every gem in the game (same viewport-clipping technique
    // as the normal picker) -- purely informational, nothing here is clickable.
    void RenderCodex(sf::RenderTarget& target, const CharacterStatsComponent& stats) {
        float contentWidth = kPanelW - 32.0f;
        std::vector<CodexEntry> entries = BuildCodexEntries(stats);

        DrawText(target, kPanelX + 12.0f, kPanelY + 22.0f,
            "全スキル/サポートジェム一覧(未所持含む、スクロールで続きを表示):", 12, sf::Color(200, 200, 200));

        float topY = CodexTopY();
        float viewportH = CodexViewportH();

        sf::Vector2u winSize = target.getSize();
        sf::View codexView(sf::FloatRect({ kPanelX, topY }, { kPanelW, viewportH }));
        codexView.setViewport(sf::FloatRect(
            { kPanelX / static_cast<float>(winSize.x), topY / static_cast<float>(winSize.y) },
            { kPanelW / static_cast<float>(winSize.x), viewportH / static_cast<float>(winSize.y) }));
        target.setView(codexView);

        for (size_t i = 0; i < entries.size(); ++i) {
            float y = topY + static_cast<float>(i) * kCodexRowHeight - m_codexScroll;
            const CodexEntry& e = entries[i];
            DrawPickerCard(target, kPanelX, contentWidth, y, kCodexRowHeight, e.name, e.reqLine, e.description, e.iconColor, true, e.statusColor);
        }

        target.setView(target.getDefaultView());

        float contentH = static_cast<float>(entries.size()) * kCodexRowHeight;
        if (contentH > viewportH) {
            sf::FloatRect track({ kPanelX + kPanelW - kScrollBarW - 4.0f, topY }, { kScrollBarW, viewportH });
            sf::RectangleShape trackShape(track.size);
            trackShape.setPosition(track.position);
            trackShape.setFillColor(sf::Color(40, 40, 45));
            target.draw(trackShape);

            float thumbH = (std::max)(16.0f, viewportH * (viewportH / contentH));
            float maxScroll = contentH - viewportH;
            float thumbY = track.position.y + (maxScroll > 0.0f ? (m_codexScroll / maxScroll) * (viewportH - thumbH) : 0.0f);
            sf::RectangleShape thumb({ kScrollBarW, thumbH });
            thumb.setPosition({ track.position.x, thumbY });
            thumb.setFillColor(sf::Color(130, 130, 150));
            target.draw(thumb);
        }
    }

    Layout ComputeLayout() const {
        Layout L;
        L.skillRowY = kPanelY + 46.0f;
        L.skillRowHeight = 54.0f;
        L.skillMainLineH = 34.0f;
        L.socketLineOffsetY = 34.0f;
        L.spiritLabelY = L.skillRowY + static_cast<float>(kSkillSlotCount) * L.skillRowHeight + 8.0f;
        L.spiritRowY = L.spiritLabelY + 18.0f;
        L.spiritRowHeight = 36.0f;
        L.pickerHeaderY = L.spiritRowY + static_cast<float>(kSpiritSlotCount) * L.spiritRowHeight + 16.0f;
        L.pickerRowY = L.pickerHeaderY + 22.0f;
        L.pickerRowHeight = 64.0f; // tall enough for name + requirement + a description line
        L.pickerViewportH = (std::max)(60.0f, (kPanelY + kPanelH) - L.pickerRowY - 34.0f);
        return L;
    }

    float SkillRowY(const Layout& L, int index) const { return L.skillRowY + static_cast<float>(index) * L.skillRowHeight; }
    float SpiritRowY(const Layout& L, int index) const { return L.spiritRowY + static_cast<float>(index) * L.spiritRowHeight; }

    sf::FloatRect SocketRect(const Layout& L, int rowIndex, int socketIndex) const {
        float x = kPanelX + 16.0f + static_cast<float>(socketIndex) * (kSocketBoxW + kSocketGap);
        float y = SkillRowY(L, rowIndex) + L.socketLineOffsetY;
        return sf::FloatRect({ x, y }, { kSocketBoxW, kSocketBoxH });
    }

    // ON/OFF button pinned to the right edge of a Spirit row (equipping a gem there
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

    // True if `candidate` shares a SupportCategory with any OTHER socketed support on
    // `hostItem` (excludeSocket is the socket being written to, not a conflict with itself).
    static bool HasCategoryConflict(const ItemComponent& hostItem, int excludeSocket, const SupportGemDefinition& candidate) {
        for (int s = 0; s < kMaxPossibleSockets; ++s) {
            if (s == excludeSocket || hostItem.skillGemSupportIds[s] < 0) continue;
            const SupportGemDefinition* other = SupportGemData::Find(hostItem.skillGemSupportIds[s]);
            if (other && other->category == candidate.category) return true;
        }
        return false;
    }

    // Moves `item` into the first free 1x1 bag cell. Returns false (item untouched by the
    // caller) if the bag has no room -- same "reject, never silently lose it" convention
    // QuickUnequip already uses for gear.
    bool ReturnToBag(InventoryComponent& inventory, ItemComponent item) const {
        int col, row;
        if (!ItemUIHelpers::FindBagFreeSpace(inventory.items, 1, 1, col, row)) return false;
        item.gridCol = col;
        item.gridRow = row;
        inventory.items.push_back(item);
        return true;
    }

    ItemComponent MakeSupportGemItem(int supportGemId) const {
        ItemComponent item;
        item.category = ItemCategory::SkillGem;
        item.skillGemIdentified = true;
        item.skillGemIsSupport = true;
        item.skillGemId = supportGemId;
        item.skillGemLevel = 1;
        const SupportGemDefinition* def = SupportGemData::Find(supportGemId);
        item.baseName = def ? def->name : "Support Gem";
        return item;
    }

    // Option list for the selected slot: -1 = Empty (unequip), followed by bag indices of
    // every identified, non-support SkillGem item matching the slot's category (activated
    // skills for skill slots, Aura gems for Spirit slots).
    std::vector<int> Options(const InventoryComponent& inventory, bool auraOnly) const {
        std::vector<int> opts = { -1 };
        for (size_t i = 0; i < inventory.items.size(); ++i) {
            const ItemComponent& item = inventory.items[i];
            if (item.category != ItemCategory::SkillGem || !item.skillGemIdentified || item.skillGemIsSupport) continue;
            const GemDefinition* def = SkillGemData::Find(item.skillGemId);
            if (!def) continue;
            bool isSpirit = SkillGemData::IsSpiritBehavior(def->skill.behaviorType);
            if (isSpirit == auraOnly) opts.push_back(static_cast<int>(i));
        }
        return opts;
    }

    // Support gems in the bag, gated by whether the host skill has the support's required
    // SkillTag (spec: only show compatible supports).
    std::vector<int> SupportOptions(unsigned int hostSkillTags, const InventoryComponent& inventory) const {
        std::vector<int> opts = { -1 };
        for (size_t i = 0; i < inventory.items.size(); ++i) {
            const ItemComponent& item = inventory.items[i];
            if (item.category != ItemCategory::SkillGem || !item.skillGemIdentified || !item.skillGemIsSupport) continue;
            const SupportGemDefinition* def = SupportGemData::Find(item.skillGemId);
            if (def && SkillTags::IsCompatible(hostSkillTags, def->requiredTag)) opts.push_back(static_cast<int>(i));
        }
        return opts;
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

    void AssignSelected(Registry& registry, Entity player, PlayerSkill& skillComp, SpiritGemLoadoutComponent& loadout, InventoryComponent& inventory,
        EquipmentComponent& equipment, CharacterStatsComponent& stats, int bagIndex) {
        if (IsSpiritSlot()) {
            AssignSpirit(registry, player, loadout, inventory, equipment, stats, bagIndex);
        } else {
            AssignGem(skillComp, inventory, equipment, stats, bagIndex);
        }
    }

    void AssignGem(PlayerSkill& skillComp, InventoryComponent& inventory,
        const EquipmentComponent& equipment, CharacterStatsComponent& stats, int bagIndex) {
        if (bagIndex < 0) {
            if (skillComp.equippedItems[m_selectedSlot]) {
                if (!ReturnToBag(inventory, *skillComp.equippedItems[m_selectedSlot])) {
                    lastActionMessage = "Bag full - can't unequip";
                    messageTimer = 2.0f;
                    return;
                }
                skillComp.equippedItems[m_selectedSlot].reset();
            }
            skillComp.skills[m_selectedSlot] = SkillData{};
            lastActionMessage = "Slot " + std::to_string(m_selectedSlot + 1) + ": Empty";
            messageTimer = 2.0f;
            return;
        }

        if (bagIndex >= static_cast<int>(inventory.items.size())) return;
        const ItemComponent& picked = inventory.items[bagIndex];
        if (picked.category != ItemCategory::SkillGem || !picked.skillGemIdentified || picked.skillGemIsSupport) return;

        for (int i = 0; i < kSkillSlotCount; ++i) {
            if (i != m_selectedSlot && skillComp.equippedItems[i] && skillComp.equippedItems[i]->skillGemId == picked.skillGemId) {
                lastActionMessage = "Already equipped in another skill slot";
                messageTimer = 2.0f;
                return;
            }
        }

        const GemDefinition* def = SkillGemData::Find(picked.skillGemId);
        if (!def) return;
        int required = SkillGemScaling::RequiredStat(def->baseRequirement, picked.skillGemLevel);
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

        ItemComponent newItem = picked; // snapshot before the erase below invalidates `picked`
        newItem.gridCol = -1;
        newItem.gridRow = -1;
        std::optional<ItemComponent> displaced = skillComp.equippedItems[m_selectedSlot];
        inventory.items.erase(inventory.items.begin() + bagIndex);
        // The item we just erased was 1x1, so the cell it freed always fits the displaced
        // gem (also 1x1) -- this can't fail in practice, but never silently drop an item.
        if (displaced) ReturnToBag(inventory, *displaced);

        skillComp.equippedItems[m_selectedSlot] = newItem;
        BuildSkillData(skillComp.skills[m_selectedSlot], newItem);
        lastActionMessage = "Slot " + std::to_string(m_selectedSlot + 1) + ": " + skillComp.skills[m_selectedSlot].name;
        messageTimer = 2.0f;
    }

    void AssignSpirit(Registry& registry, Entity player, SpiritGemLoadoutComponent& loadout, InventoryComponent& inventory,
        EquipmentComponent& equipment, CharacterStatsComponent& stats, int bagIndex) {
        int slotIndex = m_selectedSlot - kSkillSlotCount;

        if (bagIndex < 0) {
            std::optional<ItemComponent> removed = SpiritAuraSystem::TryUnequip(registry, player, loadout, slotIndex, equipment, stats);
            if (removed) {
                if (!ReturnToBag(inventory, *removed)) {
                    loadout.items[slotIndex] = removed; // put it back rather than losing it
                    lastActionMessage = "Bag full - can't unequip";
                    messageTimer = 2.0f;
                    return;
                }
            }
            lastActionMessage = "Spirit slot: Empty";
            messageTimer = 2.0f;
            return;
        }

        if (bagIndex >= static_cast<int>(inventory.items.size())) return;
        ItemComponent picked = inventory.items[bagIndex];
        if (picked.category != ItemCategory::SkillGem || !picked.skillGemIdentified || picked.skillGemIsSupport) return;
        picked.gridCol = -1;
        picked.gridRow = -1;

        std::optional<ItemComponent> displaced;
        std::string message;
        bool ok = SpiritAuraSystem::TryEquip(registry, player, loadout, slotIndex, picked, equipment, stats, displaced, message);
        if (!ok) {
            lastActionMessage = message;
            messageTimer = 2.0f;
            return;
        }

        inventory.items.erase(inventory.items.begin() + bagIndex);
        if (displaced) ReturnToBag(inventory, *displaced);
        lastActionMessage = message;
        messageTimer = 2.0f;
    }

    void AssignSupport(PlayerSkill& skillComp, InventoryComponent& inventory, CharacterStatsComponent& stats, int bagIndex) {
        if (m_selectedSlot >= kSkillSlotCount || !skillComp.equippedItems[m_selectedSlot]) return;
        ItemComponent& hostItem = *skillComp.equippedItems[m_selectedSlot];
        if (m_selectedSocket < 0 || m_selectedSocket >= hostItem.skillGemMaxSockets) return;

        if (bagIndex < 0) {
            int currentSupportId = hostItem.skillGemSupportIds[m_selectedSocket];
            if (currentSupportId >= 0) {
                if (!ReturnToBag(inventory, MakeSupportGemItem(currentSupportId))) {
                    lastActionMessage = "Bag full";
                    messageTimer = 2.0f;
                    return;
                }
            }
            hostItem.skillGemSupportIds[m_selectedSocket] = -1;
            BuildSkillData(skillComp.skills[m_selectedSlot], hostItem);
            lastActionMessage = "Socket " + std::to_string(m_selectedSocket + 1) + ": Empty";
            messageTimer = 2.0f;
            return;
        }

        if (bagIndex >= static_cast<int>(inventory.items.size())) return;
        const ItemComponent& picked = inventory.items[bagIndex];
        if (picked.category != ItemCategory::SkillGem || !picked.skillGemIdentified || !picked.skillGemIsSupport) return;

        const SupportGemDefinition* def = SupportGemData::Find(picked.skillGemId);
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
            if (s == m_selectedSocket || hostItem.skillGemSupportIds[s] < 0) continue;
            if (hostItem.skillGemSupportIds[s] == picked.skillGemId) {
                lastActionMessage = "Already socketed in this skill";
                messageTimer = 2.0f;
                return;
            }
        }
        if (HasCategoryConflict(hostItem, m_selectedSocket, *def)) {
            lastActionMessage = "Support Category conflict";
            messageTimer = 2.0f;
            return;
        }

        int displacedSupportId = hostItem.skillGemSupportIds[m_selectedSocket];
        inventory.items.erase(inventory.items.begin() + bagIndex);
        if (displacedSupportId >= 0) ReturnToBag(inventory, MakeSupportGemItem(displacedSupportId));

        hostItem.skillGemSupportIds[m_selectedSocket] = def->id;
        BuildSkillData(skillComp.skills[m_selectedSlot], hostItem);
        lastActionMessage = "Socket " + std::to_string(m_selectedSocket + 1) + ": " + def->name;
        messageTimer = 2.0f;
    }

    // Called on initial equip and again whenever a socket on this same gem changes while
    // it's equipped. See SkillGemScaling::BuildEquippedSkillData (shared with GameScene's
    // save-load restore path, so both derive identical live stats).
    void BuildSkillData(SkillData& out, const ItemComponent& gemItem) const {
        SkillGemScaling::BuildEquippedSkillData(out, gemItem);
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

    // Small floating box near the cursor (name / stat line / description), the same idea as
    // a normal game tooltip -- kept deliberately compact (one line per field, truncated
    // rather than wrapped) rather than a big panel, since the equipped-slot rows have no
    // room to show a description inline the way the picker/codex cards do.
    void DrawHoverTooltip(sf::RenderTarget& target, sf::Vector2f mouse, const std::string& name,
        const std::string& statLine, const std::string& description, sf::Color accent) {
        const float boxW = 260.0f;
        const float pad = 8.0f;
        const float lineH = 16.0f;
        int lines = 1 + (statLine.empty() ? 0 : 1) + (description.empty() ? 0 : 1);
        float boxH = pad * 2.0f + lineH * static_cast<float>(lines);

        sf::Vector2f pos = mouse + sf::Vector2f(18.0f, 18.0f);
        sf::Vector2u winSize = target.getSize();
        if (pos.x + boxW > static_cast<float>(winSize.x)) pos.x = static_cast<float>(winSize.x) - boxW - 4.0f;
        if (pos.y + boxH > static_cast<float>(winSize.y)) pos.y = static_cast<float>(winSize.y) - boxH - 4.0f;

        sf::RectangleShape box({ boxW, boxH });
        box.setPosition(pos);
        box.setFillColor(sf::Color(18, 18, 22, 240));
        box.setOutlineColor(accent);
        box.setOutlineThickness(1.5f);
        target.draw(box);

        float textW = boxW - pad * 2.0f;
        float ty = pos.y + pad;
        DrawText(target, pos.x + pad, ty, Truncate(name, textW, 13), 13, sf::Color(255, 230, 150));
        ty += lineH;
        if (!statLine.empty()) {
            DrawText(target, pos.x + pad, ty, Truncate(statLine, textW, 11), 11, sf::Color(180, 205, 225));
            ty += lineH;
        }
        if (!description.empty()) {
            DrawText(target, pos.x + pad, ty, Truncate(description, textW, 11), 11, sf::Color(225, 225, 230));
        }
    }
};
