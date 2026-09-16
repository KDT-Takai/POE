#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <array>
#include <vector>
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

// PoE2本家に寄せたアイテム画面: 左に体の部位に見立てた装備欄(ペーパードール)、
// 右に6x4マスの所持品グリッド。アイテムはPoE2同様スロット種別に応じたサイズを持ち
// (武器1x3、胴防具2x3など、`ItemUIHelpers::ItemGridSize`参照)、複数マスを占有する。
// 操作は全てマウスのみ(ボタンは無し):
//   - 左ドラッグ&ドロップ: アイテムを移動(装備⇔バッグ、バッグ内入れ替え、
//     パネル外へドロップで足元の地面に捨てる)
//   - 右クリック: そのスロットに応じて装備/外すを即実行(クイック装備)
//   - カーソルを乗せる: 名前/レアリティ/追加効果/売却額のツールチップを表示
//     (バッグアイテムの場合、同スロットの装備中アイテムとの比較も表示)
// 売却はここでは行わない: PoE2本家同様、タウン/隠れ家の商人NPCに話しかけて売却する。
class InventorySystem {
private:
    static constexpr float kPanelW = 760.0f;
    static constexpr float kPanelH = 400.0f;
    static constexpr float kContentX = 20.0f;
    static constexpr float kContentY = 70.0f;

    static constexpr float kDollSlotSize = 64.0f;
    static constexpr float kDollGap = 10.0f;
    static constexpr float kDollW = kDollSlotSize * 3.0f + kDollGap * 2.0f;
    static constexpr float kDollH = kDollSlotSize * 4.0f + kDollGap * 3.0f;

