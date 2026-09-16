# プロジェクト構造マップ

このファイルの目的: 新しいセッションのたびにフォルダ構成を探索し直さずに済むように、
「どこに何があるか」をあらかじめまとめておく。フォルダ構成が変わったら、
このファイルも同じコミットで更新すること。

---

## ディレクトリ概要

| フォルダ | 役割 |
|---|---|
| `Programs/System/` | エンジン基盤（Main/Application、SceneManager、Input、Campaign進行管理、Camera、Resource、Time、Performance計測など）。ゲーム固有ロジックは置かない。 |
| `Programs/Game/GameScene/` | メインのゲームプレイシーン。`GameScene.cpp/.h` が各ECSシステムの生成・Update/Render呼び出しを束ねる。 |
| `Programs/Game/GameScene/Entity/EntitySpawner.h` | プレイヤー/敵/構造物などエンティティ生成のファクトリ。 |
| `Programs/Game/GameScene/Zone/`, `MapGenerator/` | ゾーン（タウン/戦闘/ボス）の地形生成とビルド。 |
| `Programs/Game/TitleScene/`, `Programs/Game/ResultScene/` | タイトル画面・リザルト画面。 |
| `External/ECS/` | 自作ECSフレームワーク本体。`ECS.h` が集約ヘッダ。 |
| `External/ECS/Core/` | `ComponentPool`（スパースセット）、`Entity` など低レベル基盤。 |
| `External/ECS/Registry/` | `Registry`（エンティティ/コンポーネント管理、`View<>`）。 |
| `External/ECS/Components/` | コンポーネント定義（データのみ、ロジックなし）。サブフォルダは機能領域別（`Item/`, `Combat/`, `Stats/`, `Physics/`, `Tags/` など）。 |
| `External/ECS/Systems/` | システム定義（ロジック）。コンポーネントと同じ機能領域別のサブフォルダ構成（`Item/`, `Combat/`, `UI/`, `Progression/`, `Chara/`, `Skill/`, `Physics/`, `World/`）。 |
| `External/SFML/`, `External/ImGui*/`, `External/SpdLog/` | サードパーティライブラリ。編集しない。 |
| `Assets/Fonts/`, `Assets/Textures/`, `Assets/Sounds/`, `Assets/Data/Map/` | ゲームアセット。 |
| `png/` | README/資料用のスクリーンショット。ゲーム内では未使用。 |

## 主要機能とその場所

