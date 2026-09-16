#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "../../Registry/Registry.h"
#include "../../Components/Item/SkillGem.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Skill/SkillGemData.h"
#include "../Skill/SkillGemScaling.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"

// Uncut gems (SkillGemPickupComponent) drop with only a rolled level and Skill/Spirit
// category, not a specific skill -- matches PoE2's "identify by choosing" gem drops
// instead of PoE1's "drops already named". Clicking one on the ground (see
// ItemPickupSystem) opens this panel instead of instantly unlocking anything; picking a
// candidate here either adds a new SkillGemInventoryComponent::OwnedGemInstance or, if
// already owned, raises that instance's level (never lowers it). A "Cancel" option
// leaves the pickup on the ground so the player can decide later.
class GemIdentifySystem {
private:
    std::shared_ptr<sf::Font> m_font;
    int m_pendingLevel = 1;
    bool m_pendingIsSpirit = false;
    Entity m_pendingPickupEntity = static_cast<Entity>(-1);

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    GemIdentifySystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Open(int level, bool isSpirit, Entity pickupEntity) {
        isOpen = true;
        m_pendingLevel = level;
        m_pendingIsSpirit = isSpirit;
        m_pendingPickupEntity = pickupEntity;
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

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        Layout L = ComputeLayout();
        std::vector<int> opts = Options(gemInventory);
        for (size_t i = 0; i < opts.size(); ++i) {
            sf::FloatRect rect({ kPanelX + 12.0f, L.rowY + static_cast<float>(i) * L.rowHeight }, { kPanelW - 24.0f, L.rowHeight });
            if (rect.contains(mouse)) {
                Identify(registry, gemInventory, opts[i]);
                return;
            }
        }

        if (L.CancelRect().contains(mouse)) {
            Close(); // pickup entity is left alone -- still sitting on the ground
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, SkillGemInventoryComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::RectangleShape bg({ kPanelW, kPanelH });
        bg.setPosition({ kPanelX, kPanelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(200, 160, 220));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        std::string kindLabel = m_pendingIsSpirit ? "Spirit Gem" : "Skill Gem";
        DrawText(target, kPanelX + 12.0f, kPanelY + 8.0f,
            "Uncut " + kindLabel + ", Level " + std::to_string(m_pendingLevel) + " - choose which gem:", 13, sf::Color(255, 220, 120));

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

            const GemDefinition* def = SkillGemData::Find(gemId);
            if (!def) continue;

            const OwnedGemInstance* existing = FindOwned(gemInventory, gemId);
            std::string label = def->skill.name;
            if (existing) label += " (upgrade Lv" + std::to_string(existing->level) + " -> " + std::to_string(m_pendingLevel) + ")";

            DrawText(target, kPanelX + 16.0f, y + 4.0f, label, 13, sf::Color(220, 220, 255));
        }

        sf::FloatRect cancelRect = L.CancelRect();
        sf::RectangleShape cancelBox(cancelRect.size);
        cancelBox.setPosition(cancelRect.position);
        cancelBox.setFillColor(sf::Color(90, 50, 50));
        cancelBox.setOutlineColor(sf::Color(200, 200, 200));
        cancelBox.setOutlineThickness(1.5f);
        target.draw(cancelBox);
        DrawText(target, cancelRect.position.x + 10.0f, cancelRect.position.y + 8.0f, "Leave on ground", 13, sf::Color::White);

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

    const OwnedGemInstance* FindOwned(const SkillGemInventoryComponent& inv, int gemId) const {
        return SkillGemScaling::FindOwnedGem(inv, gemId, false);
    }

    // Every gem of the rolled category (Skill vs Spirit) that isn't already owned at
    // this level or higher -- picking an already-superior gem would be a no-op.
    std::vector<int> Options(const SkillGemInventoryComponent& gemInventory) const {
        std::vector<int> opts;
        for (const auto& def : SkillGemData::Gems()) {
            bool isAura = (def.skill.behaviorType == SkillBehaviorType::Aura);
            if (isAura != m_pendingIsSpirit) continue;

            const OwnedGemInstance* existing = FindOwned(gemInventory, def.id);
            if (existing && existing->level >= m_pendingLevel) continue;

            opts.push_back(def.id);
        }
        return opts;
    }

    void Identify(Registry& registry, SkillGemInventoryComponent& gemInventory, int gemId) {
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
        messageTimer = 2.5f;

        if (registry.IsValid(m_pendingPickupEntity)) {
            registry.DestroyEntity(m_pendingPickupEntity);
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