    static constexpr float kGridX = kContentX + kDollW + 30.0f;
    static constexpr int kGridCols = ItemUIHelpers::kBagGridCols;
    static constexpr int kGridRows = ItemUIHelpers::kBagGridRows;
    static constexpr float kCellW = 70.0f;
    static constexpr float kCellH = 70.0f;
    static constexpr float kCellGap = 6.0f;

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
    };

    std::shared_ptr<sf::Font> m_font;

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

        NormalizePlacement(inventory);

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
            return; // ドラッグ中は他の操作(右クリック等)を行わない
        }

        EquipSlot hoveredDollSlot;
        bool onDoll = HitTestDoll(layout, mouse, hoveredDollSlot);
        int hoveredItemIndex = HitTestBagItem(layout, mouse, inventory.items);
        bool hoveredCellHasItem = hoveredItemIndex >= 0;

        if (mouseInput.IsGetMouse(sf::Mouse::Button::Left)) {
            if (onDoll && equipment.slots[static_cast<size_t>(hoveredDollSlot)].has_value()) {
                m_dragging = true;
                m_dragFromEquipped = true;
                m_dragSlot = hoveredDollSlot;
                m_dragItemCache = *equipment.slots[static_cast<size_t>(hoveredDollSlot)];
            } else if (hoveredCellHasItem) {
                m_dragging = true;
                m_dragFromEquipped = false;
                m_dragBagIndex = hoveredItemIndex;
                m_dragItemCache = inventory.items[hoveredItemIndex];
            }
        } else if (mouseInput.IsGetMouse(sf::Mouse::Button::Right)) {
            // PoE2の右クリック(装備/外す即実行)を踏襲。
            if (onDoll && equipment.slots[static_cast<size_t>(hoveredDollSlot)].has_value()) {
                QuickUnequip(hoveredDollSlot, inventory, equipment, stats);
            } else if (hoveredCellHasItem) {
                QuickEquip(hoveredItemIndex, inventory, equipment, stats);
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
            "所持品 (" + closeKey + " で閉じる) - ドラッグで移動/右クリックで装備・外す/売却は商人に話しかける",
            15, sf::Color(255, 220, 120));

        int occupiedCells = 0;
        for (const auto& item : inventory.items) {
            sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item.slot);
            occupiedCells += sz.x * sz.y;
        }
        DrawText(target, panelX + 20.0f, panelY + 40.0f,
            "占有 " + std::to_string(occupiedCells) + " / " + std::to_string(kGridCols * kGridRows) + " マス",
            13, sf::Color(180, 180, 180));

        sf::Vector2f mouse = InputManager::Instance().GetMouseInput().GetMousePointF();

        const ItemComponent* hoveredItem = nullptr;
        const ItemComponent* hoveredCompare = nullptr;

        // 装備欄(ペーパードール)
        for (const auto& def : kDollSlots) {
            sf::FloatRect rect = DollSlotRect(layout, def);
            bool hasItem = equipment.slots[static_cast<size_t>(def.slot)].has_value();
            bool draggingAway = m_dragging && m_dragFromEquipped && def.slot == m_dragSlot;
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
            box.setOutlineColor(hovered && hasItem ? sf::Color::Yellow : sf::Color(120, 120, 130));
            box.setOutlineThickness(hovered && hasItem ? 2.5f : 1.5f);
            target.draw(box);

            if (hasItem && !draggingAway) {
                const ItemComponent& item = *equipment.slots[static_cast<size_t>(def.slot)];
                DrawText(target, rect.position.x + 4.0f, rect.position.y + 4.0f, Truncate(item.baseName, 7), 11, ItemUIHelpers::RarityColor(item.rarity));
                DrawText(target, rect.position.x + 4.0f, rect.position.y + rect.size.y - 16.0f, "Lv" + std::to_string(item.itemLevel), 10, sf::Color(190, 190, 190));
                ItemUIHelpers::DrawSocketPips(target, rect.position.x + 4.0f, rect.position.y + rect.size.y - 26.0f, ItemUIHelpers::SocketCount(item));
                if (hovered && !m_dragging) hoveredItem = &item;
            } else if (!hasItem) {
                DrawText(target, rect.position.x + 4.0f, rect.position.y + rect.size.y / 2.0f - 7.0f, def.label, 12, sf::Color(110, 110, 115));
            }
        }

        // 所持品グリッド: まず24マスの空セルを描画し、その上にアイテムをサイズ通りの
        // 矩形(複数マスにまたがる場合あり)として1個ずつ描画する。
        for (int row = 0; row < kGridRows; ++row) {
            for (int col = 0; col < kGridCols; ++col) {
                sf::FloatRect rect = CellRect(layout, col, row);
                sf::RectangleShape box(rect.size);
                box.setPosition(rect.position);
                box.setFillColor(sf::Color(25, 25, 30));
                box.setOutlineColor(sf::Color(70, 70, 78));
                box.setOutlineThickness(1.0f);
                target.draw(box);
            }
        }

        for (size_t i = 0; i < inventory.items.size(); ++i) {
            const ItemComponent& item = inventory.items[i];
            if (item.gridCol < 0 || item.gridRow < 0) continue; // 空き無しで未配置(異常系)
            bool draggingAway = m_dragging && !m_dragFromEquipped && static_cast<int>(i) == m_dragBagIndex;
            if (draggingAway) continue; // ドラッグ中は元の位置に描かず、カーソルのゴーストのみ表示

            sf::FloatRect rect = ItemRect(layout, item);
            bool hovered = rect.contains(mouse);
            sf::Color rc = ItemUIHelpers::RarityColor(item.rarity);
            sf::Color fill(rc.r / 4, rc.g / 4, rc.b / 4);

            sf::RectangleShape box(rect.size);
            box.setPosition(rect.position);
            box.setFillColor(hovered ? sf::Color((std::min)(255, fill.r + 25), (std::min)(255, fill.g + 25), (std::min)(255, fill.b + 25)) : fill);
            box.setOutlineColor(hovered ? sf::Color::Yellow : rc);
            box.setOutlineThickness(hovered ? 2.5f : 1.5f);
            target.draw(box);

            sf::Vector2i itemSz = ItemUIHelpers::ItemGridSize(item.slot);
            DrawText(target, rect.position.x + 4.0f, rect.position.y + 4.0f, Truncate(item.baseName, static_cast<size_t>(itemSz.x) * 9), 11, rc);
            DrawText(target, rect.position.x + 4.0f, rect.position.y + rect.size.y - 16.0f, "Lv" + std::to_string(item.itemLevel), 10, sf::Color(190, 190, 190));
            ItemUIHelpers::DrawSocketPips(target, rect.position.x + 4.0f, rect.position.y + rect.size.y - 26.0f, ItemUIHelpers::SocketCount(item));

            if (hovered && !m_dragging) {
                hoveredItem = &item;
                size_t slotIdx = static_cast<size_t>(item.slot);
                if (equipment.slots[slotIdx].has_value()) hoveredCompare = &(*equipment.slots[slotIdx]);
            }
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + kContentX, panelY + kPanelH - 22.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        // ドラッグ中のゴースト(カーソルに追従、アイテムの実サイズで表示)
        if (m_dragging) {
            sf::Vector2i sz = ItemUIHelpers::ItemGridSize(m_dragItemCache.slot);
            float gw = static_cast<float>(sz.x) * kCellW + static_cast<float>(sz.x - 1) * kCellGap;
            float gh = static_cast<float>(sz.y) * kCellH + static_cast<float>(sz.y - 1) * kCellGap;
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
        } else if (hoveredItem) {
            DrawItemTooltip(target, mouse, *hoveredItem, hoveredCompare, target.getSize());
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
        return layout;
    }

    sf::FloatRect DollSlotRect(const PanelLayout& layout, const DollSlotDef& def) const {
        float x = layout.panelX + kContentX + static_cast<float>(def.col) * (kDollSlotSize + kDollGap);
        float y = layout.panelY + kContentY + static_cast<float>(def.row) * (kDollSlotSize + kDollGap);
        return sf::FloatRect({ x, y }, { kDollSlotSize, kDollSlotSize });
    }

    sf::FloatRect CellRect(const PanelLayout& layout, int col, int row) const {
        float x = layout.panelX + kGridX + static_cast<float>(col) * (kCellW + kCellGap);
        float y = layout.panelY + kContentY + static_cast<float>(row) * (kCellH + kCellGap);
        return sf::FloatRect({ x, y }, { kCellW, kCellH });
    }

    // アイテムのグリッド座標+サイズから、複数マスにまたがりうる実際の描画/当たり判定矩形を返す。
    sf::FloatRect ItemRect(const PanelLayout& layout, const ItemComponent& item) const {
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item.slot);
        sf::FloatRect topLeft = CellRect(layout, item.gridCol, item.gridRow);
        float w = static_cast<float>(sz.x) * kCellW + static_cast<float>(sz.x - 1) * kCellGap;
        float h = static_cast<float>(sz.y) * kCellH + static_cast<float>(sz.y - 1) * kCellGap;
        return sf::FloatRect(topLeft.position, { w, h });
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

    // マウス位置がどのアイテムの矩形(複数マスにまたがりうる)に乗っているかを返す。
    int HitTestBagItem(const PanelLayout& layout, sf::Vector2f mouse, const std::vector<ItemComponent>& items) const {
        for (size_t i = 0; i < items.size(); ++i) {
            const ItemComponent& item = items[i];
            if (item.gridCol < 0 || item.gridRow < 0) continue;
            if (ItemRect(layout, item).contains(mouse)) return static_cast<int>(i);
        }
        return -1;
    }

    // マウス位置をグリッドのセル座標(col,row)へ変換する(ドロップ先の決定に使用)。
    bool MouseToCell(const PanelLayout& layout, sf::Vector2f mouse, int& outCol, int& outRow) const {
        float relX = mouse.x - (layout.panelX + kGridX);
        float relY = mouse.y - (layout.panelY + kContentY);
        if (relX < 0.0f || relY < 0.0f) return false;
        int col = static_cast<int>(relX / (kCellW + kCellGap));
        int row = static_cast<int>(relY / (kCellH + kCellGap));
        if (col < 0 || col >= kGridCols || row < 0 || row >= kGridRows) return false;
        outCol = col;
        outRow = row;
        return true;
    }

    // 未配置(gridCol<0、ロード直後やピックアップ直後)のアイテムに空きマスを割り当てる。
    void NormalizePlacement(InventoryComponent& inventory) {
        ItemUIHelpers::NormalizeBagPlacement(inventory.items);
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

        if (onDoll) {
            if (!m_dragFromEquipped) {
                if (m_dragBagIndex < 0 || m_dragBagIndex >= static_cast<int>(inventory.items.size())) return;
                const ItemComponent& item = inventory.items[m_dragBagIndex];
                if (item.slot == targetSlot) {
                    QuickEquip(m_dragBagIndex, inventory, equipment, stats);
                }
            } else if (targetSlot != m_dragSlot) {
                // 指輪1⇔指輪2のみ入れ替えに対応(それ以外の異スロット間ドラッグは無効)
                bool bothRings = (m_dragSlot == EquipSlot::Ring1 || m_dragSlot == EquipSlot::Ring2) &&
                    (targetSlot == EquipSlot::Ring1 || targetSlot == EquipSlot::Ring2);
                if (bothRings) {
                    std::swap(equipment.slots[static_cast<size_t>(m_dragSlot)], equipment.slots[static_cast<size_t>(targetSlot)]);
                    EquipmentSystem::RecalculateStats(stats, equipment);
                }
            }
            return;
        }

        int targetCol, targetRow;
        if (!MouseToCell(layout, mouse, targetCol, targetRow)) return; // ドール/グリッド以外(フッター等)は何もしない

        if (m_dragFromEquipped) {
            sf::Vector2i sz = ItemUIHelpers::ItemGridSize(m_dragSlot);
            if (ItemUIHelpers::BagRegionFree(inventory.items, targetCol, targetRow, sz.x, sz.y)) {
                UnequipToPosition(m_dragSlot, targetCol, targetRow, inventory, equipment, stats);
            } else {
                QuickUnequip(m_dragSlot, inventory, equipment, stats); // 空き無しなら自動配置にフォールバック
            }
        } else {
            TryMoveBagItem(m_dragBagIndex, targetCol, targetRow, inventory);
        }
    }

    // バッグ内アイテムを指定セルへ移動する。空いていれば移動、ちょうど同サイズの
    // 別アイテムがその位置を占めていれば入れ替え、それ以外は何もしない(元の位置に残る)。
    void TryMoveBagItem(int dragIndex, int targetCol, int targetRow, InventoryComponent& inventory) {
        if (dragIndex < 0 || dragIndex >= static_cast<int>(inventory.items.size())) return;
        ItemComponent& dragItem = inventory.items[dragIndex];
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(dragItem.slot);

        if (targetCol == dragItem.gridCol && targetRow == dragItem.gridRow) return; // 元の位置のまま

        if (ItemUIHelpers::BagRegionFree(inventory.items, targetCol, targetRow, sz.x, sz.y, dragIndex)) {
            dragItem.gridCol = targetCol;
            dragItem.gridRow = targetRow;
            return;
        }

        for (size_t i = 0; i < inventory.items.size(); ++i) {
            if (static_cast<int>(i) == dragIndex) continue;
            ItemComponent& other = inventory.items[i];
            if (other.gridCol != targetCol || other.gridRow != targetRow) continue;
            sf::Vector2i otherSz = ItemUIHelpers::ItemGridSize(other.slot);
            if (otherSz.x == sz.x && otherSz.y == sz.y) {
                std::swap(dragItem.gridCol, other.gridCol);
                std::swap(dragItem.gridRow, other.gridRow);
            }
            return;
        }
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
        } else {
            if (m_dragBagIndex < 0 || m_dragBagIndex >= static_cast<int>(inventory.items.size())) return;
            dropped = inventory.items[m_dragBagIndex];
            inventory.items.erase(inventory.items.begin() + m_dragBagIndex);
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

    std::string Truncate(const std::string& s, size_t maxLen) const {
        if (s.size() <= maxLen) return s;
        return s.substr(0, maxLen);
    }

    // バッグのアイテムを対応する装備欄へ即装備する(既に何か装備していれば入れ替えてバッグへ戻す)。
    void QuickEquip(int bagIndex, InventoryComponent& inventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        if (bagIndex < 0 || bagIndex >= static_cast<int>(inventory.items.size())) return;
        ItemComponent picked = inventory.items[bagIndex];
        size_t slotIdx = static_cast<size_t>(picked.slot);
        auto& currentSlot = equipment.slots[slotIdx];

        if (currentSlot.has_value()) {
            // 同じ装備スロット種別なのでグリッド上の占有サイズは常に同じ -> そのまま同じ位置に収まる。
            ItemComponent replaced = *currentSlot;
            replaced.gridCol = picked.gridCol;
            replaced.gridRow = picked.gridRow;
            inventory.items[bagIndex] = replaced;
        } else {
            inventory.items.erase(inventory.items.begin() + bagIndex);
        }
        currentSlot = picked;
        currentSlot->gridCol = -1;
        currentSlot->gridRow = -1;
        EquipmentSystem::RecalculateStats(stats, equipment);

        lastActionMessage = "装備した: " + picked.baseName;
        messageTimer = 2.5f;
    }

    // 装備欄のアイテムを即座に外してバッグの空きへ自動配置する(満杯なら失敗する)。
    void QuickUnequip(EquipSlot slot, InventoryComponent& inventory, EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        auto& slotOpt = equipment.slots[static_cast<size_t>(slot)];
        if (!slotOpt.has_value()) return;

        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(slot);
        int col, row;
        if (!ItemUIHelpers::FindBagFreeSpace(inventory.items, sz.x, sz.y, col, row)) {
            lastActionMessage = "バッグがいっぱいです";
            messageTimer = 2.0f;
            return;
        }

        ItemComponent item = *slotOpt;
        item.gridCol = col;
        item.gridRow = row;
        std::string name = item.baseName;
        inventory.items.push_back(item);
        slotOpt.reset();
        EquipmentSystem::RecalculateStats(stats, equipment);

        lastActionMessage = "外した: " + name;
        messageTimer = 2.5f;
    }

    // 装備欄のアイテムを、ドロップ先として狙った特定のセルへ外す(空いていることは呼び出し元で確認済み)。
    void UnequipToPosition(EquipSlot slot, int col, int row, InventoryComponent& inventory,
        EquipmentComponent& equipment, CharacterStatsComponent& stats) {
        auto& slotOpt = equipment.slots[static_cast<size_t>(slot)];
        if (!slotOpt.has_value()) return;

        ItemComponent item = *slotOpt;
        item.gridCol = col;
        item.gridRow = row;
        std::string name = item.baseName;
        inventory.items.push_back(item);
        slotOpt.reset();
        EquipmentSystem::RecalculateStats(stats, equipment);

        lastActionMessage = "外した: " + name;
        messageTimer = 2.5f;
    }

    // カーソルを乗せたアイテムの詳細ツールチップ。バッグアイテムの場合は同スロットの
    // 装備中アイテムとの比較(PoE2のホバー比較と同様)も併記する。
    void DrawItemTooltip(sf::RenderTarget& target, sf::Vector2f mouse, const ItemComponent& item,
        const ItemComponent* compared, sf::Vector2u winSize) {
        struct Line { std::string text; sf::Color color; };
        std::vector<Line> lines;

        lines.push_back({ item.baseName, ItemUIHelpers::RarityColor(item.rarity) });
        sf::Vector2i sz = ItemUIHelpers::ItemGridSize(item.slot);
        lines.push_back({ ItemUIHelpers::SlotName(item.slot) + " - " + ItemUIHelpers::RarityName(item.rarity) +
            " - Lv" + std::to_string(item.itemLevel) + " - " + std::to_string(sz.x) + "x" + std::to_string(sz.y),
            sf::Color(190, 190, 190) });

        int sockets = ItemUIHelpers::SocketCount(item);
        if (sockets > 0) {
            lines.push_back({ "ソケット: " + std::to_string(sockets), sf::Color(210, 210, 220) });
        }

        for (const auto& affix : item.affixes) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << (affix.value >= 0.0f ? "+" : "") << affix.value << " " << affix.label;
            lines.push_back({ ss.str(), sf::Color(150, 200, 255) });
        }

        std::ostringstream sellSs;
        sellSs << "売却額: " << ItemFactory::SellValue(item) << " ゴールド";
        lines.push_back({ sellSs.str(), sf::Color(200, 180, 120) });

        if (compared) {
            std::ostringstream cmp;
            cmp << "装備中: " << compared->baseName << " (スコア " << static_cast<int>(compared->PowerScore()) << ")";
            lines.push_back({ cmp.str(), sf::Color(170, 170, 170) });
            std::ostringstream self;
            self << "このアイテム: スコア " << static_cast<int>(item.PowerScore());
            lines.push_back({ self.str(), sf::Color(170, 220, 170) });
        }

        float lineH = 18.0f;
        float maxWidth = 0.0f;
        for (const auto& line : lines) {
            sf::Text probe(*m_font, sf::String::fromUtf8(line.text.begin(), line.text.end()), 13);
            maxWidth = (std::max)(maxWidth, probe.getLocalBounds().size.x);
        }

        float boxW = maxWidth + 24.0f;
        float boxH = static_cast<float>(lines.size()) * lineH + 16.0f;

        float x = mouse.x + 18.0f;
        float y = mouse.y + 10.0f;
        if (x + boxW > static_cast<float>(winSize.x) - 4.0f) x = mouse.x - boxW - 18.0f;
        if (y + boxH > static_cast<float>(winSize.y) - 4.0f) y = static_cast<float>(winSize.y) - boxH - 4.0f;

        sf::RectangleShape box({ boxW, boxH });
        box.setPosition({ x, y });
        box.setFillColor(sf::Color(10, 10, 14, 235));
        box.setOutlineColor(sf::Color(150, 150, 160));
        box.setOutlineThickness(1.5f);
        target.draw(box);

        for (size_t i = 0; i < lines.size(); ++i) {
            DrawText(target, x + 12.0f, y + 8.0f + static_cast<float>(i) * lineH, lines[i].text, 13, lines[i].color);
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
