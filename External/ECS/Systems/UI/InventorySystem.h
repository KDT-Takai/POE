#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <sstream>
#include <algorithm>
#include <array>
#include "../../Registry/Registry.h"
#include "../../Components/Item/Item.h"
#include "../../Components/Item/Inventory.h"
#include "../../Components/Item/Equipment.h"
#include "../../Components/Item/ItemPickup.h"
#include "../../Components/Stats/CharacterStats/CharacterStats.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/Components.h"
#include "../../Components/Tags/Player/Player.h"
#include "../Item/EquipmentSystem.h"
#include "../Item/ItemFactory.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"
#include "ItemUIHelpers.h"

// PoEスタイルのアイテム画面: 左に体の部位に見立てた装備欄(ペーパードール)、
// 右に24マスの所持品グリッド。クリックで選択して下部ボタンで操作するほか、
// ドラッグ&ドロップにも対応(装備⇔バッグ間の移動、バッグ内の入れ替え、
// パネル外へドロップすると足元の地面にアイテムを捨てる)。
class InventorySystem {
private:
    static constexpr float kPanelW = 760.0f;
    static constexpr float kPanelH = 500.0f;
    static constexpr float kContentX = 20.0f;
    static constexpr float kContentY = 70.0f;

    static constexpr float kDollSlotSize = 64.0f;
    static constexpr float kDollGap = 10.0f;
    static constexpr float kDollW = kDollSlotSize * 3.0f + kDollGap * 2.0f;
    static constexpr float kDollH = kDollSlotSize * 4.0f + kDollGap * 3.0f;

    static constexpr float kGridX = kContentX + kDollW + 30.0f;
    static constexpr int kGridCols = 6;
    static constexpr int kGridRows = 4; // 6x4 = 24, InventoryComponent::kCapacityと一致
    static constexpr float kCellW = 70.0f;
    static constexpr float kCellH = 70.0f;
    static constexpr float kCellGap = 6.0f;

    static constexpr float kFooterY = kContentY + 298.0f + 10.0f; // ドール/グリッドの高い方の下
    static constexpr float kButtonW = 130.0f;
    static constexpr float kButtonH = 36.0f;
    static constexpr float kButtonGap = 20.0f;

    struct DollSlotDef {
        EquipSlot slot;
        int col;
        int row;
        const char* label;
    };
    static constexpr std::array<DollSlotDef, 9> kDollSlots = { {
        { EquipSlot::Helmet,     1, 0, "兜" },
        { EquipSlot::Weapon,     0, 1, "武器" },
        { EquipSlot::BodyArmour, 1, 1, "胴防具" },
        { EquipSlot::Amulet,     2, 1, "首飾り" },
        { EquipSlot::Gloves,     0, 2, "手袋" },
        { EquipSlot::Belt,       1, 2, "ベルト" },
        { EquipSlot::Ring1,      2, 2, "指輪" },
        { EquipSlot::Boots,      1, 3, "靴" },
        { EquipSlot::Ring2,      2, 3, "指輪" },
    } };

    struct PanelLayout {
        float panelX = 0.0f;
        float panelY = 0.0f;
        sf::FloatRect equipBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect sellBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
        sf::FloatRect discardBtn{ {0.0f, 0.0f}, {0.0f, 0.0f} };
    };

    std::shared_ptr<sf::Font> m_font;

    // 選択状態: どちらか一方のみ有効
    bool m_selectedIsEquipped = false;
    int m_selectedIndex = 0;                       // !m_selectedIsEquippedのとき有効なバッグ内index
    EquipSlot m_selectedSlot = EquipSlot::Weapon;   // m_selectedIsEquippedのとき有効な装備スロット

    // ドラッグ状態
    bool m_dragging = false;
    bool m_dragFromEquipped = false;
    int m_dragBagIndex = -1;
    EquipSlot m_dragSlot = EquipSlot::Weapon;
    ItemComponent m_dragItemCache; // ゴースト描画用のスナップショット

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    InventorySystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() {
        isOpen = !isOpen;
        m_selectedIsEquipped = false;
        m_selectedIndex = 0;
        m_dragging = false;
    }

    void Close() { isOpen = false; m_dragging = false; }

