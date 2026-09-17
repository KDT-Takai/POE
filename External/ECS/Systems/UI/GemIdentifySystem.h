#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include "../../Registry/Registry.h"
#include "../../Components/Item/Inventory.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Skill/SkillGemData.h"
#include "../Skill/SupportGemData.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"

// Gem Cutting screen. Uncut gems (Skill/Support/Spirit, see GemPickupKind) drop as normal
// bag items (ItemComponent, category==SkillGem, skillGemIdentified==false) with only a
// rolled level and kind, not a specific gem. The player right-clicks one in their bag
// (InventorySystem) to open this screen, which calls Open() with that item's bag index.
// Picking a candidate identifies the item IN PLACE (same bag slot, same grid position) --
// it stops being "uncut" and becomes that specific skill/support gem. A "Cancel" option
// leaves the item uncut in the bag.
class GemIdentifySystem {
private:
    std::shared_ptr<sf::Font> m_font;
    int m_bagIndex = -1;
    int m_level = 1;
    GemPickupKind m_kind = GemPickupKind::Skill;
    float m_scroll = 0.0f;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    GemIdentifySystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Open(int bagIndex, int level, GemPickupKind kind) {
        isOpen = true;
        m_bagIndex = bagIndex;
        m_level = level;
        m_kind = kind;
        m_scroll = 0.0f;
    }
    void Close() { isOpen = false; }

