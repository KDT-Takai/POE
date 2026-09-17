#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
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

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        Layout L = ComputeLayout();
        std::vector<int> opts = Options();
        for (size_t i = 0; i < opts.size(); ++i) {
            sf::FloatRect rect({ kPanelX + 12.0f, L.rowY + static_cast<float>(i) * L.rowHeight }, { kPanelW - 24.0f, L.rowHeight });
            if (rect.contains(mouse)) {
                Identify(inventory, opts[i]);
                return;
            }
        }

        if (L.CancelRect().contains(mouse)) {
            Close(); // item stays uncut in the bag -- player can cut it later
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
            "Gem Cutting - Uncut " + std::string(KindLabel(m_kind)) + " Gem, Level " + std::to_string(m_level) + " - choose:",
            13, sf::Color(255, 220, 120));

        Layout L = ComputeLayout();
        std::vector<int> opts = Options();

        for (size_t i = 0; i < opts.size(); ++i) {
            int gemId = opts[i];
            float y = L.rowY + static_cast<float>(i) * L.rowHeight;

            sf::RectangleShape highlight({ kPanelW - 24.0f, L.rowHeight - 2.0f });
            highlight.setPosition({ kPanelX + 12.0f, y });
            highlight.setFillColor(sf::Color(35, 30, 40, 160));
            highlight.setOutlineColor(sf::Color(90, 80, 100));
            highlight.setOutlineThickness(1.0f);
            target.draw(highlight);

            std::string label;
            if (m_kind == GemPickupKind::Support) {
                const SupportGemDefinition* def = SupportGemData::Find(gemId);
                if (!def) continue;
                label = def->name;
            } else {
                const GemDefinition* def = SkillGemData::Find(gemId);
                if (!def) continue;
                label = def->skill.name;
            }

            DrawText(target, kPanelX + 16.0f, y + 4.0f, label, 13, sf::Color(220, 220, 255));
        }

        sf::FloatRect cancelRect = L.CancelRect();
        sf::RectangleShape cancelBox(cancelRect.size);
        cancelBox.setPosition(cancelRect.position);
        cancelBox.setFillColor(sf::Color(90, 50, 50));
        cancelBox.setOutlineColor(sf::Color(200, 200, 200));
        cancelBox.setOutlineThickness(1.5f);
        target.draw(cancelBox);
        DrawText(target, cancelRect.position.x + 10.0f, cancelRect.position.y + 8.0f, "Cancel", 13, sf::Color::White);

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, kPanelX + 12.0f, cancelRect.position.y + 40.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    struct Layout {
        float rowY;
        float rowHeight;
        float cancelY;
        sf::FloatRect CancelRect() const { return sf::FloatRect({ 20.0f + 12.0f, cancelY }, { 180.0f, 32.0f }); }
    };

    Layout ComputeLayout() const {
        Layout L;
        L.rowY = kPanelY + 36.0f;
        L.rowHeight = 26.0f;
        L.cancelY = kPanelY + kPanelH - 60.0f;
        return L;
    }

    static constexpr float kPanelW = 420.0f;
    static constexpr float kPanelH = 420.0f;
    static constexpr float kPanelX = 240.0f;
    static constexpr float kPanelY = 140.0f;

    static const char* KindLabel(GemPickupKind kind) {
        switch (kind) {
        case GemPickupKind::Support: return "Support";
        case GemPickupKind::Spirit: return "Spirit";
        default: return "Skill";
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
            lastActionMessage = "Cut: " + def->name;
        } else {
            const GemDefinition* def = SkillGemData::Find(gemId);
            if (!def) return;
            item.skillGemIsSupport = false;
            item.skillGemId = gemId;
            item.skillGemLevel = m_level;
            item.skillGemMaxSockets = 2;
            item.skillGemSupportIds = { -1, -1, -1, -1, -1 };
            item.baseName = def->skill.name;
            lastActionMessage = "Cut: " + def->skill.name + " Lv" + std::to_string(m_level);
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
};