- **キャンペーン進行（Act1-4/幕間/Endgame、ゾーン遷移、セーブ/ロード）**: `Programs/System/Campaign/CampaignManager.h/.cpp`
- **エンドゲーム(ウェイストーン制マップ)**: 所持ウェイストーンは`External/ECS/Components/Item/Waystone.h`(`WaystoneInventoryComponent`: ティア1-15の所持数、`WaystonePickupComponent`: ドロップ)。マップを開く処理は`CampaignManager::OpenEndgameMap`と`GameScene::TryOpenEndgameMapFromHub`(拠点のポータルで最高ティアを消費)。ドロップは`CollisionSystem::TrySpawnWaystoneDrop`(ボス確定+1ティア、Rareは同ティア35%)。ティアによる耐性スケーリングは`ZoneBuilder::ApplyTierResistance`。
- **戦闘計算（命中率/Armour軽減/属性耐性/ES/Leech/状態異常発生率）**: `External/ECS/Systems/Combat/CombatMath.h`
- **状態異常（Ignite/Chill/Freeze/Shock/Poison/Bleed/Stun）**: `External/ECS/Systems/Combat/StatusEffectSystem.h`, `External/ECS/Components/Combat/StatusEffects.h`
- **装備（9スロット、affix、PowerScore比較）**: `External/ECS/Systems/Item/EquipmentSystem.h`, `External/ECS/Components/Item/Equipment.h`
- **インベントリUI（Iキー、装備入替/売却/破棄）**: `External/ECS/Systems/UI/InventorySystem.h`, `External/ECS/Components/Item/Inventory.h`
- **アイテム生成（affixロール、レアリティ抽選、売却額計算）**: `External/ECS/Systems/Item/ItemFactory.h`
- **アイテム拾得（インベントリへ格納、満杯時は自動売却）**: `External/ECS/Systems/Item/ItemPickupSystem.h`
- **通貨/クラフト（Transmutation/Regal/Chaos）**: `External/ECS/Systems/Item/CurrencySystem.h`
- **XP/レベリング**: `External/ECS/Systems/Progression/LevelSystem.h`
- **パッシブツリーUI（Pキー、ノード割り振り）**: `External/ECS/Systems/Progression/PassiveTreeSystem.h`, `PassiveTreeData.h`, `External/ECS/Components/Progression/PassiveTree.h`
- **Vendor（商人、Bキーで購入）**: `External/ECS/Systems/Item/VendorSystem.h`（NPC自体は`ZoneBuilder::SpawnVendor`でタウン中央に配置）
- **スキルジェム（Gキー、5スロット+Spirit2スロットをマウスのクリックのみで自由付け替え）**: `External/ECS/Systems/UI/SkillGemSystem.h`、ジェムの静的定義は`External/ECS/Systems/Skill/SkillGemData.h`、ドロップ/所持は`External/ECS/Components/Item/SkillGem.h`（`SkillGemPickupComponent`/`SkillGemInventoryComponent`）
- **アイテムUI共通ヘルパー（スロット名/レアリティ名・色）**: `External/ECS/Systems/UI/ItemUIHelpers.h`（CharacterSheetSystem/InventorySystem/VendorSystemが共有）
- **キャラクターシートUI（Cキー）**: `External/ECS/Systems/UI/CharacterSheetSystem.h`
- **ボスフェーズ（HP50%でEnrage）**: `External/ECS/Systems/Chara/BossPhaseSystem.h`
- **遠距離攻撃モンスター（kite/射撃AI）**: `External/ECS/Systems/Chara/EnemyRangedAttackSystem.h`（移動のkiting挙動は`EnemyAISystem.h`側）、マーカーは`External/ECS/Components/Chara/RangedAttacker.h`
- **突進モンスター（テレグラフ後に高速直進）**: `External/ECS/Systems/Chara/EnemyChargeSystem.h`（telegraph→charge→cooldownの状態遷移。移動自体は`EnemyAISystem.h`側、実際のダメージは専用の攻撃処理を持たず通常の接触ダメージ判定に乗る）、マーカーは`External/ECS/Components/Chara/Charger.h`
- **入力管理（キーボード/マウス/パッド）**: `Programs/System/Input/`
- **キーバインド設定（リバインド可能なゲームプレイ操作、Oキーで設定UI）**: `Programs/System/Input/KeyBindings/KeyBindings.h/.cpp`（保持/永続化）、`External/ECS/Systems/UI/KeyBindSystem.h`（リバインドUI）
- **デバッグUI（ImGui、F1系）**: `Programs/System/DebugManager/`, `Programs/System/DebugGui/`

## 命名・配置のルール（あれば）

- コンポーネント（データのみの構造体）は `Components/<領域>/<名前>/<名前>.h` または `Components/<領域>/<名前>.h` に置き、`〜Component` サフィックスを付ける（例: `TransformComponent`, `InventoryComponent`）。
- システム（ロジック）は `Systems/<領域>/<名前>System.h` に置き、`〜System` サフィックスを付ける。多くはヘッダオンリー（`.h`のみ、`.cpp`なし）。
- 新規に追加したヘッダファイルは `Game-SFML.vcxproj` の `ClInclude` に追記する（Visual Studioソリューションエクスプローラー表示のため。ビルド自体には必須ではないが漏らさないこと）。

## 探索不要なパス

基本的に読む必要がない、または大きすぎて無駄になるフォルダ。

- `External/SFML/`, `External/ImGui/`, `External/ImGui-SFML/`, `External/ImGuizmo/`, `External/SpdLog/` — サードパーティ製、通常編集しない。
- `x64/` (存在する場合) — ビルド成果物（`.gitignore`対象）。
- `packages/` — NuGetパッケージ復元先（`.gitignore`対象、`nuget.exe restore Game-SFML.sln` で復元可能）。

---

## 更新ルール（重要）

**フォルダ構成やファイルの配置が変わったら、このファイルも同じコミットで更新すること。**
放置して情報が古くなると、「探索を省略できる」という利点より
「誤った場所へ誘導してしまい、結局探し直しになる」という害の方が大きくなる。