    bool IsPointInPanel(sf::Vector2f point) const {
        if (!isOpen) return false;
        return sf::FloatRect({ kPanelX, kPanelY }, { kPanelW, kPanelH }).contains(point);
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, InventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        if (!PendingEntryStillValid(inventory)) { isOpen = false; return; }

        Layout L = ComputeLayout();
        std::vector<int> opts = Options();
        sf::FloatRect listClip({ kPanelX, L.listTopY }, { kPanelW, L.viewportH });

        // Mouse-wheel scroll over the option list -- independent of the left-click gate
        // below, same pattern as SkillGemSystem's picker (see m_pickerScroll there).
        sf::Vector2f mouseAny = InputManager::Instance().GetMouseInput().GetMousePointF();
        float wheel = InputManager::Instance().GetMouseWheelDelta();
        if (wheel != 0.0f && listClip.contains(mouseAny)) {
            float contentH = static_cast<float>(opts.size()) * L.rowHeight;
            float maxScroll = (std::max)(0.0f, contentH - L.viewportH);
            m_scroll = std::clamp(m_scroll - wheel * L.rowHeight, 0.0f, maxScroll);
        }

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        if (L.CancelRect().contains(mouse)) {
            Close(); // item stays uncut in the bag -- player can cut it later
            return;
        }

        if (!listClip.contains(mouse)) return;
        for (size_t i = 0; i < opts.size(); ++i) {
            float y = L.listTopY + static_cast<float>(i) * L.rowHeight - m_scroll;
            sf::FloatRect rect({ kPanelX + 12.0f, y }, { kPanelW - 24.0f, L.rowHeight });
            if (rect.contains(mouse)) {
                Identify(inventory, opts[i]);
                return;
            }
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, InventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        if (!PendingEntryStillValid(inventory)) return;

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::RectangleShape bg({ kPanelW, kPanelH });
        bg.setPosition({ kPanelX, kPanelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(200, 160, 220));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        DrawText(target, kPanelX + 12.0f, kPanelY + 8.0f,
            "ジェム鑑定 - 未鑑定の" + std::string(KindLabel(m_kind)) + "ジェム Lv" + std::to_string(m_level) + " - 選んでください:",
            13, sf::Color(255, 220, 120));

        Layout L = ComputeLayout();
        std::vector<int> opts = Options();

        // Clipped, mouse-wheel-scrollable viewport (same technique as SkillGemSystem's
        // picker) so a long candidate list (13 skill gems, say) doesn't run into or past
        // the Cancel button with no way to reach the lower entries.
        sf::Vector2u winSize = target.getSize();
        sf::View listView(sf::FloatRect({ kPanelX, L.listTopY }, { kPanelW, L.viewportH }));
        listView.setViewport(sf::FloatRect(
            { kPanelX / static_cast<float>(winSize.x), L.listTopY / static_cast<float>(winSize.y) },
            { kPanelW / static_cast<float>(winSize.x), L.viewportH / static_cast<float>(winSize.y) }));
        target.setView(listView);

        for (size_t i = 0; i < opts.size(); ++i) {
            int gemId = opts[i];
            float y = L.listTopY + static_cast<float>(i) * L.rowHeight - m_scroll;

            std::string name;
            std::string description;
            if (m_kind == GemPickupKind::Support) {
                const SupportGemDefinition* def = SupportGemData::Find(gemId);
                if (!def) continue;
                name = def->name;
                description = def->description;
            } else {
                const GemDefinition* def = SkillGemData::Find(gemId);
                if (!def) continue;
                name = def->skill.name;
                description = def->skill.description;
            }

            sf::RectangleShape highlight({ kPanelW - 24.0f, L.rowHeight - 4.0f });
            highlight.setPosition({ kPanelX + 12.0f, y });
            highlight.setFillColor(sf::Color(35, 30, 40, 160));
            highlight.setOutlineColor(sf::Color(90, 80, 100));
            highlight.setOutlineThickness(1.0f);
            target.draw(highlight);

            float textW = kPanelW - 24.0f - 8.0f;
            DrawText(target, kPanelX + 16.0f, y + 4.0f, Truncate(name, textW, 13), 13, sf::Color(220, 220, 255));
            if (!description.empty()) {
                DrawText(target, kPanelX + 16.0f, y + 23.0f, Truncate(description, textW, 11), 11, sf::Color(175, 175, 185));
            }
        }

        target.setView(target.getDefaultView());

        float contentH = static_cast<float>(opts.size()) * L.rowHeight;
        if (contentH > L.viewportH) {
            sf::FloatRect track({ kPanelX + kPanelW - 10.0f, L.listTopY }, { 6.0f, L.viewportH });
            sf::RectangleShape trackShape(track.size);
            trackShape.setPosition(track.position);
            trackShape.setFillColor(sf::Color(40, 40, 45));
            target.draw(trackShape);

            float thumbH = (std::max)(16.0f, L.viewportH * (L.viewportH / contentH));
            float maxScroll = contentH - L.viewportH;
            float thumbY = track.position.y + (maxScroll > 0.0f ? (m_scroll / maxScroll) * (L.viewportH - thumbH) : 0.0f);
            sf::RectangleShape thumb({ 6.0f, thumbH });
            thumb.setPosition({ track.position.x, thumbY });
            thumb.setFillColor(sf::Color(130, 130, 150));
            target.draw(thumb);
        }

        sf::FloatRect cancelRect = L.CancelRect();
        sf::RectangleShape cancelBox(cancelRect.size);
        cancelBox.setPosition(cancelRect.position);
        cancelBox.setFillColor(sf::Color(90, 50, 50));
        cancelBox.setOutlineColor(sf::Color(200, 200, 200));
        cancelBox.setOutlineThickness(1.5f);
        target.draw(cancelBox);
        DrawText(target, cancelRect.position.x + 10.0f, cancelRect.position.y + 8.0f, "キャンセル", 13, sf::Color::White);

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, kPanelX + 12.0f, cancelRect.position.y + 40.0f, Truncate(lastActionMessage, kPanelW - 24.0f, 13), 13, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    struct Layout {
        float listTopY;
        float rowHeight;
        float viewportH;
        float cancelY;
        // Was hardcoded to "20.0f + 12.0f" (a leftover from a different panel's kPanelX=20)
        // -- the Cancel button rendered and clicked near the far-left edge of the screen,
        // disconnected from this panel's actual position at kPanelX=240. Fixed while adding
        // descriptions to this same layout struct.
        sf::FloatRect CancelRect() const { return sf::FloatRect({ kPanelX + 12.0f, cancelY }, { 180.0f, 32.0f }); }
    };

    Layout ComputeLayout() const {
        Layout L;
        L.listTopY = kPanelY + 36.0f;
        L.rowHeight = 46.0f; // tall enough for name + a description line
        L.cancelY = kPanelY + kPanelH - 60.0f;
        L.viewportH = (std::max)(60.0f, L.cancelY - L.listTopY - 8.0f);
        return L;
    }

    static constexpr float kPanelW = 420.0f;
    static constexpr float kPanelH = 420.0f;
    static constexpr float kPanelX = 240.0f;
    static constexpr float kPanelY = 140.0f;

    static const char* KindLabel(GemPickupKind kind) {
        switch (kind) {
        case GemPickupKind::Support: return "サポート";
        case GemPickupKind::Spirit: return "スピリット";
        default: return "スキル";
        }
    }

    // The bag can shrink/reorganize from under this panel (e.g. save/load, or the item
    // somehow leaving the bag) -- guard every access instead of assuming m_bagIndex is
    // still a valid, still-uncut SkillGem item.
    bool PendingEntryStillValid(const InventoryComponent& inv) const {
        if (m_bagIndex < 0 || static_cast<size_t>(m_bagIndex) >= inv.items.size()) return false;
        const ItemComponent& item = inv.items[m_bagIndex];
        return item.category == ItemCategory::SkillGem && !item.skillGemIdentified;
    }

    // Candidates for the rolled kind: Skill/Spirit list every SkillGemData entry split by
    // Aura behaviorType (matching the old isSpirit convention); Support lists every
    // SupportGemData entry. No "already owned" filtering -- unlike the old singleton
    // OwnedGemInstance model, gems are now independent bag items, so owning duplicates (at
    // different levels) is expected and fine, same as owning multiple Waystones.
    std::vector<int> Options() const {
        std::vector<int> opts;
        if (m_kind == GemPickupKind::Support) {
            for (const auto& def : SupportGemData::Gems()) opts.push_back(def.id);
            return opts;
        }

        bool wantSpirit = (m_kind == GemPickupKind::Spirit);
        for (const auto& def : SkillGemData::Gems()) {
            bool isSpirit = SkillGemData::IsSpiritBehavior(def.skill.behaviorType);
            if (isSpirit == wantSpirit) opts.push_back(def.id);
        }
        return opts;
    }

    void Identify(InventoryComponent& inventory, int gemId) {
        ItemComponent& item = inventory.items[m_bagIndex];

        if (m_kind == GemPickupKind::Support) {
            const SupportGemDefinition* def = SupportGemData::Find(gemId);
            if (!def) return;
            item.skillGemIsSupport = true;
            item.skillGemId = gemId;
            item.skillGemLevel = 1; // support gems aren't leveled
            item.baseName = def->name;
            lastActionMessage = "鑑定: " + def->name;
        } else {
            const GemDefinition* def = SkillGemData::Find(gemId);
            if (!def) return;
            item.skillGemIsSupport = false;
            item.skillGemId = gemId;
            item.skillGemLevel = m_level;
            item.skillGemMaxSockets = 2;
            item.skillGemSupportIds = { -1, -1, -1, -1, -1 };
            item.baseName = def->skill.name;
            lastActionMessage = "鑑定: " + def->skill.name + " Lv" + std::to_string(m_level);
        }
        item.skillGemIdentified = true;
        messageTimer = 2.5f;
        isOpen = false;
    }

    void DrawText(sf::RenderTarget& target, float x, float y, const std::string& str, unsigned int size, sf::Color color) {
        sf::Text text(*m_font, sf::String::fromUtf8(str.begin(), str.end()), size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }

    // Clips str to fit within maxWidth pixels at the given font size, appending "...".
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
};
