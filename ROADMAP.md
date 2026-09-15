# ROADMAP

2D ARPG (PoE2-style) — Act1-4 / Interludes / Endgame を目指す垂直スライス開発のロードマップ。

## 済 (Done)

### 基盤
- ECSパフォーマンス改善: `ComponentPool` をスパースセット化 (O(1) Insert/Remove/Get/Has)、`Registry` の `ComponentTypeId` を typeid ハッシュ廃止で静的カウンタ化、`View<>` をdense配列イテレーションに変更。
- キャンペーン構造: `CampaignManager` で Act1〜4 + 幕間2つ + Endgame をゾーン単位で定義 (`ZoneDefinition`/`ActDefinition`)。タウン/戦闘ゾーン/ボスゾーンの遷移、`ZoneBuilder`/`MapGenerator` (タウン生成含む) と統合。
- タイトル/リザルト画面: セーブの有無による Continue/New Game 分岐、Hardcore選択 (H キー)。

### 戦闘・キャラクター
- PoEライクな戦闘計算 (`CombatMath`): 命中率 (Accuracy vs Evasion)、Armour非線形軽減、属性耐性%、エナジーシールド (Chaos貫通)、Leech (レート上限あり)、状態異常発生率。
- 状態異常システム (`StatusEffectSystem`/`StatusEffects.h`): Ignite/Chill/Freeze/Shock/Poison(スタック)/Bleed/Stun。HUDにアイコン表示済み (`UISystem::DrawDebuffIcons`)。
- モンスターレアリティ (Normal/Magic/Rare/Unique) と接触属性ダメージのバリエーション、Act/幕間ごとの属性テーマ付け。
- ボスフェーズ (`BossPhaseSystem`): HP50%でEnrage (攻撃力/移動速度上昇、カメラシェイク)。
- 被弾VFX (ヒットフラッシュ)、死亡時バースト、カメラシェイク (`CameraManager::Shake`)。

### アイテム・進行
- 装備システム: 9スロット、Prefix/Suffix affix、レアリティ4段階、PowerScoreによる比較 (`EquipmentSystem`)。
- インベントリ管理UI (`InventorySystem`, Iキーで開閉): 拾得アイテムは`InventoryComponent`(24枠)へ格納。Up/Downで選択、Enterで装備(既存装備は同じ枠に戻る=入れ替え)、Xで売却、Delで破棄。選択中アイテムと該当スロットの現装備のPowerScoreを比較表示。満杯時は自動売却にフォールバック。セーブ/ロード対応 (`CampaignManager`)。
- 通貨/クラフト: Transmutation/Regal/Chaos を装備済みアイテムに自動適用 (`CurrencySystem`)。
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
3. **モンスターの遠距離攻撃アーケタイプ追加** — 現状は全モンスターが接触ダメージのみ。プレイヤーに向けて周期的にプロジェクタイルを発射する敵タイプを追加する(スキルジェムの自由付け替え・War Cry/Novaは実装済み、下記「済」参照)。
4. **原因不明の間欠的クラッシュの再調査** — セッション前半で発生し、ASan 6分ストレステストでは未検出。実機デバッガでの再現待ち。

## 既知の制約

- 自作ECSは高速化したが、まだ簡易実装 (Increased/More修飾はダメージ計算上簡略化されている)。
- ユーザーの他アプリ (Apex Legends / Citra) 使用中は、キーボード状態をグローバルにポーリングする入力方式 (`sf::Keyboard::isKeyPressed`) の都合上、安全な自動入力での実機テストができない制約がある。
