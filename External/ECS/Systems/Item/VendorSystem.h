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

    // The vendor's own panel and the player's bag are two visually separate boxes
    // (own background/border each, like CharacterSheet/Inventory are their own boxes)
    // with a visible gap between them, rather than one continuous panel.
    static constexpr float kBoxGap = 24.0f;
    static constexpr float kLeftW = 440.0f;
    static constexpr float kRightW = 440.0f;
    static constexpr float kPanelH = 660.0f;
    static constexpr float kPanelW = kLeftW + kBoxGap + kRightW; // bounding width, for centering only

    static constexpr int kBagCols = 6;
    static constexpr int kBagRows = 4;
    static constexpr float kCellSize = 60.0f;
    static constexpr float kCellGap = 6.0f;

    std::shared_ptr<sf::Font> m_font;
    std::vector<ItemComponent> m_stock;
    std::vector<BuybackEntry> m_buyback;
    bool m_stockGenerated = false;
    int m_stockLevel = 0; // player level the current m_stock was generated at
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
        // Stock is regenerated for free whenever the player has leveled up (or down)
        // since it was last rolled, so the vendor keeps offering gear appropriate to
        // current story progress instead of staying frozen at whatever level the
        // player was when they first talked to this vendor. Paid reroll (see
        // TryReroll) is still the only way to get a fresh roll at the *same* level.
        if (!m_stockGenerated || playerLevel != m_stockLevel) GenerateStock(playerLevel);
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
        ItemUIHelpers::NormalizeBagPlacement(inventory.items);

        auto& keyInput = InputManager::Instance().GetKeyInput();
        auto& mouseInput = InputManager::Instance().GetMouseInput();
        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;
        Layout layout = ComputeLayout(window->getSize());
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        if (keyInput.IsGetKey(KeyBindings::Instance().Get(GameAction::VendorReroll))) {
            TryReroll(stats);
        }

        // Dragging a bag item onto the vendor's (left) box sells it.
        if (m_dragging) {
            if (!mouseInput.GetMouse(sf::Mouse::Button::Left)) {
                if (layout.leftBox.contains(mouse)) {
                    SellFromBag(m_dragBagIndex, inventory, stats);
                }
                m_dragging = false;
            }
            return;
        }

        int bagHover = HitTestBagItem(layout, mouse, inventory.items);
        bool bagHoverHasItem = bagHover >= 0;
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

            int hit = HitTestShopItem(layout, mouse);
            if (hit >= 0) {
                m_selectedIndex = hit;
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
        float panelY = layout.panelY;

        // Two independent boxes (own background/border each), not one continuous panel --
        // visually the same "each menu is its own box" language as CharacterSheet/Inventory.
        sf::RectangleShape leftBg(layout.leftBox.size);
        leftBg.setPosition(layout.leftBox.position);
        leftBg.setFillColor(sf::Color(15, 15, 20, 235));
        leftBg.setOutlineColor(sf::Color(150, 150, 160));
        leftBg.setOutlineThickness(2.0f);
        target.draw(leftBg);

        sf::RectangleShape rightBg(layout.rightBox.size);
        rightBg.setPosition(layout.rightBox.position);
        rightBg.setFillColor(sf::Color(15, 15, 20, 235));
        rightBg.setOutlineColor(sf::Color(150, 150, 160));
        rightBg.setOutlineThickness(2.0f);
        target.draw(rightBg);

        std::string rerollKeyName = KeyToString(KeyBindings::Instance().Get(GameAction::VendorReroll));
        DrawText(target, layout.leftBox.position.x + 20.0f, panelY + 12.0f,
            "Vendor - drag or Ctrl+click your items to sell, click a listing to buy/reclaim", 12, sf::Color(255, 220, 120));
        DrawText(target, layout.leftBox.position.x + 20.0f, panelY + 30.0f,
            "Gold: " + std::to_string(playerStats.gold) + "   Reroll (" + rerollKeyName + "): " + std::to_string(RerollPrice(playerStats.level)) + "g",
            12, sf::Color(255, 215, 90));

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
            DrawShopGrid(target, layout, "(sold out - press " + rerollKeyName + " to reroll)");
        } else {
            DrawShopGrid(target, layout, "(nothing sold yet)");
        }

        // --- Right box: player's bag (same size-aware grid as the field InventorySystem) ---
        DrawText(target, layout.rightBox.position.x + 20.0f, panelY + 62.0f, "Your Bag", 14, sf::Color(220, 220, 220));

        for (int row = 0; row < kBagRows; ++row) {
            for (int col = 0; col < kBagCols; ++col) {
                sf::FloatRect rect = BagCellRect(layout, col, row);
                sf::RectangleShape box(rect.size);
                box.setPosition(rect.position);
                box.setFillColor(sf::Color(25, 25, 30));
                box.setOutlineColor(sf::Color(70, 70, 78));
                box.setOutlineThickness(1.0f);
                target.draw(box);
            }
        }

        sf::Vector2f bagMouse = InputManager::Instance().GetMouseInput().GetMousePointF();
        for (size_t i = 0; i < inventory.items.size(); ++i) {
            const ItemComponent& item = inventory.items[i];
            if (item.gridCol < 0 || item.gridRow < 0) continue;
            bool draggingAway = m_dragging && static_cast<int>(i) == m_dragBagIndex;
            if (draggingAway) continue;

            sf::FloatRect rect = BagItemRect(layout, item);
            bool hovered = rect.contains(bagMouse);
            sf::Color rc = ItemUIHelpers::RarityColor(item.rarity);
            sf::Color fill(rc.r / 4, rc.g / 4, rc.b / 4);

            sf::RectangleShape box(rect.size);
            box.setPosition(rect.position);
            box.setFillColor(hovered ? sf::Color((std::min)(255, fill.r + 25), (std::min)(255, fill.g + 25), (std::min)(255, fill.b + 25)) : fill);
            box.setOutlineColor(hovered ? sf::Color::Yellow : rc);
            box.setOutlineThickness(hovered ? 2.0f : 1.5f);
            target.draw(box);

            DrawText(target, rect.position.x + 3.0f, rect.position.y + 3.0f, ItemUIHelpers::SlotName(item.slot).substr(0, 2), 10, rc);
            DrawText(target, rect.position.x + 3.0f, rect.position.y + rect.size.y - 14.0f, "Lv" + std::to_string(item.itemLevel), 9, sf::Color(190, 190, 190));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, layout.leftBox.position.x + 20.0f, panelY + kPanelH - 22.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
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

            if (layout.leftBox.contains(mouse)) {
                DrawText(target, mouse.x + gw / 2.0f + 6.0f, mouse.y - 6.0f, "Sell", 12, sf::Color(120, 255, 120));
            }
        }

        target.setView(oldView);
    }

