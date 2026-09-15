# ROADMAP

2D ARPG (PoE2-style) — Act1-4 / Interludes / Endgame を目指す垂直スライス開発のロードマップ。

## 済 (Done)

### 基盤
- キーバインド設定: `KeyBindings`(シングルトン)が移動/ロール/スキル5スロット/UIパネルトグル/Vendorリロールの計16アクションのキー割り当てを保持し`keybinds.cfg`に永続化。各システムは`sf::Keyboard::Key`リテラルを直接見ず`KeyBindings::Instance().Get(GameAction::X)`経由で参照するよう統一(移動はWASD側のみ対応、矢印キーのフォールバックとメニュー内カーソル移動は従来通り固定)。Oキーで`KeyBindSystem`(リバインドUI)を開閉、Up/Downで対象選択、Enterで次に押したキーを割り当て(既に別アクションで使用中のキーやUI固定キーへの割り当ては拒否)。各パネルのヘルプテキスト(「_ to close」等)も現在のバインドを動的表示するよう変更。
- ECSパフォーマンス改善: `ComponentPool` をスパースセット化 (O(1) Insert/Remove/Get/Has)、`Registry` の `ComponentTypeId` を typeid ハッシュ廃止で静的カウンタ化、`View<>` をdense配列イテレーションに変更。
- キャンペーン構造: `CampaignManager` で Act1〜4 + 幕間2つ + Endgame をゾーン単位で定義 (`ZoneDefinition`/`ActDefinition`)。タウン/戦闘ゾーン/ボスゾーンの遷移、`ZoneBuilder`/`MapGenerator` (タウン生成含む) と統合。
- タイトル/リザルト画面: セーブの有無による Continue/New Game 分岐、Hardcore選択 (H キー)。

### 戦闘・キャラクター
- モンスターの遠距離攻撃アーケタイプ: `SpawnTrash`で雑魚モンスターの20%に`RangedAttackerComponent`を付与。`EnemyAISystem`は保持中このコンポーネント持ちを検知するとプレイヤーへ直進せず、近すぎれば離れ(kite)・遠すぎれば詰め・射程内なら停止して距離を保つ。`EnemyRangedAttackSystem`が射程内でクールダウン経過時にプレイヤーへ向けてプロジェクタイル(`ProjectileComponent::isEnemy=true`)を発射。`CollisionSystem`にプロジェクタイル対プレイヤーの当たり判定を新設(従来は接触ダメージのみで、敵の飛び道具がプレイヤーに当たる経路が存在しなかった)。
- モンスターの範囲攻撃(叩き潰し)アーケタイプ: `SpawnTrash`で雑魚モンスターの15%に`AreaAttackerComponent`を付与(遠距離アーケタイプとは排他、合計35%がどちらか一方)。`EnemyAISystem`は保持中このコンポーネント持ちを検知すると`triggerRange`まで直進で詰め、範囲内に入ると停止してテレグラフ(予兆)に入る。`EnemyAreaAttackSystem`がテレグラフ中は`CircleComponent`の色を赤く点滅させ、`telegraphTime`経過時点でプレイヤーが`radius`内にいればダメージ(`CombatMath::ApplyDamage`+状態異常抽選、既存の接触ダメージ処理と同じ経路)を与えてクールダウンへ。**このセッションでは環境のコンパイラ制約(VS2019ではv143必須プロジェクトがwinrtヘッダでICE)によりビルド未検証。実機でのビルド・動作確認が必要。**
- モンスターの召喚アーケタイプ: `SpawnTrash`で雑魚モンスターの10%に`SummonerComponent`を付与(遠距離/範囲攻撃とも排他、3アーケタイプ合計45%)。移動/接近は通常の近接チェイスのまま、`EnemySummonSystem`がプレイヤーが`triggerRange`内にいる間`summonInterval`毎に自身のHP/ATKの50%の雑魚を周囲にスポーンし(`EntitySpawner::CreateEnemy`を流用)、`maxActiveSummons`体まで同時生存を許容(死亡/無効化した召喚体は毎フレーム`activeSummonIds`から除去して上限判定に反映)。**ビルド未検証(同上の理由)。EnemySummonSystem単体は`Systems/Chara/EnemySummonSystem.h`としてisolatedなcl.exe構文チェックでエラー無しを確認済み。**
- PoEライクな戦闘計算 (`CombatMath`): 命中率 (Accuracy vs Evasion)、Armour非線形軽減、属性耐性%、エナジーシールド (Chaos貫通)、Leech (レート上限あり)、状態異常発生率。
- 状態異常システム (`StatusEffectSystem`/`StatusEffects.h`): Ignite/Chill/Freeze/Shock/Poison(スタック)/Bleed/Stun。HUDにアイコン表示済み (`UISystem::DrawDebuffIcons`)。
- モンスターレアリティ (Normal/Magic/Rare/Unique) と接触属性ダメージのバリエーション、Act/幕間ごとの属性テーマ付け。
- ボスフェーズ (`BossPhaseSystem`): HP50%でEnrage (攻撃力/移動速度上昇、カメラシェイク)。
- 被弾VFX (ヒットフラッシュ)、死亡時バースト、カメラシェイク (`CameraManager::Shake`)。

