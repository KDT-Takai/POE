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

- **進行管理(エンドゲームループのみ、ゾーン遷移、セーブ/ロード)**: `Programs/System/Campaign/CampaignManager.h/.cpp`。Act1-4/幕間のストーリーキャンペーンは廃止済み(`AI/DECISIONS.md`参照)、`m_acts`はエンドゲーム用の1要素のみを保持する。
- **エンドゲーム(ウェイストーン制マップ、本作の唯一のゲームループ)**: Waystoneは独立コンポーネントではなく`ItemComponent`(`External/ECS/Components/Item/Item.h`)の`category==ItemCategory::Waystone`(`waystoneTier`/`waystoneMods`フィールドを使用)として`InventoryComponent`に通常アイテムと同じ形式で格納される(グリッド上1x1マス、満杯時は購入/拾得不可)。マップを開く処理は拠点のポータル(Enterキー、`GameScene::TryOpenEndgameMapFromHub`)から`AtlasSystem`(`External/ECS/Systems/Progression/AtlasSystem.h`、PoE2風の全画面Atlas)を開く形になっており、既にマップアタempt中(`CampaignManager::HasActiveMapAttempt`)ならそのまま再入場、そうでなければAtlas画面でノードを選ぶ。Atlasは`AtlasComponent`(`External/ECS/Components/Progression/Atlas.h`、クリア済みノード座標のみ保持)+`AtlasData`(純粋関数群、座標→ティア/隣接/座標変換を都度算出、ノード自体は保存しない)で構成され、開始ノード(0,0)から4方向に隣接ノードをクリアするごとに次のノードが解放される、実質無限に外側へ広がるグリッド(表示範囲は既クリア最遠距離+2まで、`AtlasSystem::VisibleRadius`)。ノード選択→「Open Map」でそのティアに一致するWaystoneをバッグから1個消費し`CampaignManager::OpenEndgameMap`+`SetPendingAtlasNode`を呼ぶ(`ItemComponent::waystoneMods`を`CampaignManager`の「マップアタempt」状態(`m_mapAttemptActive`/`m_mapDeathsRemaining`=6/`m_activeMapMods`)へコピー)。マップ内で最大6回死亡可能(`CampaignManager::ConsumeMapDeath`、0を切るとハブへ強制送還しAtlasノードは未クリアのまま)。クリアすると`CampaignManager::GetPendingAtlasNode`で対象ノードを特定し`AtlasData::MarkCompleted`でクリア済みにする(`GameScene::Update`のゾーンクリア判定内)。ドロップは`CollisionSystem::TrySpawnWaystoneDrop`(ボス確定+1ティア、Rareは同ティア35%、`ItemFactory::GenerateWaystone`でMOD付きのアイテムとして生成)。ティアによる耐性スケーリングは`ZoneBuilder::ApplyTierResistance`、WaystoneMOD(モンスター強化/プレイヤー耐性低下/報酬増加、`WaystoneModStat`)は`ZoneBuilder::ApplyMapMods`(モンスター系)・`GameScene`(プレイヤー耐性系)・`CollisionSystem::TrySpawnItemDrop`(報酬系)がそれぞれ反映する。Waystone専売NPCの購入上限は`WaystoneVendorSystem::RequiredLevelForTier`のテーブル(Tier2=Lv5, Tier3=Lv15を固定アンカーとしTier15=Lv99まで補間)でゲートされる。プレイヤーは`EntitySpawner::CreatePlayer`でTier1ウェイストーンを1個所持した状態でエンドゲームハブから始まる。
- **戦闘計算（命中率/Armour軽減/属性耐性/ES/Leech/状態異常発生率）**: `External/ECS/Systems/Combat/CombatMath.h`
- **状態異常（Ignite/Chill/Freeze/Shock/Poison/Bleed/Stun）**: `External/ECS/Systems/Combat/StatusEffectSystem.h`, `External/ECS/Components/Combat/StatusEffects.h`
- **装備（9スロット、affix、PowerScore比較）**: `External/ECS/Systems/Item/EquipmentSystem.h`, `External/ECS/Components/Item/Equipment.h`
- **インベントリUI（Iキー、装備入替/売却/破棄）**: `External/ECS/Systems/UI/InventorySystem.h`, `External/ECS/Components/Item/Inventory.h`
- **アイテム生成（affixロール、レアリティ抽選、売却額計算）**: `External/ECS/Systems/Item/ItemFactory.h`
- **アイテム拾得（インベントリへ格納、満杯時は自動売却）**: `External/ECS/Systems/Item/ItemPickupSystem.h`
- **通貨/クラフト（Transmutation/Regal/Chaos）**: `External/ECS/Systems/Item/CurrencySystem.h`
- **XP/レベリング**: `External/ECS/Systems/Progression/LevelSystem.h`
- **パッシブツリーUI（Pキー、ノード割り振り）**: `External/ECS/Systems/Progression/PassiveTreeSystem.h`, `PassiveTreeData.h`, `External/ECS/Components/Progression/PassiveTree.h`
- **町のNPC（アイテムVendor/Waystone Vendor/Stash、クリックで話しかける）**: `ZoneBuilder::PlaceTownNpcs`が町の生成毎に3体を離れた位置へ配置し、`GameScene`が`ZoneBuildResult::townNpcs`(`TownNpcKind`+座標、`Programs/Game/GameScene/Zone/TownNpc.h`)を見てプレイヤー近接判定・クリック判定・該当UIの開閉を行う(単一Vendorだった旧実装を汎用化)。アイテム売買は`External/ECS/Systems/Item/VendorSystem.h`、Waystone専売は`External/ECS/Systems/Item/WaystoneVendorSystem.h`(ゴールドのみ、Buyback無し、全15ティアをレベルテーブルでゲート、購入したWaystoneは通常アイテムとしてバッグへ入る)、アイテム保管庫は`External/ECS/Systems/UI/StashSystem.h`(`External/ECS/Components/Item/Stash.h`の`StashComponent`、10x8グリッド×4タブ、タブ切替ボタン、バッグとの双方向ドラッグ、セーブ対象)。
- **スキルジェム（Gキー、5スキルスロット+5スピリットスロット=計10枠をマウスのクリックのみで自由付け替え）**: `External/ECS/Systems/UI/SkillGemSystem.h`。ジェムの静的定義は`External/ECS/Systems/Skill/SkillGemData.h`（レベル1-20要件の基礎値/必要属性STR・DEX・INT付き）、サポートジェムの静的定義は`External/ECS/Systems/Skill/SupportGemData.h`。両者の適用・レベルスケーリングは`External/ECS/Systems/Skill/SkillGemScaling.h`(`BuildEquippedSkillData`)/`SupportGemSystem.h`。所持ジェムの個体管理(レベル/ソケット数/装着中サポート)は`External/ECS/Components/Item/SkillGem.h`の`OwnedGemInstance`（`SkillGemInventoryComponent.ownedGems`）。ドロップは未鑑定のUncut Gem(`SkillGemPickupComponent`、レベル+`GemPickupKind`でSkill/Support/Spiritの種別のみ判定済み)として発生し、拾うと即座に`SkillGemInventoryComponent.pendingUncutGems`(所持上限`kPendingCapacity`=8)へ格納される。`SkillGemSystem`画面上部の「Uncut Gems」チップをクリックして`External/ECS/Systems/UI/GemIdentifySystem.h`(Gem Cutting)を開き、どのジェムになるか選んで初めて所持化する(サポートジェムも同じ経路が必須、カタログからの自由装着は不可)。ソケット拡張はJeweller's Orb(`CurrencyType::JewellersOrb`、`CharacterStatsComponent::jewellersOrbs`)をSkillGemSystem画面上のボタンで消費。実発動時の可否判定は`External/ECS/Systems/Skill/SkillActivationSystem.h`の`CanUseSkill`に一元化(`SkillSystem::Update`が使用)。
- **Permanent Minion（Spirit Gemの一種、`SkillBehaviorType::Minion`）**: `External/ECS/Systems/Chara/MinionSystem.h`が召喚体の追従/近接AI・死亡検知・リスポーンを管理。スポーン/デスポーン自体は`SpiritAuraSystem`のON/OFFがトリガー(`EntitySpawner::CreateMinion`で生成)。マーカーは`External/ECS/Components/Chara/Minion.h`(`AllyTagComponent`=プレイヤー陣営、`PermanentMinionComponent`=所有者/スロット参照)。
- **アイテムUI共通ヘルパー（スロット名/レアリティ名・色）**: `External/ECS/Systems/UI/ItemUIHelpers.h`（CharacterSheetSystem/InventorySystem/VendorSystemが共有）
- **キャラクターシートUI（Cキー）**: `External/ECS/Systems/UI/CharacterSheetSystem.h`
- **ボスフェーズ（HP50%でEnrage）**: `External/ECS/Systems/Chara/BossPhaseSystem.h`
- **遠距離攻撃モンスター（kite/射撃AI）**: `External/ECS/Systems/Chara/EnemyRangedAttackSystem.h`（移動のkiting挙動は`EnemyAISystem.h`側）、マーカーは`External/ECS/Components/Chara/RangedAttacker.h`
- **突進モンスター（テレグラフ後に高速直進）**: `External/ECS/Systems/Chara/EnemyChargeSystem.h`（telegraph→charge→cooldownの状態遷移。移動自体は`EnemyAISystem.h`側、実際のダメージは専用の攻撃処理を持たず通常の接触ダメージ判定に乗る）、マーカーは`External/ECS/Components/Chara/Charger.h`
- **入力管理（キーボード/マウス/パッド）**: `Programs/System/Input/`
- **キーバインド設定（リバインド可能なゲームプレイ操作、Oキーで設定UI）**: `Programs/System/Input/KeyBindings/KeyBindings.h/.cpp`（保持/永続化）、`External/ECS/Systems/UI/KeyBindSystem.h`（リバインドUI）
- **デバッグUI（ImGui、F1系）**: `Programs/System/DebugManager/`, `Programs/System/DebugGui/`。ジェム専用のデバッグツール(Uncut Gemスポーン/ステータス編集/Spirit・スキル計算値表示)は`GameScene::RenderGemDebugTools`（Registryアクセスが要るため`DebugManager`本体ではなく`GameScene::RenderImGui`側に実装、"Gem Debug"ウィンドウ）。

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
