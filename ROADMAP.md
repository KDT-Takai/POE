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
- 装備システム: 9スロット、Prefix/Suffix affix、レアリティ4段階、PowerScoreによる自動装備判定 (`EquipmentSystem`)。
- 通貨/クラフト: Transmutation/Regal/Chaos を装備済みアイテムに自動適用 (`CurrencySystem`)。
- XP/レベリング: 指数カーブ、レベルアップ時ランダムパッシブ付与 (11種プール、`PassiveSystem`)。`baseStats`/`live`分離によりレベル・XPが装備変更で消えないよう修正済み。
- ゴールド経済: 拾った弱い装備を自動売却。
- キャラクターシートUI (`CharacterSheetSystem`, Cキーで開閉)。

### 永続化
- ローカルファイルセーブ/ロード (`CampaignManager::SaveToDisk/LoadFromDisk`): キャンペーン進行 + ステータス + 装備。
- Hardcoreモード: 死亡でセーブ削除 (permadeath)。

### エンコーディング対応
- UTF-8 BOM付与 (新規日本語コンテンツファイル) + `CampaignManager.cpp` のみ `/utf-8` per-file指定。
- 新規システム (Item/Currency/UI等) のテキストは原則英語表記として文字化けリスクを回避。

## 未 (Todo / 次にやること)

優先度順（上ほど先）:

1. **実機での目視検証** — 今セッション後半 (アイテム/装備システム以降) は分離テスト・ビルド成功のみでの検証。実際にプレイしての確認が必要。
2. **インベントリ管理UI** — 現状は自動装備/自動売却のみ。手動でのアイテム比較・入れ替え・破棄ができない。
3. **パッシブツリーUI** — 現状はレベルアップ時ランダム自動付与のみ。PoEらしい選択制のツリーUIが未実装。
4. **NPC/Vendor(商人)システム** — タウンでの購入/売却が未実装 (ゴールドは貯まるが使い道がまだ無い)。
5. **サウンド** — 効果音・BGMが `test.mp3` 以外未整備 (アセット不足によりブロック中)。
6. **スキル/モンスターバリエーション拡充** — 現状のスキル数・敵種類は最小限。
7. **原因不明の間欠的クラッシュの再調査** — セッション前半で発生し、ASan 6分ストレステストでは未検出。実機デバッガでの再現待ち。

## 既知の制約

- 自作ECSは高速化したが、まだ簡易実装 (Increased/More修飾はダメージ計算上簡略化されている)。
- ユーザーの他アプリ (Apex Legends / Citra) 使用中は、キーボード状態をグローバルにポーリングする入力方式 (`sf::Keyboard::isKeyPressed`) の都合上、安全な自動入力での実機テストができない制約がある。