### アイテム・進行
- 装備システム: 9スロット、Prefix/Suffix affix、レアリティ4段階、PowerScoreによる比較 (`EquipmentSystem`)。
- インベントリ管理UI (`InventorySystem`, Iキーで開閉): 拾得アイテムは`InventoryComponent`(24枠)へ格納。Up/Downで選択、Enterで装備(既存装備は同じ枠に戻る=入れ替え)、Xで売却、Delで破棄。選択中アイテムと該当スロットの現装備のPowerScoreを比較表示。満杯時は自動売却にフォールバック。セーブ/ロード対応 (`CampaignManager`)。
- 通貨/クラフト: Transmutation/Regal/Chaos/Alchemy/Augmentation/Annulment/Chance/Scouring を装備済みアイテムに自動適用 (`CurrencySystem`)。各通貨ごとの対象条件 (`FindEligibleSlot`) を満たす最初の装備スロットへ即座に効果を適用する設計 (所持・選択UIは無し)。`CurrencyType`に`Count`番兵を追加し、ドロップ抽選 (`CollisionSystem`) がハードコードした範囲ではなく`CurrencyType::Count`から動的に算出するよう修正 (新規追加時の抜け漏れ防止)。Artificer's Orb (Socket追加) はSocket/Runeシステム自体が未実装のため、Vaal Orb/Mirror of KalandraはリスクリワードUI設計が必要なため今回は見送り。ビルド未検証(環境制約は下記)だが、`CurrencySystem.h`単体はcl.exe構文チェックでエラー無しを確認済み。
- XP/レベリング: 指数カーブ。レベルアップでランダム自動付与だったパッシブ廃止 (`PassiveSystem`削除)、代わりにレベルアップ毎に1ポイント獲得しパッシブツリーUI (`PassiveTreeSystem`, Pキーで開閉) で手動選択。中央スタートノードから4方向(攻撃/防御/速度・マナ/耐性)へ5ノードずつ伸びるグラフ構造 (`PassiveTreeData`)。矢印キーでノード間移動、Enterで隣接済みノードのみ割り振り可能。`baseStats`/`live`分離によりレベル・XP・パッシブが装備変更で消えないよう修正済み（ゾーン遷移時に`equipment.baseStats`が未復元だったバグも合わせて修正）。
- NPC/Vendor(商人)システム: タウン中央に商人NPCを配置 (`ZoneBuilder::SpawnVendor`)。近づいてBキーで開く`VendorSystem`から、プレイヤーレベルに応じて生成された在庫6点(訪問毎に再生成、購入で減る)をゴールドで購入しインベントリへ追加できる。買値はSellValueの3倍。売却は引き続きインベントリUI側で行う。在庫はTキーでゴールドを払ってリロール可能 (`VendorSystem::TryReroll`、価格は`RerollPrice(playerLevel) = 20 + level*4`)。在庫が売り切れていてもリロールは可能。
- ゴールド経済: 拾った弱い装備を自動売却。
- キャラクターシートUI (`CharacterSheetSystem`, Cキーで開閉)。
- スキルジェムシステム: PoE2のように5スキルスロットを自由に(コスト無し・いつでも)付け替え可能にした。`SkillGemData`に全スキルのプリセットを定義(既存4種 + 新規War Cry/Nova)、モンスターがドロップする`SkillGemPickupComponent`を拾うと`SkillGemInventoryComponent::unlockedGemIds`に恒久追加(重複取得は無視)。スキルジェムUI (`SkillGemSystem`, Kキーで開閉) で各スロット(E/Q/R/V/F)にUp/Downで移動しLeft/Rightで割り当てジェムをサイクル切替(Emptyも選択可)。開始時は既存4スキル(Spark/Thunder Slam/Lightning Warp/Lightning Ball)を所持、5枠目は空。セーブ/ロード対応(`unlockedGemIds`と5スロットの割り当てgemId)。旧セーブとの互換のため、未保存(空)ならEntitySpawnerのデフォルト構成を維持。
  - 新規スキル: **War Cry** (`SkillBehaviorType::Buff`、自己バフでatk+30%/moveSpeed+25%を6秒間)。**Nova** (`SkillBehaviorType::AreaEffect`、自分中心の即時範囲ダメージ)。両behaviorTypeは元々定義のみで未実装だったものを今回実装。
  - バフの安全な解除: `StatusEffectsComponent`に`buffRemaining`/`buffAtkMult`/`buffSpeedMult`を追加。持続時間経過時は乗算を逆算せず`EquipmentSystem::RecalculateStats`を呼び直して装備/パッシブ基準に再計算することで、バフ中にレベルアップ等でRecalculateStatsが走ってもステータスが壊れないようにした。
  - 副作用の修正: Lightning Warpキル時のマナ全回復+クールダウンリセットが`skills[2]`固定インデックス参照だったのを、スロット自由化に伴い`behaviorType`一致検索に変更。