private:
    struct Layout {
        float panelX = 0.0f, panelY = 0.0f;
        sf::FloatRect leftBox;
        sf::FloatRect rightBox;
        sf::FloatRect buyTabRect;
        sf::FloatRect buybackTabRect;
        sf::FloatRect closeBtnRect;
        float listY = 0.0f;
    };

    Layout ComputeLayout(sf::Vector2u winSize) const {
        Layout layout;
        layout.panelX = (static_cast<float>(winSize.x) - kPanelW) / 2.0f;
        layout.panelY = (static_cast<float>(winSize.y) - kPanelH) / 2.0f;
        layout.leftBox = sf::FloatRect({ layout.panelX, layout.panelY }, { kLeftW, kPanelH });
        layout.rightBox = sf::FloatRect({ layout.panelX + kLeftW + kBoxGap, layout.panelY }, { kRightW, kPanelH });
        layout.buyTabRect = sf::FloatRect({ layout.leftBox.position.x + 20.0f, layout.panelY + 56.0f }, { 90.0f, 24.0f });
        layout.buybackTabRect = sf::FloatRect({ layout.leftBox.position.x + 118.0f, layout.panelY + 56.0f }, { 140.0f, 24.0f });
        layout.closeBtnRect = sf::FloatRect({ layout.rightBox.position.x + kRightW - 90.0f, layout.panelY + 10.0f }, { 70.0f, 24.0f });
        layout.listY = layout.panelY + 90.0f;
        return layout;
    }

    // --- Player's bag (right box): same size-aware grid rendering as the field
    // InventorySystem, since both read/write the same InventoryComponent.

    sf::FloatRect BagCellRect(const Layout& layout, int col, int row) const {
        float x = layout.rightBox.position.x + 20.0f + static_cast<float>(col) * (kCellSize + kCellGap);
        float y = layout.panelY + 90.0f + static_cast<float>(row) * (kCellSize + kCellGap);
        return sf::FloatRect({ x, y }, { kCellSize, kCellSize });
    }

    sf::FloatRect BagItemRect(const Layout& layout, const ItemComponent& item) const {
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item.slot);
        sf::FloatRect topLeft = BagCellRect(layout, item.gridCol, item.gridRow);
        float w = static_cast<float>(sz.x) * kCellSize + static_cast<float>(sz.x - 1) * kCellGap;
        float h = static_cast<float>(sz.y) * kCellSize + static_cast<float>(sz.y - 1) * kCellGap;
        return sf::FloatRect(topLeft.position, { w, h });
    }

    int HitTestBagItem(const Layout& layout, sf::Vector2f mouse, const std::vector<ItemComponent>& items) const {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].gridCol < 0 || items[i].gridRow < 0) continue;
            if (BagItemRect(layout, items[i]).contains(mouse)) return static_cast<int>(i);
        }
        return -1;
    }

    // --- Vendor's own panel (left box): Buy stock / Buyback list, rendered as an icon
    // grid matching the bag's visual language (per user request: "make it look like the
    // item screen"). Neither list has a persisted layout, so it's shelf-packed fresh
    // each call from the current list order.
    static constexpr int kShopGridCols = 6;

    sf::FloatRect ShopCellRect(const Layout& layout, int col, int row) const {
        float x = layout.leftBox.position.x + 20.0f + static_cast<float>(col) * (kCellSize + kCellGap);
        float y = layout.listY + static_cast<float>(row) * (kCellSize + kCellGap);
        return sf::FloatRect({ x, y }, { kCellSize, kCellSize });
    }

    sf::FloatRect ShopItemRect(const Layout& layout, sf::Vector2i pos, sf::Vector2i sz) const {
        sf::FloatRect topLeft = ShopCellRect(layout, pos.x, pos.y);
        float w = static_cast<float>(sz.x) * kCellSize + static_cast<float>(sz.x - 1) * kCellGap;
        float h = static_cast<float>(sz.y) * kCellSize + static_cast<float>(sz.y - 1) * kCellGap;
        return sf::FloatRect(topLeft.position, { w, h });
    }

    // Slot + size for the i-th entry of whichever tab is active, so packing/hit-testing/
    // drawing all agree on the same shape without duplicating the Buy-vs-Buyback branch.
    EquipSlot ActiveTabSlot(int index) const {
        return (m_activeTab == VendorTab::Buy) ? m_stock[index].slot : m_buyback[index].item.slot;
    }

    int ActiveTabCount() const {
        return (m_activeTab == VendorTab::Buy) ? static_cast<int>(m_stock.size()) : static_cast<int>(m_buyback.size());
    }

    std::vector<sf::Vector2i> ActiveTabPositions() const {
        int count = ActiveTabCount();
        std::vector<sf::Vector2i> sizes(count);
        for (int i = 0; i < count; ++i) sizes[i] = ItemUIHelpers::ItemGridSize(ActiveTabSlot(i));
        return ItemUIHelpers::ShelfPack(sizes, kShopGridCols);
    }

    int HitTestShopItem(const Layout& layout, sf::Vector2f mouse) const {
        int count = ActiveTabCount();
        std::vector<sf::Vector2i> positions = ActiveTabPositions();
        for (int i = 0; i < count; ++i) {
            sf::Vector2i sz = ItemUIHelpers::ItemGridSize(ActiveTabSlot(i));
            if (ShopItemRect(layout, positions[i], sz).contains(mouse)) return i;
        }
        return -1;
    }

    void DrawShopGrid(sf::RenderTarget& target, const Layout& layout, const std::string& emptyMessage) {
        int count = ActiveTabCount();
        if (count == 0) {
            DrawText(target, layout.leftBox.position.x + 20.0f, layout.listY, emptyMessage, 13, sf::Color(150, 150, 150));
            return;
        }

        std::vector<sf::Vector2i> positions = ActiveTabPositions();
        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();

        for (int i = 0; i < count; ++i) {
            const ItemComponent& item = (m_activeTab == VendorTab::Buy) ? m_stock[i] : m_buyback[i].item;
            int price = (m_activeTab == VendorTab::Buy) ? BuyPrice(item) : m_buyback[i].price;
            sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item.slot);
            sf::FloatRect rect = ShopItemRect(layout, positions[i], sz);
            bool selected = (i == m_selectedIndex);
            bool hovered = rect.contains(mouse);
            sf::Color rc = ItemUIHelpers::RarityColor(item.rarity);
            sf::Color fill(rc.r / 4, rc.g / 4, rc.b / 4);

            sf::RectangleShape box(rect.size);
            box.setPosition(rect.position);
            box.setFillColor((hovered || selected) ? sf::Color((std::min)(255, fill.r + 25), (std::min)(255, fill.g + 25), (std::min)(255, fill.b + 25)) : fill);
            box.setOutlineColor(selected ? sf::Color::Yellow : rc);
            box.setOutlineThickness(selected ? 2.5f : 1.5f);
            target.draw(box);

            DrawText(target, rect.position.x + 4.0f, rect.position.y + 4.0f, ItemUIHelpers::SlotName(item.slot).substr(0, 2), 11, rc);
            DrawText(target, rect.position.x + 4.0f, rect.position.y + rect.size.y - 16.0f, "Lv" + std::to_string(item.itemLevel), 10, sf::Color(190, 190, 190));

            if (hovered) {
                DrawShopTooltip(target, mouse, item, price);
            }
        }
    }

    void DrawShopTooltip(sf::RenderTarget& target, sf::Vector2f mouse, const ItemComponent& item, int price) {
        std::string line1 = item.baseName + " (" + ItemUIHelpers::RarityName(item.rarity) + ", iLvl " + std::to_string(item.itemLevel) + ")";
        std::string line2 = ItemUIHelpers::SlotName(item.slot) + " - " + std::to_string(item.affixes.size()) + " mods - " + std::to_string(price) + "g";

        sf::Text probe1(*m_font, sf::String::fromUtf8(line1.begin(), line1.end()), 13);
        sf::Text probe2(*m_font, sf::String::fromUtf8(line2.begin(), line2.end()), 12);
        float w = (std::max)(probe1.getLocalBounds().size.x, probe2.getLocalBounds().size.x) + 20.0f;
        float h = 44.0f;

        sf::RectangleShape box({ w, h });
        box.setPosition({ mouse.x + 16.0f, mouse.y + 8.0f });
        box.setFillColor(sf::Color(10, 10, 14, 235));
        box.setOutlineColor(sf::Color(150, 150, 160));
        box.setOutlineThickness(1.5f);
        target.draw(box);

        DrawText(target, mouse.x + 26.0f, mouse.y + 14.0f, line1, 13, ItemUIHelpers::RarityColor(item.rarity));
        DrawText(target, mouse.x + 26.0f, mouse.y + 32.0f, line2, 12, sf::Color(200, 200, 200));
    }

    void GenerateStock(int playerLevel) {
        m_stock.clear();
        static std::random_device rd;
        static std::mt19937 rng(rd());
        int itemLevel = std::clamp(playerLevel, 1, 100);
        m_stockLevel = playerLevel;

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
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item.slot);
        int col, row;
        if (!ItemUIHelpers::FindBagFreeSpace(inventory.items, sz.x, sz.y, col, row)) {
            lastActionMessage = "Inventory full";
            messageTimer = 2.0f;
            return;
        }

        stats.gold -= price;
        ItemComponent placed = item;
        placed.gridCol = col;
        placed.gridRow = row;
        inventory.items.push_back(placed);
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
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(entry.item.slot);
        int col, row;
        if (!ItemUIHelpers::FindBagFreeSpace(inventory.items, sz.x, sz.y, col, row)) {
            lastActionMessage = "Inventory full";
            messageTimer = 2.0f;
            return;
        }

        stats.gold -= entry.price;
        ItemComponent placed = entry.item;
        placed.gridCol = col;
        placed.gridRow = row;
        inventory.items.push_back(placed);
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
