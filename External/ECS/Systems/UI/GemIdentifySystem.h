#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "../../Registry/Registry.h"
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Skill/SkillGemData.h"
#include "../Skill/SupportGemData.h"
#include "../Skill/SkillGemScaling.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"

// Gem Cutting screen. Uncut gems (Skill/Support/Spirit, see GemPickupKind) drop with only
// a rolled level and kind, not a specific gem -- matches the spec's "拾う -> インベントリ
// -> 使用 -> Gem Cutting -> 生成" flow. Held pickups sit in
// SkillGemInventoryComponent::pendingUncutGems (a lightweight stand-in for an inventory
// slot) until the player explicitly "uses" one from SkillGemSystem's Uncut Gems row,
// which calls Open() here with that entry's queue index. Picking a candidate either adds
// a new OwnedGemInstance or, for Skill/Spirit, raises an already-owned instance's level
// (never lowers it) -- Support gems have no level, so an already-owned Support is simply
// excluded from the candidate list. A "Cancel" option leaves the entry in the queue.
class GemIdentifySystem {
private:
    std::shared_ptr<sf::Font> m_font;
    int m_pendingIndex = -1;
    int m_pendingLevel = 1;
    GemPickupKind m_pendingKind = GemPickupKind::Skill;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    GemIdentifySystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Open(int pendingIndex, int level, GemPickupKind kind) {
        isOpen = true;
        m_pendingIndex = pendingIndex;
        m_pendingLevel = level;
        m_pendingKind = kind;
    }
    void Close() { isOpen = false; }

    bool IsPointInPanel(sf::Vector2f point) const {
        if (!isOpen) return false;
        return sf::FloatRect({ kPanelX, kPanelY }, { kPanelW, kPanelH }).contains(point);
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, SkillGemInventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);
        if (!PendingEntryStillValid(gemInventory)) { isOpen = false; return; }

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        Layout L = ComputeLayout();
        std::vector<int> opts = Options(gemInventory);
        for (size_t i = 0; i < opts.size(); ++i) {
            sf::FloatRect rect({ kPanelX + 12.0f, L.rowY + static_cast<float>(i) * L.rowHeight }, { kPanelW - 24.0f, L.rowHeight });
            if (rect.contains(mouse)) {
                Identify(gemInventory, opts[i]);
                return;
            }
        }

        if (L.CancelRect().contains(mouse)) {
            Close(); // entry stays in the pending queue -- player can cut it later
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, SkillGemInventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);
        if (!PendingEntryStillValid(gemInventory)) return;

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::RectangleShape bg({ kPanelW, kPanelH });
        bg.setPosition({ kPanelX, kPanelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(200, 160, 220));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        DrawText(target, kPanelX + 12.0f, kPanelY + 8.0f,
            "Gem Cutting - Uncut " + std::string(KindLabel(m_pendingKind)) + " Gem, Level " + std::to_string(m_pendingLevel) + " - choose:",
            13, sf::Color(255, 220, 120));

        Layout L = ComputeLayout();
        std::vector<int> opts = Options(gemInventory);

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
            if (m_pendingKind == GemPickupKind::Support) {
                const SupportGemDefinition* def = SupportGemData::Find(gemId);
                if (!def) continue;
                label = def->name;
            } else {
                const GemDefinition* def = SkillGemData::Find(gemId);
                if (!def) continue;
                const OwnedGemInstance* existing = FindOwned(gemInventory, gemId);
                label = def->skill.name;
                if (existing) label += " (upgrade Lv" + std::to_string(existing->level) + " -> " + std::to_string(m_pendingLevel) + ")";
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

    // The pending queue can shrink from under this panel (e.g. save/load) -- guard every
    // access instead of assuming m_pendingIndex is still in range.
    bool PendingEntryStillValid(const SkillGemInventoryComponent& inv) const {
        return m_pendingIndex >= 0 && static_cast<size_t>(m_pendingIndex) < inv.pendingUncutGems.size();
    }

    const OwnedGemInstance* FindOwned(const SkillGemInventoryComponent& inv, int gemId) const {
        return SkillGemScaling::FindOwnedGem(inv, gemId, false);
    }

    // Candidates for the rolled kind: Skill/Spirit list SkillGemData entries (split by
    // Aura behaviorType, matching the old isSpirit convention) not already owned at this
    // level or higher; Support lists not-yet-owned SupportGemData entries (support gems
    // have no level, so an owned one has nothing left to upgrade).
    std::vector<int> Options(const SkillGemInventoryComponent& gemInventory) const {
        std::vector<int> opts;
        if (m_pendingKind == GemPickupKind::Support) {
            for (const auto& def : SupportGemData::Gems()) {
                if (SkillGemScaling::FindOwnedGem(gemInventory, def.id, true)) continue;
                opts.push_back(def.id);
            }
            return opts;
        }

        bool wantSpirit = (m_pendingKind == GemPickupKind::Spirit);
        for (const auto& def : SkillGemData::Gems()) {
            bool isSpirit = SkillGemData::IsSpiritBehavior(def.skill.behaviorType);
            if (isSpirit != wantSpirit) continue;

            const OwnedGemInstance* existing = FindOwned(gemInventory, def.id);
            if (existing && existing->level >= m_pendingLevel) continue;

            opts.push_back(def.id);
        }
        return opts;
    }

    void Identify(SkillGemInventoryComponent& gemInventory, int gemId) {
        if (m_pendingKind == GemPickupKind::Support) {
            const SupportGemDefinition* def = SupportGemData::Find(gemId);
            if (!def) return;
            if (!SkillGemScaling::FindOwnedGem(gemInventory, gemId, true)) {
                gemInventory.ownedGems.push_back(OwnedGemInstance{ gemId, true, 1, 2, { -1, -1, -1, -1, -1 } });
            }
            lastActionMessage = "Cut: " + def->name;
        } else {
            const GemDefinition* def = SkillGemData::Find(gemId);
            if (!def) return;

            OwnedGemInstance* existing = nullptr;
            for (auto& owned : gemInventory.ownedGems) {
                if (!owned.isSupport && owned.gemId == gemId) { existing = &owned; break; }
            }

            if (existing) {
                existing->level = m_pendingLevel;
                lastActionMessage = "Upgraded: " + def->skill.name + " to Lv" + std::to_string(m_pendingLevel);
            } else {
                gemInventory.ownedGems.push_back(OwnedGemInstance{ gemId, false, m_pendingLevel, 2, { -1, -1, -1, -1, -1 } });
                lastActionMessage = "Learned: " + def->skill.name + " Lv" + std::to_string(m_pendingLevel);
            }
        }
        messageTimer = 2.5f;

        if (PendingEntryStillValid(gemInventory)) {
            gemInventory.pendingUncutGems.erase(gemInventory.pendingUncutGems.begin() + m_pendingIndex);
        }
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