- パッシブツリーのrespec: `CurrencyType::Regret` (Orb of Regret) を追加。ドロップ/`CurrencySystem::ApplyCurrency`経由で入手すると`CharacterStatsComponent::regretOrbs`に加算 (装備には触れない)。パッシブツリーUIでBackspaceキーを押すと、選択中の割り振り済みノードをオーブ1個消費して解放 (`PassiveTreeSystem::TryDeallocate`)。他の割り振り済みノードがスタートノードから到達不能にならないかBFSで検証 (`CanRemoveWithoutDisconnecting`) してから解放するため、途中のノードだけを抜いて枝を分断することはできない。ステータスへの適用は`EquipmentSystem::ApplyAffix`の逆演算`RemoveAffix`で行う。セーブ/ロード対応 (`regretOrbs`)。

### 永続化
- ローカルファイルセーブ/ロード (`CampaignManager::SaveToDisk/LoadFromDisk`): キャンペーン進行 + ステータス + 装備。
- Hardcoreモード: 死亡でセーブ削除 (permadeath)。

### エンコーディング対応
- UTF-8 BOM付与 (新規日本語コンテンツファイル) + `CampaignManager.cpp` のみ `/utf-8` per-file指定。
- 新規システム (Item/Currency/UI等) のテキストは原則英語表記として文字化けリスクを回避。

## 未 (Todo / 次にやること)

優先度順（上ほど先）:

1. **実機での目視検証** — 今セッション後半 (アイテム/装備システム以降、インベントリUI・パッシブツリーUI・Vendor含む) は分離テスト・ビルド成功のみでの検証。実際にプレイしての確認が必要。
2. **サウンド** — 効果音・BGMが `test.mp3` 以外未整備 (アセット不足によりブロック中)。
3. **スキル/モンスターバリエーション拡充(継続)** — スキルジェムの自由付け替え、War Cry/Nova、モンスターの遠距離攻撃/範囲攻撃/召喚アーケタイプを実装済み(下記「済」参照)。さらにジェム種類・モンスター行動パターンを増やす余地あり。
4. **原因不明の間欠的クラッシュの再調査** — セッション前半で発生し、ASan 6分ストレステストでは未検出。実機デバッガでの再現待ち。**有力な原因候補を発見・修正済み**: `InventorySystem`で装備/売却/破棄によりインベントリの最後の1個が無くなる瞬間、`std::clamp(m_selectedIndex, 0, size()-1)`が`size()==0`で`std::clamp(x, 0, -1)`(lo>hi)という未定義動作を踏んでいた。MSVC DebugビルドのSTLはlo>hiを`_STL_ASSERT`で検出しうるため、ASanでは検出されず(UBSanではなくASanのため)実機デバッガでのみ再現するクラッシュ、という特徴と一致する。`ClampSelection()`ヘルパーに切り出しempty時は0にフォールバックするよう修正。ビルド未検証のため実機での再発確認が必要。

