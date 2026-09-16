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
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"

// PoE2-style vendor trade window: the vendor's panel is on the left (Buy tab = their
// stock; Buyback tab = items you've sold this visit, reclaimable for exactly what you
// were paid), and the player's own bag is on the right. Selling is done by dragging a
// bag item onto the left panel, or Ctrl+left-clicking it -- never from the field
// inventory screen (that always requires a vendor visit, matching real PoE2).
class VendorSystem {
private:
    enum class VendorTab { Buy, Buyback };

    struct BuybackEntry {
        ItemComponent item;
        int price = 0; // exactly what the player was paid when selling it
    };

    static constexpr int kMaxBuyback = 12; // matches PoE's buyback tab size

    static constexpr float kPanelW = 900.0f;
    static constexpr float kPanelH = 660.0f;

    static constexpr float kLeftW = 420.0f;
    static constexpr float kRightW = 420.0f;
    static constexpr float kRightGap = 20.0f; // gap between the two columns

    static constexpr int kBagCols = 6;
    static constexpr int kBagRows = 4;
    static constexpr float kCellSize = 60.0f;
    static constexpr float kCellGap = 6.0f;

    std::shared_ptr<sf::Font> m_font;
    std::vector<ItemComponent> m_stock;
    std::vector<BuybackEntry> m_buyback;
    bool m_stockGenerated = false;
    int m_selectedIndex = 0;
    VendorTab m_activeTab = VendorTab::Buy;

    // Drag state: only the player's bag (right column) can be dragged, and only onto
    // the left column (which always means "sell").
    bool m_dragging = false;
    int m_dragBagIndex = -1;
    ItemComponent m_dragItemCache;

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    VendorSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Open(int playerLevel) {
        if (!m_stockGenerated) GenerateStock(playerLevel);
        isOpen = true;
        m_selectedIndex = 0;
        m_activeTab = VendorTab::Buy;
        m_dragging = false;
    }

    void Close() { isOpen = false; m_dragging = false; }

    void Toggle(int playerLevel) {
        if (isOpen) Close();
        else Open(playerLevel);
    }

    static int BuyPrice(const ItemComponent& item) {
        return ItemFactory::SellValue(item) * 3;
    }

    static int RerollPrice(int playerLevel) {
        return 20 + playerLevel * 4;
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, InventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);
        auto& inventory = registry.GetComponent<InventoryComponent>(player);

        auto& keyInput = InputManager::Instance().GetKeyInput();
        auto& mouseInput = InputManager::Instance().GetMouseInput();
        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;
        Layout layout = ComputeLayout(window->getSize());
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        if (keyInput.IsGetKey(KeyBindings::Instance().Get(GameAction::VendorReroll))) {
            TryReroll(stats);
        }

        // Dragging a bag item onto the vendor's (left) panel sells it.
        if (m_dragging) {
            if (!mouseInput.GetMouse(sf::Mouse::Button::Left)) {
                sf::FloatRect leftRect({ layout.panelX + 20.0f, layout.panelY }, { kLeftW, kPanelH });
                if (leftRect.contains(mouse)) {
                    SellFromBag(m_dragBagIndex, inventory, stats);
                }
                m_dragging = false;
            }
            return;
        }

        int bagHover = HitTestBagCell(layout, mouse);
        bool bagHoverHasItem = bagHover >= 0 && bagHover < static_cast<int>(inventory.items.size());
        bool ctrlHeld = keyInput.GetKey(sf::Keyboard::Key::LControl) || keyInput.GetKey(sf::Keyboard::Key::RControl);

        if (mouseInput.IsGetMouse(sf::Mouse::Button::Left)) {
            if (layout.closeBtnRect.contains(mouse)) { Close(); return; }
            if (bagHoverHasItem && ctrlHeld) {
                SellFromBag(bagHover, inventory, stats);
                return;
            }
            if (bagHoverHasItem) {
                m_dragging = true;
                m_dragBagIndex = bagHover;
                m_dragItemCache = inventory.items[bagHover];
                return;
            }

            // Left panel: tab headers, then the active list.
            if (layout.buyTabRect.contains(mouse)) { m_activeTab = VendorTab::Buy; m_selectedIndex = 0; return; }
            if (layout.buybackTabRect.contains(mouse)) { m_activeTab = VendorTab::Buyback; m_selectedIndex = 0; return; }

            int row = HitTestListRow(layout, mouse);
            if (row >= 0) {
                m_selectedIndex = row;
                if (m_activeTab == VendorTab::Buy) TryBuy(inventory, stats);
                else TryBuyback(inventory, stats);
            }
        }