    // ワールドをポーズしないメニュー同士(インベントリ/キャラクターシート/スキルジェム)は
    // マウスがカーソル下のパネルに実際に重なっている時だけスキル発動クリックを奪うべき
    // なので、GameScene側でこの判定を使ってクリックの取り合いを解決する。
    bool IsPointInPanel(sf::Vector2f point, sf::Vector2u winSize) const {
        if (!isOpen) return false;
        PanelLayout layout = ComputeLayout(winSize);
        sf::FloatRect panelRect({ layout.panelX, layout.panelY }, { kPanelW, kPanelH });
        return panelRect.contains(point);
    }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto players = registry.View<PlayerTag, InventoryComponent, EquipmentComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];

        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        if (!m_selectedIsEquipped) {
            if (inventory.items.empty()) {
                m_selectedIndex = 0;
            } else {
                m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);

                auto& keyInput = InputManager::Instance().GetKeyInput();
                int count = static_cast<int>(inventory.items.size());

                if (keyInput.IsGetKey(sf::Keyboard::Key::Down)) {
                    m_selectedIndex = (m_selectedIndex + 1) % count;
                }
                if (keyInput.IsGetKey(sf::Keyboard::Key::Up)) {
                    m_selectedIndex = (m_selectedIndex - 1 + count) % count;
                }

                if (keyInput.IsGetKey(sf::Keyboard::Key::Enter)) {
                    EquipSelected(inventory, equipment, stats);
                } else if (keyInput.IsGetKey(sf::Keyboard::Key::X)) {
                    SellSelected(inventory, stats);
                } else if (keyInput.IsGetKey(sf::Keyboard::Key::Delete)) {
                    DiscardSelected(inventory);
                }
            }
        }

        sf::RenderWindow* window = InputManager::Instance().GetWindow();
        if (!window) return;
        PanelLayout layout = ComputeLayout(window->getSize());

        auto& mouseInput = InputManager::Instance().GetMouseInput();
        sf::Vector2f mouse = mouseInput.GetMousePointF();

        // ドラッグ中: 左ボタンが離された瞬間がこのUpdate呼び出し内で唯一の
        // 「離した」判定チャンス(m_draggingがtrueの間は毎フレームここを通る)。
        if (m_dragging) {
            if (!mouseInput.GetMouse(sf::Mouse::Button::Left)) {
                HandleDrop(registry, player, mouse, layout, inventory, equipment, stats);
                m_dragging = false;
            }
            return; // ドラッグ中はクリック操作(選択/ボタン)を行わない
        }

        if (mouseInput.IsGetMouse(sf::Mouse::Button::Left)) {
            EquipSlot clickedSlot;
            int clickedCell = HitTestGridCell(layout, mouse);
            bool onDoll = HitTestDoll(layout, mouse, clickedSlot);

            if (onDoll && equipment.slots[static_cast<size_t>(clickedSlot)].has_value()) {
                m_selectedIsEquipped = true;
                m_selectedSlot = clickedSlot;
                m_dragging = true;
                m_dragFromEquipped = true;
                m_dragSlot = clickedSlot;
                m_dragItemCache = *equipment.slots[static_cast<size_t>(clickedSlot)];
            } else if (clickedCell >= 0 && clickedCell < static_cast<int>(inventory.items.size())) {
                m_selectedIsEquipped = false;
                m_selectedIndex = clickedCell;
                m_dragging = true;
                m_dragFromEquipped = false;
                m_dragBagIndex = clickedCell;
                m_dragItemCache = inventory.items[clickedCell];
            } else if (layout.equipBtn.contains(mouse)) {
                if (m_selectedIsEquipped) UnequipSelected(inventory, equipment, stats);
                else EquipSelected(inventory, equipment, stats);
            } else if (layout.sellBtn.contains(mouse)) {
                if (m_selectedIsEquipped) SellEquippedSlot(equipment, stats);
                else SellSelected(inventory, stats);
            } else if (layout.discardBtn.contains(mouse)) {
                if (!m_selectedIsEquipped) DiscardSelected(inventory);
            }
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto players = registry.View<PlayerTag, InventoryComponent, EquipmentComponent, CharacterStatsComponent>();
        if (players.empty()) return;
        Entity player = players[0];

        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        PanelLayout layout = ComputeLayout(target.getSize());
        float panelX = layout.panelX;
        float panelY = layout.panelY;

        sf::RectangleShape bg({ kPanelW, kPanelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        std::string closeKey = KeyToString(KeyBindings::Instance().Get(GameAction::ToggleInventory));
        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "所持品 (" + closeKey + " で閉じる) - クリックまたはドラッグで操作、パネル外にドロップで地面に捨てる",
            15, sf::Color(255, 220, 120));
        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            std::to_string(inventory.items.size()) + " / " + std::to_string(InventoryComponent::kCapacity) + " 個所持",
            13, sf::Color(180, 180, 180));

        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();

        // 装備欄(ペーパードール)
        for (const auto& def : kDollSlots) {
            sf::FloatRect rect = DollSlotRect(layout, def);
            bool hasItem = equipment.slots[static_cast<size_t>(def.slot)].has_value();
            bool draggingAway = m_dragging && m_dragFromEquipped && def.slot == m_dragSlot;
            bool selected = m_selectedIsEquipped && def.slot == m_selectedSlot;
            bool hovered = rect.contains(mouse);

            sf::RectangleShape box(rect.size);
            box.setPosition(rect.position);
            sf::Color fill = hasItem ? sf::Color(45, 45, 55) : sf::Color(25, 25, 30);
            if (hasItem) {
                sf::Color rc = ItemUIHelpers::RarityColor(equipment.slots[static_cast<size_t>(def.slot)]->rarity);
                fill = sf::Color(rc.r / 4, rc.g / 4, rc.b / 4);
            }
            if (draggingAway) fill = sf::Color(30, 30, 35, 120);
            box.setFillColor(hovered ? sf::Color((std::min)(255, fill.r + 25), (std::min)(255, fill.g + 25), (std::min)(255, fill.b + 25), fill.a) : fill);
            box.setOutlineColor(selected ? sf::Color::Yellow : sf::Color(120, 120, 130));
            box.setOutlineThickness(selected ? 2.5f : 1.5f);
            target.draw(box);

            if (hasItem && !draggingAway) {
                const ItemComponent& item = *equipment.slots[static_cast<size_t>(def.slot)];
                DrawText(target, rect.position.x + 4.0f, rect.position.y + 4.0f, Truncate(item.baseName, 7), 11, ItemUIHelpers::RarityColor(item.rarity));
                DrawText(target, rect.position.x + 4.0f, rect.position.y + rect.size.y - 16.0f, "Lv" + std::to_string(item.itemLevel), 10, sf::Color(190, 190, 190));
            } else if (!hasItem) {
                DrawText(target, rect.position.x + 4.0f, rect.position.y + rect.size.y / 2.0f - 7.0f, def.label, 12, sf::Color(110, 110, 115));
            }
        }

        // 所持品グリッド
        for (int i = 0; i < kGridCols * kGridRows; ++i) {
            sf::FloatRect rect = CellRect(layout, i);
            bool hasItem = i < static_cast<int>(inventory.items.size());
            bool draggingAway = m_dragging && !m_dragFromEquipped && hasItem && i == m_dragBagIndex;
            bool selected = !m_selectedIsEquipped && hasItem && i == m_selectedIndex;
            bool hovered = rect.contains(mouse);

            sf::RectangleShape box(rect.size);
            box.setPosition(rect.position);
            sf::Color fill = sf::Color(25, 25, 30);
            if (hasItem) {
                sf::Color rc = ItemUIHelpers::RarityColor(inventory.items[i].rarity);
                fill = sf::Color(rc.r / 4, rc.g / 4, rc.b / 4);
            }
            if (draggingAway) fill = sf::Color(20, 20, 24);
            box.setFillColor(hovered && hasItem ? sf::Color((std::min)(255, fill.r + 25), (std::min)(255, fill.g + 25), (std::min)(255, fill.b + 25)) : fill);
            box.setOutlineColor(selected ? sf::Color::Yellow : sf::Color(90, 90, 100));
            box.setOutlineThickness(selected ? 2.5f : 1.0f);
            target.draw(box);

            if (hasItem && !draggingAway) {
                const ItemComponent& item = inventory.items[i];
                DrawText(target, rect.position.x + 4.0f, rect.position.y + 4.0f, ShortSlotCode(item.slot), 11, ItemUIHelpers::RarityColor(item.rarity));
                DrawText(target, rect.position.x + 4.0f, rect.position.y + rect.size.y - 16.0f, "Lv" + std::to_string(item.itemLevel), 10, sf::Color(190, 190, 190));
            }
        }

        // 選択中アイテムの詳細(フッター)
        std::string detailLine1, detailLine2;
        if (m_selectedIsEquipped) {
            if (equipment.slots[static_cast<size_t>(m_selectedSlot)].has_value()) {
                const ItemComponent& item = *equipment.slots[static_cast<size_t>(m_selectedSlot)];
                detailLine1 = ItemUIHelpers::SlotName(item.slot) + ": " + item.baseName + " (" + ItemUIHelpers::RarityName(item.rarity) +
                    ", 装備Lv" + std::to_string(item.itemLevel) + ", 追加効果" + std::to_string(item.affixes.size()) +
                    "個, スコア" + std::to_string(static_cast<int>(item.PowerScore())) + ")";
                detailLine2 = "装備中 - 「外す」ボタンでバッグに戻せます。";
            }
        } else if (!inventory.items.empty()) {
            const ItemComponent& item = inventory.items[m_selectedIndex];
            detailLine1 = ItemUIHelpers::SlotName(item.slot) + ": " + item.baseName + " (" + ItemUIHelpers::RarityName(item.rarity) +
                ", 装備Lv" + std::to_string(item.itemLevel) + ", 追加効果" + std::to_string(item.affixes.size()) +
                "個, スコア" + std::to_string(static_cast<int>(item.PowerScore())) + ")";
            size_t slotIdx = static_cast<size_t>(item.slot);
            if (equipment.slots[slotIdx].has_value()) {
                const ItemComponent& equipped = *equipment.slots[slotIdx];
                detailLine2 = "そのスロットに装備中: " + equipped.baseName + " (スコア" +
                    std::to_string(static_cast<int>(equipped.PowerScore())) + ")";
            } else {
                detailLine2 = "そのスロットは空です。";
            }
        } else {
            detailLine1 = "(バッグは空です)";
        }
        DrawText(target, panelX + kContentX, panelY + kFooterY, detailLine1, 13, sf::Color(220, 220, 220));
        if (!detailLine2.empty()) {
            DrawText(target, panelX + kContentX, panelY + kFooterY + 18.0f, detailLine2, 13, sf::Color(200, 200, 200));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + kContentX, panelY + kFooterY + 36.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        bool hasSelection = m_selectedIsEquipped
            ? equipment.slots[static_cast<size_t>(m_selectedSlot)].has_value()
            : !inventory.items.empty();
        std::string equipLabel = m_selectedIsEquipped ? "外す" : "装備";
        DrawButton(target, layout.equipBtn, equipLabel, layout.equipBtn.contains(mouse), hasSelection ? sf::Color(60, 110, 150) : sf::Color(60, 60, 65));
        DrawButton(target, layout.sellBtn, "売却", layout.sellBtn.contains(mouse), hasSelection ? sf::Color(150, 130, 50) : sf::Color(60, 60, 65));
        bool canDiscard = hasSelection && !m_selectedIsEquipped;
        DrawButton(target, layout.discardBtn, "破棄", layout.discardBtn.contains(mouse), canDiscard ? sf::Color(130, 60, 60) : sf::Color(60, 60, 65));

        // ドラッグ中のゴースト(カーソルに追従)
        if (m_dragging) {
            float gw = 70.0f, gh = 40.0f;
            sf::RectangleShape ghost({ gw, gh });
            ghost.setPosition({ mouse.x - gw / 2.0f, mouse.y - gh / 2.0f });
            sf::Color rc = ItemUIHelpers::RarityColor(m_dragItemCache.rarity);
            ghost.setFillColor(sf::Color(rc.r, rc.g, rc.b, 160));
            ghost.setOutlineColor(sf::Color::White);
            ghost.setOutlineThickness(2.0f);
            target.draw(ghost);
            DrawText(target, mouse.x - gw / 2.0f + 4.0f, mouse.y - gh / 2.0f + 4.0f, Truncate(m_dragItemCache.baseName, 8), 11, sf::Color::Black);

            sf::FloatRect panelRect({ panelX, panelY }, { kPanelW, kPanelH });
            if (!panelRect.contains(mouse)) {
                DrawText(target, mouse.x + gw / 2.0f + 6.0f, mouse.y - 6.0f, "地面に捨てる", 12, sf::Color(255, 120, 120));
            }
        }

        target.setView(oldView);
    }

private:
    PanelLayout ComputeLayout(sf::Vector2u winSize) const {
        // PoE2同様、画面右側にドッキング(ゲームを止めずに開ける画面なので
        // 中央を塞がず、フィールドの様子が見えるようにする)。
        PanelLayout layout;
        layout.panelX = static_cast<float>(winSize.x) - kPanelW - 20.0f;
        layout.panelY = (static_cast<float>(winSize.y) - kPanelH) / 2.0f;

        float totalBtnW = kButtonW * 3.0f + kButtonGap * 2.0f;
        float btnX = layout.panelX + (kPanelW - totalBtnW) / 2.0f;
        float btnY = layout.panelY + kFooterY + 56.0f;
        layout.equipBtn = sf::FloatRect({ btnX, btnY }, { kButtonW, kButtonH });
        layout.sellBtn = sf::FloatRect({ btnX + kButtonW + kButtonGap, btnY }, { kButtonW, kButtonH });
        layout.discardBtn = sf::FloatRect({ btnX + (kButtonW + kButtonGap) * 2.0f, btnY }, { kButtonW, kButtonH });
        return layout;
    }

    sf::FloatRect DollSlotRect(const PanelLayout& layout, const DollSlotDef& def) const {
        float x = layout.panelX + kContentX + static_cast<float>(def.col) * (kDollSlotSize + kDollGap);
        float y = layout.panelY + kContentY + static_cast<float>(def.row) * (kDollSlotSize + kDollGap);
        return sf::FloatRect({ x, y }, { kDollSlotSize, kDollSlotSize });
    }

    sf::FloatRect CellRect(const PanelLayout& layout, int index) const {
        int col = index % kGridCols;
        int row = index / kGridCols;
        float x = layout.panelX + kGridX + static_cast<float>(col) * (kCellW + kCellGap);
        float y = layout.panelY + kContentY + static_cast<float>(row) * (kCellH + kCellGap);
        return sf::FloatRect({ x, y }, { kCellW, kCellH });
    }

    bool HitTestDoll(const PanelLayout& layout, sf::Vector2f mouse, EquipSlot& outSlot) const {
        for (const auto& def : kDollSlots) {
            if (DollSlotRect(layout, def).contains(mouse)) {
                outSlot = def.slot;
                return true;
            }
        }
        return false;
    }

    int HitTestGridCell(const PanelLayout& layout, sf::Vector2f mouse) const {
        for (int i = 0; i < kGridCols * kGridRows; ++i) {
            if (CellRect(layout, i).contains(mouse)) return i;
        }
        return -1;
    }

    // ドラッグ終了時のドロップ先判定。装備⇔バッグの移動、バッグ内の入れ替え、
    // パネル外へのドロップ(足元に投棄)を処理する。
    void HandleDrop(Registry& registry, Entity player, sf::Vector2f mouse, const PanelLayout& layout,
        InventoryComponent& inventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {

        sf::FloatRect panelRect({ layout.panelX, layout.panelY }, { kPanelW, kPanelH });
        if (!panelRect.contains(mouse)) {
            DropDraggedItemOnGround(registry, player, inventory, equipment, stats);
            return;
        }

        EquipSlot targetSlot;
        bool onDoll = HitTestDoll(layout, mouse, targetSlot);
        int targetCell = HitTestGridCell(layout, mouse);

        if (onDoll) {
            if (!m_dragFromEquipped) {
                if (m_dragBagIndex < 0 || m_dragBagIndex >= static_cast<int>(inventory.items.size())) return;
                const ItemComponent& item = inventory.items[m_dragBagIndex];
                if (item.slot == targetSlot) {
                    m_selectedIsEquipped = false;
                    m_selectedIndex = m_dragBagIndex;
                    EquipSelected(inventory, equipment, stats);
                }
            } else if (targetSlot != m_dragSlot) {
                // 指輪1⇔指輪2のみ入れ替えに対応(それ以外の異スロット間ドラッグは無効)
                bool bothRings = (m_dragSlot == EquipSlot::Ring1 || m_dragSlot == EquipSlot::Ring2) &&
                    (targetSlot == EquipSlot::Ring1 || targetSlot == EquipSlot::Ring2);
                if (bothRings) {
                    std::swap(equipment.slots[static_cast<size_t>(m_dragSlot)], equipment.slots[static_cast<size_t>(targetSlot)]);
                    EquipmentSystem::RecalculateStats(stats, equipment);
                    m_selectedSlot = targetSlot;
                }
            }
        } else if (targetCell >= 0) {
            if (m_dragFromEquipped) {
                m_selectedIsEquipped = true;
                m_selectedSlot = m_dragSlot;
                UnequipSelected(inventory, equipment, stats);
            } else if (targetCell < static_cast<int>(inventory.items.size()) && targetCell != m_dragBagIndex) {
                std::swap(inventory.items[m_dragBagIndex], inventory.items[targetCell]);
                m_selectedIsEquipped = false;
                m_selectedIndex = targetCell;
            }
        }
        // ドール/グリッドどちらでもない場所(フッターやボタン付近)へのドロップは
        // 何もせず元の位置に留める。
    }

    void DropDraggedItemOnGround(Registry& registry, Entity player, InventoryComponent& inventory,
        EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        ItemComponent dropped;
        if (m_dragFromEquipped) {
            auto& slotOpt = equipment.slots[static_cast<size_t>(m_dragSlot)];
            if (!slotOpt.has_value()) return;
            dropped = *slotOpt;
            slotOpt.reset();
            EquipmentSystem::RecalculateStats(stats, equipment);
            m_selectedIsEquipped = false;
        } else {
            if (m_dragBagIndex < 0 || m_dragBagIndex >= static_cast<int>(inventory.items.size())) return;
            dropped = inventory.items[m_dragBagIndex];
            inventory.items.erase(inventory.items.begin() + m_dragBagIndex);
            ClampSelection(inventory);
        }

        if (!registry.HasComponent<TransformComponent>(player)) return;
        auto& playerTrans = registry.GetComponent<TransformComponent>(player);

        auto pickup = registry.CreateEntityObject();
        pickup.AddComponent(TransformComponent{ playerTrans.position, {1.f, 1.f}, 0.f });
        pickup.AddComponent(CircleComponent{ 8.0f, ItemUIHelpers::RarityColor(dropped.rarity), true });
        pickup.AddComponent(ItemPickupComponent{ dropped });

        lastActionMessage = "地面に捨てた: " + dropped.baseName;
        messageTimer = 2.5f;
    }

    std::string ShortSlotCode(EquipSlot slot) const {
        switch (slot) {
        case EquipSlot::Weapon: return "武器";
        case EquipSlot::BodyArmour: return "胴";
        case EquipSlot::Helmet: return "兜";
        case EquipSlot::Gloves: return "手袋";
        case EquipSlot::Boots: return "靴";
        case EquipSlot::Ring1:
        case EquipSlot::Ring2: return "指輪";
        case EquipSlot::Amulet: return "首飾";
        case EquipSlot::Belt: return "帯";
        default: return "?";
        }
    }

    std::string Truncate(const std::string& s, size_t maxLen) const {
        if (s.size() <= maxLen) return s;
        return s.substr(0, maxLen);
    }

    void DrawButton(sf::RenderTarget& target, const sf::FloatRect& rect, const std::string& label, bool hovered, sf::Color fillColor) {
        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        sf::Color drawColor = hovered
            ? sf::Color((std::min)(255, fillColor.r + 35), (std::min)(255, fillColor.g + 35), (std::min)(255, fillColor.b + 35), fillColor.a)
            : fillColor;
        box.setFillColor(drawColor);
        box.setOutlineColor(sf::Color(200, 200, 200));
        box.setOutlineThickness(1.5f);
        target.draw(box);

        sf::Text text(*m_font, sf::String::fromUtf8(label.begin(), label.end()), 15);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition({
            rect.position.x + (rect.size.x - textBounds.size.x) / 2.0f - textBounds.position.x,
            rect.position.y + (rect.size.y - textBounds.size.y) / 2.0f - textBounds.position.y
            });
        target.draw(text);
    }

    void EquipSelected(InventoryComponent& inventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        if (inventory.items.empty()) return;
        ItemComponent picked = inventory.items[m_selectedIndex];
        size_t slotIdx = static_cast<size_t>(picked.slot);
        auto& currentSlot = equipment.slots[slotIdx];

        if (currentSlot.has_value()) {
            inventory.items[m_selectedIndex] = *currentSlot;
        } else {
            inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        }
        currentSlot = picked;
        EquipmentSystem::RecalculateStats(stats, equipment);

        lastActionMessage = "装備した: " + picked.baseName;
        messageTimer = 2.5f;
        ClampSelection(inventory);
    }

    void UnequipSelected(InventoryComponent& inventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        auto& slotOpt = equipment.slots[static_cast<size_t>(m_selectedSlot)];
        if (!slotOpt.has_value()) return;

        if (inventory.items.size() >= InventoryComponent::kCapacity) {
            lastActionMessage = "バッグがいっぱいです";
            messageTimer = 2.0f;
            return;
        }

        std::string name = slotOpt->baseName;
        inventory.items.push_back(*slotOpt);
        slotOpt.reset();
        EquipmentSystem::RecalculateStats(stats, equipment);

        lastActionMessage = "外した: " + name;
        messageTimer = 2.5f;

        m_selectedIsEquipped = false;
        m_selectedIndex = static_cast<int>(inventory.items.size()) - 1;
    }

    void SellSelected(InventoryComponent& inventory, CharacterStatsComponent& stats) {
        if (inventory.items.empty()) return;
        const ItemComponent& item = inventory.items[m_selectedIndex];
        int goldValue = ItemFactory::SellValue(item);
        stats.gold += goldValue;

        lastActionMessage = "売却した: " + item.baseName + " (" + std::to_string(goldValue) + "ゴールド)";
        messageTimer = 2.5f;

        inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        ClampSelection(inventory);
    }

    void SellEquippedSlot(EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        auto& slotOpt = equipment.slots[static_cast<size_t>(m_selectedSlot)];
        if (!slotOpt.has_value()) return;

        int goldValue = ItemFactory::SellValue(*slotOpt);
        stats.gold += goldValue;
        lastActionMessage = "売却した: " + slotOpt->baseName + " (" + std::to_string(goldValue) + "ゴールド)";
        messageTimer = 2.5f;

        slotOpt.reset();
        EquipmentSystem::RecalculateStats(stats, equipment);
        m_selectedIsEquipped = false;
    }

    void DiscardSelected(InventoryComponent& inventory) {
        if (inventory.items.empty()) return;
        const ItemComponent& item = inventory.items[m_selectedIndex];

        lastActionMessage = "破棄した: " + item.baseName;
        messageTimer = 2.5f;

        inventory.items.erase(inventory.items.begin() + m_selectedIndex);
        ClampSelection(inventory);
    }

    // std::clamp(x, 0, size-1)はsize==0のときlo>hiとなり未定義動作になるため
    // (最後の1個を装備/売却/破棄した瞬間に発生する)、空チェックを必ず挟む。
    void ClampSelection(const InventoryComponent& inventory) {
        if (inventory.items.empty()) {
            m_selectedIndex = 0;
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(inventory.items.size()) - 1);
        }
    }

    void DrawText(sf::RenderTarget& target, float x, float y, const std::string& str, unsigned int size, sf::Color color) {
        // std::string -> sf::Text's implicit sf::String constructor treats the bytes as
        // ANSI/locale text, not UTF-8; fromUtf8 is required or Japanese text corrupts.
        sf::Text text(*m_font, sf::String::fromUtf8(str.begin(), str.end()), size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }

};
