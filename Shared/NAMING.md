# 命名法則

## 1. 目的
このドキュメントは、本プロジェクト（C++ / SFML / 自作ECS）内で使用する命名規則を統一するためのガイドラインを定義する。
テンプレート由来の一般ルールではなく、実際のコードベースの慣習に合わせて調整済み。

---

## 2. 基本ルール
- 名前は意味が分かるものにする。略語の多用・1文字変数（ループ変数は除く）・意味のない省略は避ける。
- 識別子（変数・関数・クラス名）は英語で統一する。コメントやログ文字列は日本語可。
- 命名から責務が推測できるようにする。

---

## 3. 命名スタイル

| 対象 | スタイル | 例 |
|------|---------|----|
| クラス / システム | PascalCase | `GameScene`, `EquipmentSystem`, `InventorySystem` |
| コンポーネント（データ構造体） | PascalCase + `Component`サフィックス | `TransformComponent`, `InventoryComponent`, `ItemPickupComponent` |
| 純粋データ型（コンポーネントのフィールドとして使う値オブジェクト） | PascalCase（`Component`サフィックスなし） | `ItemComponent`, `ItemAffix`, `ZoneDefinition` |
| 関数・メソッド | PascalCase、動詞から始める | `Update()`, `RecalculateStats()`, `TryEquip()` |
| enum型名 / enum値 | PascalCase | `enum class EquipSlot { Weapon, BodyArmour, ... }` |
| ローカル変数・関数引数 | camelCase | `dt`, `playerTrans`, `itemLevel` |
| メンバ変数（新規コード） | `m_`+camelCase | `m_font`, `m_selectedIndex`, `m_actIndex`（`CampaignManager`, `CharacterSheetSystem`, `InventorySystem` 等の比較的新しいファイルで採用） |
| メンバ変数（既存の古いコード） | camelCase（`m_`無し） | `playerEntity`, `registry`, `itemPickupSystem`（`GameScene`, `EntitySpawner` 等） |
| 静的定数 | `k`+PascalCase（新規）/ `UPPER_SNAKE_CASE`（既存） | `kCapacity`（新規）, `KEY_MAX`（既存の`KeyInput`） |
| ファイル名 | クラス名と同名のPascalCase | `EquipmentSystem.h`, `InventorySystem.h` |

**既存メンバ変数の`m_`有無・定数の命名スタイルは混在している。既存ファイルを編集する際はそのファイルの既存スタイルに合わせ、新規ファイルでは`m_`+camelCase（メンバ）・`k`+PascalCase（静的定数）を使う。**

---

## 4. 用途別命名ルール

### 4.1 ブール値
- 接頭辞に `is` / `has` / `can` を使用する。
**例:** `isOpen`, `hasSavedPlayer`, `isValid`, `isFacingRight`

### 4.2 関数
- 動詞から始め、処理内容が分かるようにする。
**推奨プレフィックス:** `Get`, `Try`, `Create`, `Update`, `Render`, `Apply`, `Recalculate`, `Advance`
**例:** `TryEquip()`, `RecalculateStats()`, `AdvanceToNextZone()`

### 4.3 コレクション
- 配列やリストは複数形を使用する。
**例:** `items`, `affixes`, `slots`, `zones`

---

## 5. ファイル・フォルダ配置

- コンポーネントは `Components/<領域>/<名前>.h`（または `Components/<領域>/<名前>/<名前>.h`）に置く。
- システムは `Systems/<領域>/<名前>System.h` に置く。ロジックのみで多くはヘッダオンリー。
- 詳細は `AI/STRUCTURE.md` を参照。

---

## 6. ブランチ

- ブランチ名は `{type}/{kebab-case-name}` 形式（現行の実際の運用に合わせる。例: `feature/poe2-vertical-slice`）。
- 種別: `feature`（新機能）, `fix`（バグ修正）, `refactor`（リファクタリング）。
- 過去に `future/AddLanguage` のようなPascalCase・タイプミス表記のブランチが存在するが、これは踏襲しない。

---

## 7. 禁止パターン
- 意味のない省略、1文字変数（ループ変数除く）、文脈依存の名前、長すぎる名前。
**例（悪い例）:** `tmp`, `data2`, `func`, `aaa`