        // Keyboard remains available too (Vendor pauses the world, so there's no
        // movement-key conflict here unlike the field Inventory/SkillGem screens).
        if (keyInput.IsGetKey(sf::Keyboard::Key::Left) || keyInput.IsGetKey(sf::Keyboard::Key::Right)) {
            m_activeTab = (m_activeTab == VendorTab::Buy) ? VendorTab::Buyback : VendorTab::Buy;
            m_selectedIndex = 0;
        }

        int count = (m_activeTab == VendorTab::Buy) ? static_cast<int>(m_stock.size()) : static_cast<int>(m_buyback.size());
        if (count == 0) return;
        m_selectedIndex = std::clamp(m_selectedIndex, 0, count - 1);

        if (keyInput.IsGetKey(sf::Keyboard::Key::Down)) m_selectedIndex = (m_selectedIndex + 1) % count;
        if (keyInput.IsGetKey(sf::Keyboard::Key::Up)) m_selectedIndex = (m_selectedIndex - 1 + count) % count;

        if (keyInput.IsGetKey(sf::Keyboard::Key::Enter)) {
            if (m_activeTab == VendorTab::Buy) TryBuy(inventory, stats);
            else TryBuyback(inventory, stats);
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, InventoryComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];
        const auto& playerStats = registry.GetComponent<CharacterStatsComponent>(player);
        const auto& inventory = registry.GetComponent<InventoryComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        Layout layout = ComputeLayout(target.getSize());
        float panelX = layout.panelX;
        float panelY = layout.panelY;

