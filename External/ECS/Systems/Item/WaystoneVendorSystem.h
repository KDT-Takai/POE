#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "../../Registry/Registry.h"
#include "../../Components/Item/Waystone.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Tags/Player/Player.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"

// A separate NPC from the general item Vendor (see VendorSystem/ZoneBuilder): sells only
// Waystones, one tier per row, gold-only, buy-only (no Buyback/no accepting Waystones
// back -- the general Vendor's bag-drag sell flow doesn't apply here since Waystones
// aren't ItemComponents, just per-tier counts on WaystoneInventoryComponent). All 15
// tiers are listed from the start; high tiers are gated by gold cost rather than an
// unlock system, matching this project's "endgame loop only" simplification.
class WaystoneVendorSystem {
private:
    std::shared_ptr<sf::Font> m_font;

    static constexpr float kPanelW = 380.0f;
    static constexpr float kPanelH = 560.0f;
    static constexpr float kRowH = 30.0f;
    static constexpr float kRowY0 = 70.0f;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    WaystoneVendorSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Open() { isOpen = true; }
    void Close() { isOpen = false; }
    void Toggle() { isOpen = !isOpen; }

    static int PriceForTier(int tier) { return 20 + tier * 20; }

    bool IsPointInPanel(sf::Vector2f point, sf::Vector2u winSize) const {
        if (!isOpen) return false;
        sf::FloatRect panel = PanelRect(winSize);
        return panel.contains(point);
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, WaystoneInventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& waystones = registry.GetComponent<WaystoneInventoryComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;

        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();
        sf::FloatRect panel = PanelRect(window->getSize());

        sf::FloatRect closeBtn = CloseButtonRect(panel);
        if (closeBtn.contains(mouse)) { Close(); return; }

        for (int tier = 1; tier <= WaystoneInventoryComponent::kMaxTier; ++tier) {
            if (RowRect(panel, tier).contains(mouse)) {
                TryBuy(waystones, stats, tier);
                return;
            }
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, WaystoneInventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        const auto& waystones = registry.GetComponent<WaystoneInventoryComponent>(player);
        const auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::FloatRect panel = PanelRect(target.getSize());

        sf::RectangleShape bg(panel.size);
        bg.setPosition(panel.position);
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(200, 160, 90));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        DrawText(target, panel.position.x + 16.0f, panel.position.y + 10.0f,
            "Waystone Vendor - Gold: " + std::to_string(stats.gold), 14, sf::Color(255, 220, 120));

        sf::FloatRect closeBtn = CloseButtonRect(panel);
        sf::RectangleShape closeBox(closeBtn.size);
        closeBox.setPosition(closeBtn.position);
        bool closeHovered = closeBtn.contains(InputManager::Instance().GetMouseInput().GetMousePointF());
        closeBox.setFillColor(closeHovered ? sf::Color(140, 50, 50) : sf::Color(90, 35, 35));
        closeBox.setOutlineColor(sf::Color(200, 150, 150));
        closeBox.setOutlineThickness(1.0f);
        target.draw(closeBox);
        DrawText(target, closeBtn.position.x + 18.0f, closeBtn.position.y + 5.0f, "Close", 13, sf::Color::White);

        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();
        for (int tier = 1; tier <= WaystoneInventoryComponent::kMaxTier; ++tier) {
            sf::FloatRect rowRect = RowRect(panel, tier);
            bool hovered = rowRect.contains(mouse);
            int price = PriceForTier(tier);
            bool canAfford = stats.gold >= price;

            sf::RectangleShape row(rowRect.size);
            row.setPosition(rowRect.position);
            row.setFillColor(hovered ? sf::Color(50, 45, 30) : sf::Color(30, 28, 22));
            row.setOutlineColor(canAfford ? sf::Color(200, 160, 90) : sf::Color(90, 80, 70));
            row.setOutlineThickness(hovered ? 2.0f : 1.0f);
            target.draw(row);

            std::string label = "Tier " + std::to_string(tier) + "  -  " + std::to_string(price) + "g  (owned: "
                + std::to_string(waystones.counts[tier - 1]) + ")";
            DrawText(target, rowRect.position.x + 8.0f, rowRect.position.y + 6.0f, label, 13,
                canAfford ? sf::Color(220, 220, 220) : sf::Color(150, 120, 100));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panel.position.x + 16.0f, panel.position.y + panel.size.y - 26.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    sf::FloatRect PanelRect(sf::Vector2u winSize) const {
        float x = (static_cast<float>(winSize.x) - kPanelW) / 2.0f;
        float y = (static_cast<float>(winSize.y) - kPanelH) / 2.0f;
        return sf::FloatRect({ x, y }, { kPanelW, kPanelH });
    }

    sf::FloatRect CloseButtonRect(sf::FloatRect panel) const {
        return sf::FloatRect({ panel.position.x + panel.size.x - 90.0f, panel.position.y + 10.0f }, { 70.0f, 24.0f });
    }

    sf::FloatRect RowRect(sf::FloatRect panel, int tier) const {
        float y = panel.position.y + kRowY0 + static_cast<float>(tier - 1) * kRowH;
        return sf::FloatRect({ panel.position.x + 12.0f, y }, { panel.size.x - 24.0f, kRowH - 3.0f });
    }

    void TryBuy(WaystoneInventoryComponent& waystones, CharacterStatsComponent& stats, int tier) {
        int price = PriceForTier(tier);
        if (stats.gold < price) {
            lastActionMessage = "Not enough gold";
            messageTimer = 2.0f;
            return;
        }
        stats.gold -= price;
        waystones.counts[tier - 1]++;
        lastActionMessage = "Bought Tier " + std::to_string(tier) + " Waystone";
        messageTimer = 2.0f;
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
