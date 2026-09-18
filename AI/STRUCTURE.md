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
- **エンドゲーム(ウェイストーン制マップ、本作の唯一のゲームループ)**: Waystoneは独立コンポーネントではなく`ItemComponent`(`External/ECS/Components/Item/Item.h`)の`category==ItemCategory::Waystone`(`waystoneTier`/`waystoneMods`フィールドを使用)として`InventoryComponent`に通常アイテムと同じ形式で格納される(グリッド上1x1マス、満杯時は購入/拾得不可)。マップを開く処理は拠点のポータル(NPCと同じ左クリック操作、`GameScene::TryOpenEndgameMapFromHub`)から`AtlasSystem`(`External/ECS/Systems/Progression/AtlasSystem.h`、PoE2風の全画面Atlas)を開く形になっており、既にマップアタempt中(`CampaignManager::HasActiveMapAttempt`)ならそのまま再入場、そうでなければAtlas画面でノードを選ぶ。Atlasは`AtlasComponent`(`External/ECS/Components/Progression/Atlas.h`、クリア済みノード座標のみ保持)+`AtlasData`(純粋関数群、座標→隣接/座標変換を都度算出、ノード自体は保存しない)で構成され、開始ノード(0,0)から4方向に隣接ノードをクリアするごとに次のノードが解放される、実質無限に外側へ広がるグリッド(表示範囲は既クリア最遠距離+2まで、`AtlasSystem::VisibleRadius`)。ノードはティアを持たず、画面右にドッキングしたプレイヤーのバッグ(6x4グリッド)からWaystoneをノード選択後に現れる1マスの「ソケット」へドラッグ&ドロップすると、Waystoneでありさえすれば(ティアは不問)その場で消費して`CampaignManager::OpenEndgameMap`+`SetPendingAtlasNode`を呼ぶ(マップのティアはドロップしたWaystone自身のティアで決まる、`ItemComponent::waystoneMods`を`CampaignManager`の「マップアタempt」状態(`m_mapAttemptActive`/`m_mapDeathsRemaining`=6/`m_activeMapMods`)へコピー)。マップ内で最大6回死亡可能(`CampaignManager::ConsumeMapDeath`、0を切るとハブへ強制送還しAtlasノードは未クリアのまま)。クリアすると`CampaignManager::GetPendingAtlasNode`で対象ノードを特定し`AtlasData::MarkCompleted`でクリア済みにする(`GameScene::Update`のゾーンクリア判定内)。ドロップは`CollisionSystem::TrySpawnWaystoneDrop`(ボス確定+1ティア、Rareは同ティア35%、`ItemFactory::GenerateWaystone`でMOD付きのアイテムとして生成)。ティアによる耐性スケーリングは`ZoneBuilder::ApplyTierResistance`、WaystoneMOD(モンスター強化/プレイヤー耐性低下/報酬増加、`WaystoneModStat`)は`ZoneBuilder::ApplyMapMods`(モンスター系)・`GameScene`(プレイヤー耐性系)・`CollisionSystem::TrySpawnItemDrop`(報酬系)がそれぞれ反映する。Waystone専売NPCの購入上限は`WaystoneVendorSystem::RequiredLevelForTier`のテーブル(Tier2=Lv5, Tier3=Lv15を固定アンカーとしTier15=Lv99まで補間)でゲートされる。プレイヤーは`EntitySpawner::CreatePlayer`でTier1ウェイストーンを1個所持した状態でエンドゲームハブから始まる。
- **マップの帰還用ポータル(死亡ポータル数/円状の町側再入場口/詠唱生成/レア・ボス討伐出現)**: `CampaignManager::MaxMapDeathsForTier`がWaystoneのTierに応じてそのアタempトの最大ポータル数(死亡許容回数)を決める(Tier1-3=6、4-6=5、7-9=4、10-12=3、13-15=2、`OpenEndgameMap`が`m_mapDeathsMax`/`m_mapDeathsRemaining`へ反映)。拠点側は`ZoneBuilder::Build`が`CampaignManager::HasActiveMapAttempt`/`GetMapDeathsRemaining`を見て、進行中のアタempトがあればマップデバイス中心を囲む円状に残りポータル数ぶんの入場口を配置し(`ZoneBuilder::SpawnReturnPortalRing`)、どれをクリックしても同じマップへ無料で再入場する(`GameScene::m_portalMarkers`、無ければ従来通り単体のAtlas起動ポータル)。マップ内(Combatゾーン)では画面下部のスキルスロット列の右隣に帰還用ポータル生成ボタンがあり(`UISystem::PortalButtonRect`で位置を共有、`UISystem::Render`のshowPortalButton引数)、クリックすると詠唱(`GameScene::kPortalChannelDuration`=1.5秒)が始まり、詠唱中にHPが減る(被弾/床ギミック等)とキャンセルされる(`GameScene::Update`が前フレームのHPと比較)。完了するとプレイヤー位置に`MapReturnPortalTag`持ちのポータルが出現する(`GameScene::SpawnMapReturnPortalAtPlayer`)。同じ関数がゾーン内のRare敵+ボスを全滅させた最初のフレームにも呼ばれる(`GameScene::m_notableEnemiesRemaining`/`m_notablePortalSpawned`、1体討伐ごとではなく全滅後に1回だけ)。どちらもクリックで`GameScene::ReturnToHubViaPortal`(`CampaignManager::ReturnToLastTown`のみを呼び`CompleteCurrentZoneAndAdvance`を経由しない=マップアタempト自体は継続、死亡でもクリアでもない一時帰宅)。死亡時は既存の`ConsumeMapDeath`が別途1減算する。
- **ミニマップ/Tab全体マップ**: `External/ECS/Systems/World/MinimapSystem.h`。`MapComponent::visited`(`External/ECS/Components/World/Map.h`)にプレイヤーが歩いて可視範囲に入ったタイルだけを記録し(`RevealCircle`)、画面右上の常時表示ミニマップと、Tabキーで開く全画面の半透明オーバーレイ(矢印キーでパン)の両方をそこから描画する(未探索は真っ暗)。Tabの開閉自体はPassiveTree/Atlas等と同様`GameScene::Update`が他の全画面メニューと排他制御し、開いている間は`isPaused`に含めてワールド/プレイヤー移動を止める(矢印キーが移動入力と衝突しないようにするため)。
- **床属性ギミック（燃焼床/雷の床/氷の床/混沌ダメの床、タイル属性）**: `HazardGroundKind`(`External/ECS/Components/World/HazardGround.h`)は円形エンティティではなく`MapComponent::hazardTiles`(タイル単位、`Map.h`の`GetHazard`/`SetHazard`)に焼き込む地形情報。`ZoneBuilder::PlaceHazardGrounds`がCombatゾーン生成時にDirtタイル上へランダムウォークで数タイル分のパッチとして塗る(開始地点付近は除外、タウンには塗らない)。判定は`HazardGroundSystem`がキャラクターの現在タイルを見るだけで済み(距離計算不要)、効果の強さはタイル自体にデータを持たず`CampaignManager::GetEndgameMapTier`から都度算出する。燃焼床/混沌ダメの床は`CombatMath::ApplyDamageOverTime`で直接HPを削り(混沌はESを無視)、雷の床/氷の床は下記のElemental Ailment Threshold経由でShock/Chillを間接的に更新する。描画は`HazardGroundRenderSystem`が`MapRenderSystem`と同じくカメラ可視範囲のタイルだけを1本の`sf::VertexArray`にまとめて1回のdraw callで描く。
- **元素耐性によるIgnite/Freeze/Shockの発症遅延(Elemental Ailment Threshold、PoE2参考)**: `StatusEffectsComponent`の`igniteBuildup`/`freezeBuildup`/`shockBuildup`に、耐性で軽減された後のダメージだけを蓄積し(`CombatMath::AccumulateIgnite`/`AccumulateFreeze`/`AccumulateShock`)、対象の最大HPの25%(`kAilmentThresholdFraction`)に達した時点で発症・蓄積リセットする。蓄積は未発症の間ずっと減衰する(`CombatMath::DecayAilmentBuildup`、`StatusEffectSystem::Update`が毎フレーム呼ぶ)ため、耐性が高いほど1ヒット/1フレームあたりの蓄積量が減り閾値到達=発症が遅くなる("耐性で遅らせる")。クリティカルヒットは従来通り即発症。Chillそのもの(スロウ)は従来通りヒットで即時付与、Bleed(Physical)/Poison(Chaos)は対象外(従来の`RollAilmentChance`のまま)。床属性ギミック(雷の床/氷の床)もこの同じ蓄積関数へ毎フレームの疑似ダメージを流し込むことで、戦闘のヒットと全く同じ「耐性で遅らせる」ロジックを共有する(`HazardGroundSystem`)。
- **マップ描画（タイル）**: `External/ECS/Systems/World/MapRenderSystem.h`。各`TileType`の見た目は`External/ECS/Systems/World/TileTextureFactory.h`が起動時に1回だけ手続き的に生成する256x256のノイズテクスチャ(床=斑点/壁=レンガ目地/木材=板目)で、手描きのタイル素材が無い代替(`Assets/Textures/`には背景/タイトル用の画像のみ存在)。1マスごとに同じアトラスの異なる256x256内オフセット(タイル座標から決定論的に算出)を切り出して貼ることで、同種タイルが並んでも同じ絵の繰り返しに見えないようにしている。
- **戦闘計算（命中率/Armour軽減/属性耐性/ES/Leech/状態異常発生率）**: `External/ECS/Systems/Combat/CombatMath.h`
- **状態異常（Ignite/Chill/Freeze/Shock/Poison/Bleed/Stun）**: `External/ECS/Systems/Combat/StatusEffectSystem.h`, `External/ECS/Components/Combat/StatusEffects.h`
- **元素攻撃モンスター**: `ZoneBuilder::SpawnTrash`が(Rareの個別ロールとは別に)まだPhysicalのままの個体へ追加で`stats.contactDamageType`をFire/Cold/Lightningへ変える(遠距離アーケタイプは60%、それ以外は25%、色/名前接頭辞も変える)。既存の`stats.contactDamageType`は接触ダメージ(`CollisionSystem`)・範囲攻撃(`EnemyAreaAttackSystem`)・遠距離投射物(`EnemyRangedAttackSystem::proj.damageType`)すべてが共通して参照しているため追加実装は不要だった。`EnemyRangedAttackSystem`の弾自体には元々見た目(`SparkVisualComponent`)が無かった(不可視のバグ)ため合わせて追加し、`ImpactVfx::ElementColor`で属性色に、雷は稲妻描画(`electric=true`)にした。
- **装備（9スロット、affix、PowerScore比較）**: `External/ECS/Systems/Item/EquipmentSystem.h`, `External/ECS/Components/Item/Equipment.h`
- **インベントリUI（Iキー、装備入替/売却/破棄）**: `External/ECS/Systems/UI/InventorySystem.h`, `External/ECS/Components/Item/Inventory.h`
- **アイテム生成（affixロール、レアリティ抽選、売却額計算）**: `External/ECS/Systems/Item/ItemFactory.h`
- **アイテム拾得（インベントリへ格納、満杯時は自動売却）**: `External/ECS/Systems/Item/ItemPickupSystem.h`
- **通貨/クラフト（Transmutation/Regal/Chaos）**: `External/ECS/Systems/Item/CurrencySystem.h`
- **XP/レベリング**: `External/ECS/Systems/Progression/LevelSystem.h`
- **パッシブツリーUI（Pキー、ノード割り振り）**: `External/ECS/Systems/Progression/PassiveTreeSystem.h`, `PassiveTreeData.h`, `External/ECS/Components/Progression/PassiveTree.h`
- **町のNPC（アイテムVendor/Waystone Vendor/Stash、クリックで話しかける）**: `ZoneBuilder::PlaceTownNpcs`が町の生成毎に3体を離れた位置へ配置し、`GameScene`が`ZoneBuildResult::townNpcs`(`TownNpcKind`+座標、`Programs/Game/GameScene/Zone/TownNpc.h`)を見てプレイヤー近接判定・クリック判定・該当UIの開閉を行う(単一Vendorだった旧実装を汎用化)。アイテム売買は`External/ECS/Systems/Item/VendorSystem.h`、Waystone専売は`External/ECS/Systems/Item/WaystoneVendorSystem.h`(ゴールドのみ、Buyback無し、全15ティアをレベルテーブルでゲート、購入したWaystoneは通常アイテムとしてバッグへ入る)、アイテム保管庫は`External/ECS/Systems/UI/StashSystem.h`(`External/ECS/Components/Item/Stash.h`の`StashComponent`、10x8グリッド×4タブ、タブ切替ボタン、バッグとの双方向ドラッグ、セーブ対象)。
- **スキルジェム（Gキー、5スキルスロット+5スピリットスロット=計10枠をマウスのクリックのみで自由付け替え）**: `External/ECS/Systems/UI/SkillGemSystem.h`。ジェムはWaystone同様`ItemComponent`(`category==ItemCategory::SkillGem`、`skillGemIdentified`/`skillGemIsSupport`/`skillGemUncutKind`/`skillGemId`/`skillGemLevel`/`skillGemMaxSockets`/`skillGemSupportIds[5]`フィールド)として`InventoryComponent`/`StashComponent`に通常アイテムと同じ形式で格納される(専用の所持リストは廃止済み)。ジェムの静的定義は`External/ECS/Systems/Skill/SkillGemData.h`（レベル1-20要件の基礎値/必要属性STR・DEX・INT付き）、サポートジェムの静的定義は`External/ECS/Systems/Skill/SupportGemData.h`。両者の適用・レベルスケーリングは`External/ECS/Systems/Skill/SkillGemScaling.h`(`BuildEquippedSkillData`、装備中アイテムを直接引数に取る)/`SupportGemSystem.h`。ドロップは未鑑定のUncut Gem(`ItemFactory::GenerateUncutSkillGem`、レベル+`GemPickupKind`でSkill/Support/Spiritの種別のみ判定済み)として`ItemPickupComponent`経由で拾う(Waystoneと同じ経路)。`InventorySystem`(Iキー)でUncut Gemアイテムを右クリックすると`External/ECS/Systems/UI/GemIdentifySystem.h`(Gem Cutting)が開き、選んだジェムでそのアイテム自身が同じバッグ位置のまま識別済みに変わる(サポートジェムも同じ経路が必須、カタログからの自由装着は不可)。`SkillGemSystem`(Gキー)はスロット選択→バッグ内の識別済みジェムをピッカーから選んで装備(バッグ⇔`PlayerSkill::equippedItems[5]`/`SpiritGemLoadoutComponent::items[5]`間で実体移動、`EquipmentComponent`のドールスロットと同じ方式)。ソケット拡張はJeweller's Orb(`CurrencyType::JewellersOrb`、`CharacterStatsComponent::jewellersOrbs`)をSkillGemSystem画面上のボタンで消費。実発動時の可否判定は`External/ECS/Systems/Skill/SkillActivationSystem.h`の`CanUseSkill`に一元化(`SkillSystem::Update`が使用)。画面右上の「All Skills」ボタンで、所持の有無に関わらず全スキル/サポートジェムを一覧できる読み取り専用の一覧モード(`SkillGemSystem::RenderCodex`)に切り替えられる(人間向け資料`Docs/スキル一覧.md`とは別に、ゲーム内でも一覧できるようにしたもの)。
- **Permanent Minion（Spirit Gemの一種、`SkillBehaviorType::Minion`）**: `External/ECS/Systems/Chara/MinionSystem.h`が召喚体の追従/近接AI・死亡検知・リスポーンを管理。スポーン/デスポーン自体は`SpiritAuraSystem`のON/OFFがトリガー(`EntitySpawner::CreateMinion`で生成)。マーカーは`External/ECS/Components/Chara/Minion.h`(`AllyTagComponent`=プレイヤー陣営、`PermanentMinionComponent`=所有者/スロット参照)。
- **アイテムUI共通ヘルパー（スロット名/レアリティ名・色、グリッド配置）**: `External/ECS/Systems/UI/ItemUIHelpers.h`（CharacterSheetSystem/InventorySystem/VendorSystem/StashSystem/AtlasSystemが共有）。グリッド内の移動は`TryMoveWithinGrid`(同一グリッド内、ドロップ位置への移動または同サイズ入れ替え)/`TryMoveBetweenGrids`(別グリッド間、同じくドロップ位置をそのまま使う)が常に実際のドロップ先セルへ配置する(`FindBagFreeSpace`は新規追加アイテム(拾得/購入/クイック装備解除)の自動配置専用で、既存アイテムのドラッグ移動には使わない、という役割分担)。
- **常時表示HUD（HP/MP オーブ、Energy Shieldリング、スキルスロット、デバフアイコン）**: `External/ECS/Systems/UI/UISystem.h`。画面左下にHP、右下にMPのオーブ(`DrawOrb`)。Energy Shieldは`CharacterStatsComponent::maxES > 0`の時だけ、PoE2本家同様HPオーブの縁を囲む光るリング(`DrawShieldRing`、12時位置から`current/max`比率ぶん時計回りに描く、SFMLに円形プログレスの標準プリミティブが無いため`sf::VertexArray`のTriangleStrip/LineStripで自前生成)として表示する(ESは非Chaosダメージを先に吸収する、`CombatMath::ApplyDamage`参照)。
- **キャラクターシートUI（Cキー）**: `External/ECS/Systems/UI/CharacterSheetSystem.h`
- **敵のアグロ/リーシュ判定（一定範囲で検知、離れると待機に戻る）**: `External/ECS/Components/Chara/EnemyAIState.h`(`EnemyAIStateComponent`、`EntitySpawner::CreateEnemy`で雑魚・ボス両方に付与)を`External/ECS/Systems/Chara/EnemyAISystem.h`が`Update`冒頭で判定し、未検知/検知範囲外なら各アーキタイプ別の移動ロジック(Ranged/Charger/AreaAttack/通常追跡)自体をスキップする。
- **ボス部屋（ボスは専用の広い部屋で戦う）**: `Programs/Game/GameScene/MapGenerator/MapGenerator.h`の`CarveBossRoom`(プレイヤー開始地点から最遠のDirtタイル周辺を正方形に上書きして専用の部屋を作る)を`Programs/Game/GameScene/Zone/ZoneBuilder.h::Build`がボスゾーンの時だけ呼び、その部屋の中心をボスの湧き位置(`goalPos`)にする。雑魚の湧き候補からも部屋の範囲を除外する。
- **着弾時のVFX(色付き爆発バースト)**: `External/ECS/Systems/Combat/ImpactVfx.h`(`SpawnHitBurst`)。プレイヤー/敵の攻撃が命中した瞬間に属性色の小さな爆発を出す(`CollisionSystem`/`EnemyAreaAttackSystem`から呼ばれる、既存の`HitFlashComponent`(白フラッシュ)に加えての演出)。
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