        sf::RectangleShape bg({ kPanelW, kPanelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        std::string rerollKeyName = KeyToString(KeyBindings::Instance().Get(GameAction::VendorReroll));
        DrawText(target, panelX + 20.0f, panelY + 12.0f,
            "Vendor - drag or Ctrl+click your items to sell, click a listing to buy/reclaim, " + rerollKeyName + " reroll stock",
            13, sf::Color(255, 220, 120));
        DrawText(target, panelX + 20.0f, panelY + 30.0f,
            "Gold: " + std::to_string(playerStats.gold) + "   Reroll cost: " + std::to_string(RerollPrice(playerStats.level)) + "g",
            13, sf::Color(255, 215, 90));

        sf::RectangleShape closeBtn(layout.closeBtnRect.size);
        closeBtn.setPosition(layout.closeBtnRect.position);
        bool closeHovered = layout.closeBtnRect.contains(InputManager::Instance().GetMouseInput().GetMousePointF());
        closeBtn.setFillColor(closeHovered ? sf::Color(140, 50, 50) : sf::Color(90, 35, 35));
        closeBtn.setOutlineColor(sf::Color(200, 150, 150));
        closeBtn.setOutlineThickness(1.0f);
        target.draw(closeBtn);
        DrawText(target, layout.closeBtnRect.position.x + 18.0f, layout.closeBtnRect.position.y + 5.0f, "Close", 13, sf::Color::White);

        // --- Left column: vendor panel (Buy / Buyback tabs) ---
        bool buyActive = m_activeTab == VendorTab::Buy;
        sf::RectangleShape buyTab(layout.buyTabRect.size);
        buyTab.setPosition(layout.buyTabRect.position);
        buyTab.setFillColor(buyActive ? sf::Color(70, 70, 110) : sf::Color(35, 35, 40));
        buyTab.setOutlineColor(sf::Color(150, 150, 160));
        buyTab.setOutlineThickness(1.0f);
        target.draw(buyTab);
        DrawText(target, layout.buyTabRect.position.x + 10.0f, layout.buyTabRect.position.y + 5.0f, "Buy", 13, sf::Color::White);

        sf::RectangleShape buybackTab(layout.buybackTabRect.size);
        buybackTab.setPosition(layout.buybackTabRect.position);
        buybackTab.setFillColor(!buyActive ? sf::Color(70, 70, 110) : sf::Color(35, 35, 40));
        buybackTab.setOutlineColor(sf::Color(150, 150, 160));
        buybackTab.setOutlineThickness(1.0f);
        target.draw(buybackTab);
        DrawText(target, layout.buybackTabRect.position.x + 10.0f, layout.buybackTabRect.position.y + 5.0f,
            "Buyback (" + std::to_string(m_buyback.size()) + ")", 13, sf::Color::White);

        if (buyActive) {
            DrawBuyList(target, layout, "(sold out - press " + rerollKeyName + " to reroll)");
        } else {
            DrawBuybackList(target, layout, "(nothing sold yet)");
        }

        // --- Right column: player's bag ---
        DrawText(target, layout.rightX, panelY + 62.0f, "Your Bag", 14, sf::Color(220, 220, 220));
        for (int i = 0; i < kBagCols * kBagRows; ++i) {
            sf::FloatRect rect = BagCellRect(layout, i);
            bool hasItem = i < static_cast<int>(inventory.items.size());
            bool draggingAway = m_dragging && i == m_dragBagIndex;
            bool hovered = rect.contains(InputManager::Instance().GetMouseInput().GetMousePointF());

            sf::RectangleShape box(rect.size);
            box.setPosition(rect.position);
            sf::Color fill = sf::Color(25, 25, 30);
            if (hasItem) {
                sf::Color rc = ItemUIHelpers::RarityColor(inventory.items[i].rarity);
                fill = sf::Color(rc.r / 4, rc.g / 4, rc.b / 4);
            }
            if (draggingAway) fill = sf::Color(20, 20, 24);
            box.setFillColor(hovered && hasItem ? sf::Color((std::min)(255, fill.r + 25), (std::min)(255, fill.g + 25), (std::min)(255, fill.b + 25)) : fill);
            box.setOutlineColor(hovered && hasItem ? sf::Color::Yellow : sf::Color(90, 90, 100));
            box.setOutlineThickness(hovered && hasItem ? 2.0f : 1.0f);
            target.draw(box);

            if (hasItem && !draggingAway) {
                const ItemComponent& item = inventory.items[i];
                DrawText(target, rect.position.x + 3.0f, rect.position.y + 3.0f, ItemUIHelpers::SlotName(item.slot).substr(0, 2), 10, ItemUIHelpers::RarityColor(item.rarity));
                DrawText(target, rect.position.x + 3.0f, rect.position.y + rect.size.y - 14.0f, "Lv" + std::to_string(item.itemLevel), 9, sf::Color(190, 190, 190));
            }
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + kPanelH - 22.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        // Drag ghost
        if (m_dragging) {
            sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();
            float gw = 70.0f, gh = 36.0f;
            sf::RectangleShape ghost({ gw, gh });
            ghost.setPosition({ mouse.x - gw / 2.0f, mouse.y - gh / 2.0f });
            sf::Color rc = ItemUIHelpers::RarityColor(m_dragItemCache.rarity);
            ghost.setFillColor(sf::Color(rc.r, rc.g, rc.b, 160));
            ghost.setOutlineColor(sf::Color::White);
            ghost.setOutlineThickness(2.0f);
            target.draw(ghost);
            DrawText(target, mouse.x - gw / 2.0f + 4.0f, mouse.y - gh / 2.0f + 4.0f, m_dragItemCache.baseName.substr(0, 8), 11, sf::Color::Black);

            sf::FloatRect leftRect({ panelX + 20.0f, panelY }, { kLeftW, kPanelH });
            if (leftRect.contains(mouse)) {
                DrawText(target, mouse.x + gw / 2.0f + 6.0f, mouse.y - 6.0f, "Sell", 12, sf::Color(120, 255, 120));
            }
        }

        target.setView(oldView);
    }

private:
    struct Layout {
        float panelX = 0.0f, panelY = 0.0f;
        float rightX = 0.0f;
        sf::FloatRect buyTabRect;
        sf::FloatRect buybackTabRect;
        sf::FloatRect closeBtnRect;
        float listY = 0.0f;
        float lineHeight = 20.0f;
    };

    Layout ComputeLayout(sf::Vector2u winSize) const {
        Layout layout;
        layout.panelX = (static_cast<float>(winSize.x) - kPanelW) / 2.0f;
        layout.panelY = (static_cast<float>(winSize.y) - kPanelH) / 2.0f;
        layout.rightX = layout.panelX + 20.0f + kLeftW + kRightGap;
        layout.buyTabRect = sf::FloatRect({ layout.panelX + 20.0f, layout.panelY + 56.0f }, { 90.0f, 24.0f });
        layout.buybackTabRect = sf::FloatRect({ layout.panelX + 118.0f, layout.panelY + 56.0f }, { 140.0f, 24.0f });
        layout.closeBtnRect = sf::FloatRect({ layout.panelX + kPanelW - 90.0f, layout.panelY + 10.0f }, { 70.0f, 24.0f });
        layout.listY = layout.panelY + 90.0f;
        layout.lineHeight = 20.0f;
        return layout;
    }

    sf::FloatRect BagCellRect(const Layout& layout, int index) const {
        int col = index % kBagCols;
        int row = index / kBagCols;
        float x = layout.rightX + static_cast<float>(col) * (kCellSize + kCellGap);
        float y = layout.panelY + 90.0f + static_cast<float>(row) * (kCellSize + kCellGap);
        return sf::FloatRect({ x, y }, { kCellSize, kCellSize });
    }

    int HitTestBagCell(const Layout& layout, sf::Vector2f mouse) const {
        for (int i = 0; i < kBagCols * kBagRows; ++i) {
            if (BagCellRect(layout, i).contains(mouse)) return i;
        }
        return -1;
    }

    int HitTestListRow(const Layout& layout, sf::Vector2f mouse) const {
        int count = (m_activeTab == VendorTab::Buy) ? static_cast<int>(m_stock.size()) : static_cast<int>(m_buyback.size());
        for (int i = 0; i < count; ++i) {
            float y = layout.listY + static_cast<float>(i) * layout.lineHeight;
            sf::FloatRect rowRect({ layout.panelX + 20.0f, y }, { kLeftW, layout.lineHeight });
            if (rowRect.contains(mouse)) return i;
        }
        return -1;
    }

    void DrawBuyList(sf::RenderTarget& target, const Layout& layout, const std::string& emptyMessage) {
        if (m_stock.empty()) {
            DrawText(target, layout.panelX + 20.0f, layout.listY, emptyMessage, 13, sf::Color(150, 150, 150));
            return;
        }
        for (size_t i = 0; i < m_stock.size(); ++i) {
            const ItemComponent& item = m_stock[i];
            float y = layout.listY + static_cast<float>(i) * layout.lineHeight;
            DrawListRow(target, layout, y, static_cast<int>(i), item, BuyPrice(item));
        }
    }

    void DrawBuybackList(sf::RenderTarget& target, const Layout& layout, const std::string& emptyMessage) {
        if (m_buyback.empty()) {
            DrawText(target, layout.panelX + 20.0f, layout.listY, emptyMessage, 13, sf::Color(150, 150, 150));
            return;
        }
        for (size_t i = 0; i < m_buyback.size(); ++i) {
            const BuybackEntry& entry = m_buyback[i];
            float y = layout.listY + static_cast<float>(i) * layout.lineHeight;
            DrawListRow(target, layout, y, static_cast<int>(i), entry.item, entry.price);
        }
    }

    void DrawListRow(sf::RenderTarget& target, const Layout& layout, float y, int index, const ItemComponent& item, int price) {
        bool selected = (index == m_selectedIndex);
        if (selected) {
            sf::RectangleShape highlight({ kLeftW - 4.0f, layout.lineHeight });
            highlight.setPosition({ layout.panelX + 20.0f, y });
            highlight.setFillColor(sf::Color(60, 60, 90, 180));
            target.draw(highlight);
        }

        std::string line = ItemUIHelpers::SlotName(item.slot) + ": " + item.baseName + " (" +
            ItemUIHelpers::RarityName(item.rarity) + ", iLvl " + std::to_string(item.itemLevel) + ", " +
            std::to_string(item.affixes.size()) + " mods) - " + std::to_string(price) + "g";

        DrawText(target, layout.panelX + 24.0f, y + 2.0f, line, 12, ItemUIHelpers::RarityColor(item.rarity));
    }

    void GenerateStock(int playerLevel) {
        m_stock.clear();
        static std::random_device rd;
        static std::mt19937 rng(rd());
        int itemLevel = std::clamp(playerLevel, 1, 100);

        for (int i = 0; i < 6; ++i) {
            EquipSlot slot = ItemFactory::RollSlot(rng);
            ItemRarity rarity = ItemFactory::RollRarity(rng);
            m_stock.push_back(ItemFactory::GenerateItem(slot, rarity, itemLevel, rng));
        }
        m_stockGenerated = true;
    }

    void TryBuy(InventoryComponent& inventory, CharacterStatsComponent& stats) {
        if (m_stock.empty() || m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_stock.size())) return;
        const ItemComponent& item = m_stock[m_selectedIndex];
        int price = BuyPrice(item);

        if (stats.gold < price) {
            lastActionMessage = "Not enough gold";
            messageTimer = 2.0f;
            return;
        }
        if (inventory.items.size() >= InventoryComponent::kCapacity) {
            lastActionMessage = "Inventory full";
            messageTimer = 2.0f;
            return;
        }

        stats.gold -= price;
        inventory.items.push_back(item);
        lastActionMessage = "Bought: " + item.baseName;
        messageTimer = 2.0f;

        m_stock.erase(m_stock.begin() + m_selectedIndex);
        m_selectedIndex = m_stock.empty() ? 0 : std::clamp(m_selectedIndex, 0, static_cast<int>(m_stock.size()) - 1);
    }

    // Sells a bag item into the buyback tray (a temporary hold, not gone for good) so a
    // misclick can always be undone by buying it back for the same gold.
    void SellFromBag(int bagIndex, InventoryComponent& inventory, CharacterStatsComponent& stats) {
        if (bagIndex < 0 || bagIndex >= static_cast<int>(inventory.items.size())) return;

        const ItemComponent& item = inventory.items[bagIndex];
        int goldValue = ItemFactory::SellValue(item);
        stats.gold += goldValue;

        m_buyback.push_back({ item, goldValue });
        if (m_buyback.size() > static_cast<size_t>(kMaxBuyback)) {
            m_buyback.erase(m_buyback.begin()); // drop the oldest entry once the tray is full
        }

        lastActionMessage = "Sold: " + item.baseName + " (+" + std::to_string(goldValue) + "g, buyback available)";
        messageTimer = 2.5f;

        inventory.items.erase(inventory.items.begin() + bagIndex);
    }

    void TryBuyback(InventoryComponent& inventory, CharacterStatsComponent& stats) {
        if (m_buyback.empty() || m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_buyback.size())) return;
        const BuybackEntry& entry = m_buyback[m_selectedIndex];

        if (stats.gold < entry.price) {
            lastActionMessage = "Not enough gold to buy back";
            messageTimer = 2.0f;
            return;
        }
        if (inventory.items.size() >= InventoryComponent::kCapacity) {
            lastActionMessage = "Inventory full";
            messageTimer = 2.0f;
            return;
        }

        stats.gold -= entry.price;
        inventory.items.push_back(entry.item);
        lastActionMessage = "Bought back: " + entry.item.baseName;
        messageTimer = 2.0f;

        m_buyback.erase(m_buyback.begin() + m_selectedIndex);
        m_selectedIndex = m_buyback.empty() ? 0 : std::clamp(m_selectedIndex, 0, static_cast<int>(m_buyback.size()) - 1);
    }

    void TryReroll(CharacterStatsComponent& stats) {
        int price = RerollPrice(stats.level);
        if (stats.gold < price) {
            lastActionMessage = "Not enough gold to reroll";
            messageTimer = 2.0f;
            return;
        }

        stats.gold -= price;
        GenerateStock(stats.level);
        m_selectedIndex = 0;
        lastActionMessage = "Stock rerolled";
        messageTimer = 2.0f;
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