## 既知の制約

- ~~自作ECSは高速化したが、まだ簡易実装 (Increased/More修飾はダメージ計算上簡略化されている)。~~ → 修正済み。`CharacterStatsComponent`に`increasedAttackDamage`/`increasedMoveSpeed`の合算専用フィールドを追加し、`EquipmentSystem::ApplyAffix`(装備)・`PassiveTreeSystem`経由の呼び出し(パッシブ)とも同系統%は加算するだけに変更、`RecalculateStats`の最後で1回だけ`atk`/`moveSpeed`に掛ける方式(PoEのIncreased/Reduced合算方式)へ修正。あわせて`leechRateCap`が0.02(2%/秒)になっていた数値ミスを0.20(本家PoE2準拠)へ修正。いずれもビルド未検証(環境制約は下記)だが、`EquipmentSystem.h`単体はcl.exe構文チェックでエラー無しを確認済み。あわせて`CampaignManager`の`WriteStats`/`ReadStats`に新設2フィールドのシリアライズを追加(これを忘れるとセーブ&ロードでパッシブの%系ボーナスが消える回帰バグになるところだった)。`CampaignManager.cpp`は`/utf-8`込みでcl.exe構文チェック済み。
- `KeyBindSystem::IsReserved`にOキー(キーバインド画面自体の固定トグルキー)が含まれておらず、何らかのアクションをOへ再割り当てすると同時に両方トリガーされる不具合を発見・修正。予約キー一覧に追加。
- `InventorySystem`同様のUB(`std::clamp(idx,0,size-1)`をsize==0で呼ぶ)が他のUI(SkillGemSystem/CharacterSheetSystem/KeyBindSystem/VendorSystem)には無いことを確認済み(VendorSystemは元々empty時に0へフォールバックする実装済みだった)。
- **`MovementSystem`を削除(実プレイに影響するバグを発見)**: プレイヤーの移動は`InputSystem`が既に入力→正規化8方向ベクトル→`VelocityComponent`まで一貫して計算していたが、その後段で`MovementSystem`が`input.moveLeft`/`moveRight`のみを見てX速度を`±moveSpeed`(非正規化)で再上書きし、Y速度はInputSystemの正規化済みの値を残す、という中途半端な二重処理になっていた。結果、斜め移動時にX成分だけ非正規化された値になり、実効速度が正規化時より最大約1.22倍速くなる(例: moveSpeed=50なら理論上50のところ実測≈61.2)バグが常時発生していた。`PhysicsSystem`は`VelocityComponent`を持つ全エンティティの位置積分を汎用的に行うため`MovementSystem`はプレイヤー専用の冗長な処理でしかなく、他に依存箇所も無かったため、ファイルごと削除(`GameScene.h/.cpp`・`Game-SFML.vcxproj`からも参照除去)。ビルド未検証、実機での移動フィーリング再確認が必要。
- **Spark/Thunder Slam(GroundSlam)/Lightning Ballのダメージ計算式を修正**: この3スキルだけ`SkillSystem::ActivateSkill`が`proj.damage = stats.atk`(atk全量そのまま)としており、`SkillGemData`側の`damage`値(spark=25/slam=120/ball=40、いずれも他スキルと同じ「% of atk」の想定値)が完全に無視されていた。Melee/Projectile/AreaEffect/LightningWarpは`stats.atk * (skill.damage/100.0f)`と正しく%スケーリングしており計算式が不統一だった。ユーザー確認の上、この3スキルも同じ%スケーリング式に統一(体感火力は下がる: 例えばSparkは1発あたり従来の25%に低下)。`SkillGemData.h`に「% of atk」の注記を追記。ビルド未検証、実機でのバランス確認が必要。
- ユーザーの他アプリ (Apex Legends / Citra) 使用中は、キーボード状態をグローバルにポーリングする入力方式 (`sf::Keyboard::isKeyPressed`) の都合上、安全な自動入力での実機テストができない制約がある。
