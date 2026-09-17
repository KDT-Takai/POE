#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include "../../Registry/Registry.h"
#include "../../Components/Item/Item.h"
#include "../../Components/Item/Inventory.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Tags/Player/Player.h"
#include "ItemFactory.h"
#include "../UI/ItemUIHelpers.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"

// A separate NPC from the general item Vendor (see VendorSystem/ZoneBuilder): sells only
// Waystones, one tier per row, gold-only, buy-only (no Buyback -- selling one back is
// still possible through the general Vendor's bag-drag flow, since a purchased Waystone
// is a normal inventory item like any other). All 15 tiers are listed from the start;
// Tier 1 is free, and tiers beyond the player's current character level are locked (shown
// greyed out with the level requirement, never hidden -- matches this project's existing
// "always show why, never just grey out silently" convention, e.g. SkillGemSystem's
// requirement display). A purchase is rejected if the bag has no room, exactly like
// picking one up off the ground would be (see ItemPickupSystem) -- "アイテムがいっぱいの
// 時は買えない".
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

    // Tier 1 is always free -- it's the only tier available at character level 1, so
    // charging for it would gate the very first map behind gold the player has no way
    // to have earned yet.
    static int PriceForTier(int tier) { return tier <= 1 ? 0 : 20 + tier * 20; }

    // Custom early curve (tier2 unlocks at Lv5, tier3 at Lv15, per user request), then a
    // gentler linear ramp out to tier15 at Lv99 so the whole 1-15 range stays reachable
    // within this project's compressed level range instead of accelerating forever.
    static int RequiredLevelForTier(int tier) {
        static constexpr int kTable[15] = { 1, 5, 15, 22, 29, 36, 43, 50, 57, 64, 71, 78, 85, 92, 99 };
        int idx = std::clamp(tier, 1, 15) - 1;
        return kTable[idx];
    }

    static int MaxTierForLevel(int level) {
        int maxTier = 1;
        for (int tier = 1; tier <= kMaxWaystoneTier; ++tier) {
            if (level >= RequiredLevelForTier(tier)) maxTier = tier;
        }
        return maxTier;
    }

    bool IsPointInPanel(sf::Vector2f point, sf::Vector2u winSize) const {
        if (!isOpen) return false;
        sf::FloatRect panel = PanelRect(winSize);
        return panel.contains(point);
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, InventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        if (!mouseInput.IsGetMouse(sf::Mouse::Button::Left)) return;

        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;
        sf::Vector2f mouse = mouseInput.GetMousePointF();
        sf::FloatRect panel = PanelRect(window->getSize());

        sf::FloatRect closeBtn = CloseButtonRect(panel);
        if (closeBtn.contains(mouse)) { Close(); return; }

        for (int tier = 1; tier <= kMaxWaystoneTier; ++tier) {
            if (RowRect(panel, tier).contains(mouse)) {
                TryBuy(inventory, stats, tier);
                return;
            }
        }
    }

    // Central gate shared by TryBuy (Update) and Render's grey-out -- so the row can
    // never be shown as buyable while the click handler would actually refuse it.
    static bool IsUnlocked(int tier, int playerLevel) { return tier <= MaxTierForLevel(playerLevel); }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, InventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        const auto& inventory = registry.GetComponent<InventoryComponent>(player);
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
        int maxTier = MaxTierForLevel(stats.level);
        for (int tier = 1; tier <= kMaxWaystoneTier; ++tier) {
            sf::FloatRect rowRect = RowRect(panel, tier);
            bool hovered = rowRect.contains(mouse);
            int price = PriceForTier(tier);
            bool unlocked = tier <= maxTier;
            bool canAfford = unlocked && stats.gold >= price;
            int owned = CountOwned(inventory, tier);

            sf::RectangleShape row(rowRect.size);
            row.setPosition(rowRect.position);
            row.setFillColor(hovered ? sf::Color(50, 45, 30) : sf::Color(30, 28, 22));
            row.setOutlineColor(canAfford ? sf::Color(200, 160, 90) : sf::Color(90, 80, 70));
            row.setOutlineThickness(hovered ? 2.0f : 1.0f);
            target.draw(row);

            std::string label = "Tier " + std::to_string(tier) + "  -  " + (price == 0 ? std::string("Free") : std::to_string(price) + "g")
                + "  (owned: " + std::to_string(owned) + ")";
            if (!unlocked) label += "  [Requires Lv" + std::to_string(RequiredLevelForTier(tier)) + "]";
            DrawText(target, rowRect.position.x + 8.0f, rowRect.position.y + 6.0f, label, 13,
                !unlocked ? sf::Color(150, 90, 90) : (canAfford ? sf::Color(220, 220, 220) : sf::Color(150, 120, 100)));
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

    static int CountOwned(const InventoryComponent& inventory, int tier) {
        int count = 0;
        for (const auto& item : inventory.items) {
            if (item.category == ItemCategory::Waystone && item.waystoneTier == tier) count++;
        }
        return count;
    }

    void TryBuy(InventoryComponent& inventory, CharacterStatsComponent& stats, int tier) {
        if (!IsUnlocked(tier, stats.level)) {
            lastActionMessage = "Requires level " + std::to_string(RequiredLevelForTier(tier));
            messageTimer = 2.0f;
            return;
        }
        int price = PriceForTier(tier);
        if (stats.gold < price) {
            lastActionMessage = "Not enough gold";
            messageTimer = 2.0f;
            return;
        }

        static std::random_device rd;
        static std::mt19937 rng(rd());
        ItemComponent waystone = ItemFactory::GenerateWaystone(tier, rng);
        int col, row;
        if (!ItemUIHelpers::FindBagFreeSpace(inventory.items, 1, 1, col, row)) {
            lastActionMessage = "Inventory full";
            messageTimer = 2.0f;
            return;
        }

        stats.gold -= price;
        waystone.gridCol = col;
        waystone.gridRow = row;
        inventory.items.push_back(waystone);
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
