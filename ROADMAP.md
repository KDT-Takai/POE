# ROADMAP

2D ARPG (PoE2-style) — Act1-4 / Interludes / Endgame を目指す垂直スライス開発のロードマップ。

## 済 (Done)

### 基盤
- キーバインド設定: `KeyBindings`(シングルトン)が移動/ロール/スキル5スロット/UIパネルトグル/Vendorリロールの計16アクションのキー割り当てを保持し`keybinds.cfg`に永続化。各システムは`sf::Keyboard::Key`リテラルを直接見ず`KeyBindings::Instance().Get(GameAction::X)`経由で参照するよう統一(移動はWASD側のみ対応、矢印キーのフォールバックとメニュー内カーソル移動は従来通り固定)。Oキーで`KeyBindSystem`(リバインドUI)を開閉、Up/Downで対象選択、Enterで次に押したキーを割り当て(既に別アクションで使用中のキーやUI固定キーへの割り当ては拒否)。各パネルのヘルプテキスト(「_ to close」等)も現在のバインドを動的表示するよう変更。
- ECSパフォーマンス改善: `ComponentPool` をスパースセット化 (O(1) Insert/Remove/Get/Has)、`Registry` の `ComponentTypeId` を typeid ハッシュ廃止で静的カウンタ化、`View<>` をdense配列イテレーションに変更。
- キャンペーン構造: `CampaignManager` で Act1〜4 + 幕間2つ + Endgame をゾーン単位で定義 (`ZoneDefinition`/`ActDefinition`)。タウン/戦闘ゾーン/ボスゾーンの遷移、`ZoneBuilder`/`MapGenerator` (タウン生成含む) と統合。
- **マップ生成を部屋+通路方式へ刷新**: 従来の1タイル幅ランダムウォークから、矩形の部屋(5〜9タイル角、マップ面積に応じて4〜14室)を非重複配置しL字型通路で順に接続する方式(`MapGenerator::GenerateRoomsAndCorridors`)へ変更。部屋を1室も置けない極小マップの場合のみ旧ランダムウォークへフォールバック。開始/ゴールは最初/最後の部屋の中心に設置。**この変更はwinrt依存が無いため唯一実際にコンパイル・リンク・実行して検証済み**: 5サイズ×20試行=計100パターンでStart→Goal間のBFS到達可能性・Wood/Grassタイルが必ず1つずつ・部屋の範囲外はみ出し無し、を全て確認(`ECS.h`本体を経由するフルビルドは環境制約により別途未検証)。
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
- **インベントリ管理UIをPoE2風のペーパードール+グリッドへ刷新、UI文言を日本語化** (`InventorySystem`, Iキーで開閉): 左に体の部位に見立てた装備欄9枠(頭/武器/胴/首飾り/手袋/ベルト/指輪2/靴)、右に24マス(6×4、`InventoryComponent`容量と一致しスクロール不要)の所持品グリッドを表示。クリックで選択して下部の装備(装備中選択時は自動的に「外す」に切替)/売却/破棄ボタンで操作するほか、**ドラッグ&ドロップ**にも対応: バッグ→装備欄(対応スロットのみ)、装備欄→バッグ、バッグ内入れ替え、指輪1⇔指輪2の入れ替え、そして**パネル外へドロップするとその場でアイテムを地面に投棄**(`ItemPickupComponent`として再スポーン、通常のドロップ拾得と同じ経路で再度拾える)。旧キーボード操作(Up/Down/Enter/X/Del)も併存。表示テキストは全て日本語化(`ItemUIHelpers`のスロット名/レアリティ名も含む)、新規Japanese文言を含むため両ファイルとも規約通りUTF-8 BOM付きで保存(VS2022実機ビルドでC4819警告が出ないことを確認済み)。満杯時は自動売却にフォールバック。セーブ/ロード対応 (`CampaignManager`)。**PoE2同様、画面右側にドッキング配置**(中央を塞がずフィールドが見える)し、**インベントリ/キャラクターシートは開いていてもワールドシミュレーション(敵AI・移動・物理・当たり判定等)を止めない**よう変更(パッシブツリー/ベンダー/スキルジェム/キーバインドは引き続き一時停止)。インベントリを開いている間はマウス操作の取り合いを避けるため、地面アイテムのクリック拾得・ホバー名前表示・Skill1の左クリック割当を一時的に無効化。**さらにインベントリ/キャラクターシート/スキルジェムの3画面は互いに閉じ合わずに同時に開けるよう変更**(アイテムを見ながらステータスやジェム装備を確認できる)。パッシブツリー/ベンダー/キーバインドはこれら3画面も含めて全て閉じる排他仕様のまま(自分自身は閉じ対象から除外)。**バグ修正: スキルジェム画面は「非ポーズ」系として扱うと決めていたにも関わらず、`GameScene::Update()`の`isPaused`判定式に`skillGemSystem->isOpen`が残ったままだったため、実際にはスキルジェムを開くとワールドシミュレーションが停止(見た目上ゲームが固まる)する不具合があった。`isPaused`からスキルジェムを除外し、インベントリ/キャラクターシートと同様に開いたまま戦闘・移動を継続できるよう修正。**

**画面レイアウトの最終仕様: 左(キャラクターシート/スキルジェム)・右(インベントリ)の2枠。左枠のキャラクターシートとスキルジェムは排他とし、最後にトグルした方だけを表示するタブ切り替え方式に変更(両方常時表示すると1280x720では情報過多になるため)。右枠のインベントリは左枠と独立に同時に開ける。またこれら非ポーズ系メニューが開いていても、マウスカーソルが実際にそのパネルの矩形へ重なっている時だけスキル発動クリック/アイテム拾得クリックをUI側へ譲るよう変更(`InventorySystem`/`CharacterSheetSystem`/`SkillGemSystem`に`IsPointInPanel()`を追加、`GameScene`側でカーソル位置から判定)。以前は「メニューが開いているかどうか」だけでフィールド上のクリックを丸ごと止めていたため、パネルに重ならない場所をクリックしてもスキルが撃てなかった。**
**注意点: `SkillGemSystem.h`/`CharacterSheetSystem.h`はBOM無しの純ASCIIファイル(規約通り、新規システムのテキストは英語表記)だったため、この変更で追加したコメントは日本語ではなく英語で記述した(誤って日本語コメントを入れるとコードページ932での誤読でビルドが壊れることを実機で確認済み — 詳細は本ファイル内の文字化け根本原因修正の項を参照)。**

**操作競合の修正: インベントリ/スキルジェムはワールドを止めない仕様のため、これらのメニュー内でUp/Down/Left/Right矢印キーによる選択・切替操作を行うと、`InputSystem`側の移動入力(WASDに加え矢印キーでも移動可能)と同時に反応し、メニュー操作のつもりがプレイヤーキャラも一緒に動いてしまう不具合があった。対応として両メニューを完全にマウス操作へ寄せた: `InventorySystem`は元々クリック/ドラッグ&ドロップで完結していたため、重複していたUp/Down/Enter/X/Deleteのキーボード選択パスを削除。`SkillGemSystem`はキーボードでのスロット切替(Up/Down)とジェムサイクル(Left/Right)しか無かったため、PoE2の宝石UIのように「スロット行をクリックして選択→下に表示される所持ジェム一覧から目的のジェムをクリックしてソケット」というマウス完結の操作に全面的に作り直した(`Update()`の当たり判定と`Render()`の描画がズレないよう、両方から共有する`ComputeLayout()`で行位置を一元管理)。パネルサイズが460x680に拡張されたことで一覧表示にも余裕ができた。**
**あわせてスキルジェム画面のトグルキーをKからG(PoE2のデフォルト操作に合わせる)へ変更(`KeyBindings::ResetToDefaults`)。**

**インベントリ操作をPoE2本家準拠へ全面刷新**: 「装備/売却/破棄」の3ボタン(と、それに紐づく「選択状態」の概念)を廃止し、PoE2本家のインベントリ操作に合わせて完全にマウス直感操作化した。
- 左ドラッグ&ドロップ: 装備⇔バッグの移動、バッグ内入れ替え、指輪1⇔指輪2の入れ替え、パネル外へドロップで地面に投棄(従来通り)
- **右クリック: そのスロットに応じて装備/外すを即実行**(PoE2の右クリック=クイック装備/使用を踏襲)
- **Ctrl+左クリック: そのアイテムを即売却**(PoE2のCtrl+クリック=ヴェンダー/倉庫へのクイック移動を、本ゲームでは場所を問わず売却できる仕様([[決定事項]]参照)に合わせてクイック売却として踏襲)
- **カーソルを乗せるとツールチップ表示**: 名前(レアリティ色)/スロット/レアリティ/アイテムレベル/追加効果一覧(値+効果名)/売却額を表示。バッグアイテムの場合は同スロットに装備中のアイテムとのスコア比較も併記(PoE2のホバー比較ツールチップに相当)。画面端でパネル外にはみ出さないようクランプ。
「破棄」専用ボタンは廃止(PoE2本家にも無く、パネル外へドラッグして地面に捨てる操作で代替可能なため)。パネル高さもボタン/フッター分が不要になったため500→400に縮小。
実装中、Writeツールでファイル全体を書き直した際にUTF-8 BOMが失われ(Writeツールの既知の挙動)、日本語を含む同ファイルがコードページ932で誤読されてビルドが壊れる事故が発生(このROADMAPの文字化け根本原因修正の項と同種の問題)。BOMを手動で復元して解決した。

**仕様の訂正: 売却はインベントリ画面からではなくVendor NPCに話しかけて行う方式へ修正**。前項のCtrl+クリック即売却は「売却は場所を問わずインベントリUIで行う」という誤った決定([[AI/DECISIONS.md]]に記載していたもの)に基づいていたが、これはPoE2本家の仕様と異なる(PoE2は町/隠れ家の商人NPCに話しかけて初めて売却できる)との指摘を受け訂正。`InventorySystem`からCtrl+クリック売却(`QuickSellBag`/`QuickSellDoll`)を完全に削除し、代わりに`VendorSystem`(Bキーで話しかけて開くウィンドウ)にBuy/Sellの2タブを追加: Left/Rightキーでタブ切替、Up/Downで選択、Enterで購入/売却を実行。Sellタブは開いている間プレイヤーの`InventoryComponent`を直接参照して一覧表示する(バッグ最大24個を収めるためパネル高さを480→660に拡張)。`AI/DECISIONS.md`の該当記述も修正済み。

**マップ生成を元のランダムウォーク方式に戻した**: 今セッション前半で導入した部屋+通路方式(`GenerateRoomsAndCorridors`)は区画が整い過ぎて狭く感じるとの指摘を受け、`CreateProceduralWorld`の呼び出しを元の`GenerateRandomWalk`(steps = 幅×高さ/2、このセッション以前からの実装)に戻した。`GenerateRoomsAndCorridors`自体は将来使う可能性があるため削除はせず未使用のまま残置。

**Vendor画面をPoE2本家の取引ウィンドウ相当へ全面刷新**: 右にプレイヤーの所持品グリッド(6x4、常時表示)、左に商人側パネル(Buy/Buybackの2タブ、Left/Rightまたはタブをクリックで切替)という構成に変更。売却は右のバッグからアイテムを左パネルへドラッグ&ドロップ、またはCtrl+左クリックで即売却する。**売却したアイテムは消えず「Buyback」タブに一時保管され、売った時と同じ金額で買い戻せる**(誤って売却しても復元できるようにするPoE2の仕様を再現。最大12件保持、超過分は古い順に破棄)。購入/買い戻しはリスト行をクリックするか、従来通りUp/Down+Enterでも操作可能(Vendorはワールドを一時停止する「ポーズ」系画面のため、矢印キーを使ってもプレイヤー移動とは競合しない)。
またNPCへの話しかけ操作もPoE2同様「左クリック」を基本にした: 商人に近づいた状態で商人本体を左クリックするとVendor画面が開く(`GameScene::m_clickedOnVendor`で判定し、同フレームのSkill1発動クリックと競合しないよう抑制する)。従来のBキーはその代替として残した。

**フォローアップ: 「左クリックの判定が分かりづらい」「Bキーがまだ残っている」との指摘を受けて修正**。
- **Bキーでの話しかけを完全に廃止**: `GameAction::VendorToggle`をキーバインド一覧(enum/デフォルト/表示名/セーブキー)ごと削除し、リバインドUIにも出てこないようにした(残しておくと「割り当てても何も起きない」キーになってしまうため)。
- **クリック判定を視覚化**: 商人に近づいている間、ワールド上に商人を中心としたクリック可能範囲の輪(半径36px、`GameScene::kVendorClickRadius`)を常時表示し、カーソルが輪の内側にあるときは黄色く強調表示することで「ここをクリックすれば良い」と分かるようにした。
- **Vendor画面を閉じる手段をB以外に用意**: パネル右上に「Close」ボタンを新設し、クリックで閉じられるようにした(`VendorSystem`のUpdate/Renderに`closeBtnRect`を追加)。あわせてヘッダーの「Bキーで閉じる」表記や、拠点でのHUD表記("Press B to trade")も「Click the merchant to trade」に修正。

**さらにフォローアップ: 「商人のタブと自分のアイテム欄が今は全く同じメニューに見える、分けてほしい」との指摘を受けて修正**。従来は`kPanelW=900`の単一の背景矩形(`sf::RectangleShape bg`)を描いてから内部で左右にコンテンツを配置していたため、実質1枚の連続したパネルに見えていた。CharacterSheet/Inventoryなど他のパネルが「それぞれ独立した箱」であるのと同じ見た目に揃えるため、**商人パネル(Buy/Buyback)と自分の所持品(バッグ)を、それぞれ独自の背景・枠線を持つ別々の矩形(`Layout::leftBox`/`rightBox`、間に24pxの視認できる隙間)として描画し直した**。ヘッダー文言(タイトル/ゴールド/リロール)は商人側の箱の中に、"Your Bag"見出しは自分の所持品側の箱の中に、それぞれ独立して表示するよう変更。当たり判定(ドラッグ売却の判定範囲、リスト行、バッグセル)も新しい2つの矩形基準に更新。

**商人の在庫がプレイヤーレベルに追従するよう修正**: `VendorSystem::GenerateStock`は元々`playerLevel`を`itemLevel`として在庫生成に使っており仕組み自体は正しかったが、`m_stockGenerated`フラグにより初回訪問時に一度生成されたら以降は固定され、レベルが上がってもTキーの有料リロールをしない限り在庫が更新されないバグがあった(序盤で話しかけた商人の在庫が、終盤でも低レベルのまま売られ続ける)。生成時のレベルを`m_stockLevel`として保持し、再訪問時に現在のプレイヤーレベルと異なっていれば無料で再生成するよう変更(同じレベルでの再訪問では在庫は維持され、違うレベルの品を求める場合のみ引き続きTキーの有料リロールが必要)。

**文字化けの再発を根本から潰す: コンパイラオプションをプロジェクト全体の既定に格上げ**: 「文字化けしてる」との再報告を受けて調査した結果、原因はこれまでの個別ファイル対応漏れとは別種だった。`ItemUIHelpers.h`(日本語のスロット名/レアリティ名を返す、BOM付き)は`VendorSystem.h`/`InventorySystem.h`/`CharacterSheetSystem.h`/`ItemPickupSystem.h`という複数のヘッダから`inline`関数としてインクルードされており、それらは最終的に`GameScene.h`経由で**3つの異なる.cppファイル**(`GameScene.cpp`、`ResultScene.cpp`、`TitleScene.cpp`)にそれぞれ翻訳単位として取り込まれていた。`/utf-8`の per-file 指定は`GameScene.cpp`にしか付けていなかったため、`ResultScene.cpp`/`TitleScene.cpp`側の翻訳単位ではコードページ932で日本語リテラルが誤エンコードされ、同一inline関数の実体が翻訳単位ごとに異なるバイト列で生成される(One Definition Rule違反)状態になっていた。リンカがどちらの実体を採用するかはビルドのたびに(オブジェクトファイルの処理順などに依存して)変わりうるため、**同じコードなのに文字化けする/しないが不安定に再現する**という厄介な挙動になっていた。個別ファイルへの`/utf-8`追加を繰り返すのはもぐら叩きなので、`Game-SFML.vcxproj`の4構成(Debug/Release × Win32/x64)全ての`<ClCompile>`に`/utf-8`を既定オプションとして追加し、プロジェクト全体でexecution charsetをUTF-8に統一。これにより「新しいヘッダに日本語を書いたら、それがどの.cppから取り込まれるかによって文字化けするかもしれない」という構造的リスクを解消した(個別ファイル指定は重複するが無害なのでそのまま残置)。

**アイテムにPoE2同様のグリッドサイズを導入し、商人の売買画面もアイテム欄と同じグリッド表示に統一**: 「商人の売っている画面もアイテム欄と同じにしてほしい」「アイテムにサイズを持たせたい(2マスなど)」との要望を受けて実装。
- `ItemComponent`に`gridCol`/`gridRow`(バッグ内の配置座標、-1は未配置)を追加。サイズ自体はアイテムごとに保存せず`ItemUIHelpers::ItemGridSize(EquipSlot)`から都度算出(武器1x3、胴防具2x3、兜/手袋/靴2x2、ベルト2x1、指輪/首飾り1x1、PoE2のインベントリサイズを参考に設定)。
- `ItemUIHelpers.h`にバッグの占有グリッド管理ヘルパーを新設: `BagRegionFree`(矩形が空いているか)、`FindBagFreeSpace`(w×hの空きを走査)、`NormalizeBagPlacement`(未配置アイテムへ自動で空きを割り当て)、`ShelfPack`(配置を持たない一時リスト向けの単純な左上詰めパッキング)。
- `InventorySystem`のバッググリッドを「24マスに1アイテムずつ」から「アイテムごとのサイズ分だけマスを占有する」表示・当たり判定へ全面書き換え。ドラッグ&ドロップは、移動先が空いていれば移動、ちょうど同じサイズの別アイテムがその位置にあれば入れ替え、それ以外(サイズ違いとの重なりなど)は何もせず元の位置に留まる、というシンプルな規則にした(PoE本家のような任意形状の押し出し整理までは実装していない)。装備からバッグへ入れる/バッグから装備する各操作、`ItemPickupSystem`(地面拾得)、`VendorSystem`(購入/買い戻し)も全て`FindBagFreeSpace`ベースの空き判定に統一(旧来の「所持数が24個未満か」という個数ベースの満杯判定を廃止、アイテムの形状によって実際に入るかどうかが決まるようになった)。
- `VendorSystem`のBuy/Buybackリストをテキスト行からアイテム欄と同じ見た目のアイコングリッド(レアリティ色の背景、スロット略称、アイテムレベル、ホバーで名前/価格のツールチップ)に刷新。在庫/買い戻しリストは配置を保存しないため、毎フレーム`ShelfPack`で並び順通りに詰め直して表示・当たり判定を算出する。プレイヤーの所持品側(右の箱)も同じ占有グリッド方式に統一。
- 実装未検証事項: 2x3などの大きいアイテムでバッグが実際に埋まる/入りきらなくなる状況、同サイズ入れ替えドラッグの操作感は実機での確認が必要。

**「商人のところでもじばけしてる」の真因判明・修正**: 前項の「翻訳単位ごとの/utf-8不一致(ODR違反)」説は実在するバグとして修正する価値はあったが、今回ユーザーが実際に見ていた文字化け(□の豆腐文字)の直接の原因は別だった。`VendorSystem.h`のショップ/バッググリッド描画で、スロット名の短縮表示に`ItemUIHelpers::SlotName(item.slot).substr(0, 2)`を使っていたが、`SlotName`が返す日本語文字列(例: "武器")はUTF-8で1文字3バイトのため、`.substr(0, 2)`は**先頭2バイト**を取り出すだけで1文字にも満たない不正なバイト列になり、`sf::String::fromUtf8`が豆腐(□)としてレンダリングしていた。`InventorySystem`側は元々専用の`ShortSlotCode(EquipSlot)`(文字列全体を返す辞書引き、バイト単位の切り詰めをしない)を使っていたため無事だった。`ShortSlotCode`を`ItemUIHelpers.h`へ共通化して`InventorySystem`/`VendorSystem`の両方から使うよう統一し、バイト単位substrによる切り詰めを廃止して解消。**教訓: UTF-8文字列を「先頭Nバイト」で切り詰める操作(`.substr`, 固定長バッファへのコピー等)は多バイト文字の途中で切れて文字化けするため、日本語文字列には使わない。**
検証時のスクリーンショット確認で、PowerShellの`AppActivate`によるゲームウィンドウのフォーカス制御に失敗し、送信したキー入力(移動キー)がフォーカスの外れた別アプリ(Discord)へ渡ってしまう事故があった(メッセージ送信等の実害は確認されていない)。ゲーム外アプリへの影響リスクを認識した時点で自動キー入力操作は中止し、以降はクリップボード上の既存スクリーンショットを確認する方式に切り替えた。

**フォローアップ: 商人のショップグリッドでツールチップが名前を隠す描画順序バグを修正**。文字化け修正後のスクリーンショットで、アイテムにカーソルを乗せたときのツールチップが隣接セルの「Lv」表示等を隠してしまっているのを発見。原因は`VendorSystem::DrawShopGrid`がグリッドの各セルを描く`for`ループの**内側**でツールチップも即座に描画していたため、ループ後半で描かれる隣接セルの矩形が、先に描画済みのツールチップの上に不透明な背景ごと重なって隠してしまっていたこと。`InventorySystem`側は元々ループ中はホバー中アイテムのポインタだけ記録し、ツールチップは全セルを描き終えた**後に1回だけ**描画する設計になっており、この問題が起きなかった。`VendorSystem`も同じ設計(ループ中はhoveredItem/hoveredPriceを保持するだけにし、ループ終了後にツールチップを描画)に合わせて修正。

**自分の所持品欄にも名前/ソケット穴/モッドを表示するよう拡充**。
- **アイテム名の表示**: これまでバッグ/ショップのセルには汎用のスロット短縮コード(「武器」「胴」等)しか表示されておらず、同スロットの別アイテムが見分けられなかった。セルにはアイテム固有の`baseName`(英語、ItemFactory生成のため常にASCIIで安全)をセルの横幅に応じた文字数で切り詰めて表示するよう変更(`InventorySystem`のバッグ/装備欄、`VendorSystem`のショップグリッド/自分のバッグ欄、全て統一)。
- **ソケット穴(見た目のみ、新規)**: `ItemUIHelpers::SocketCount(ItemComponent)`を追加。武器/胴防具/兜/手袋/靴はレアリティに応じて0〜2個(指輪/首飾り/ベルトは常に0、PoE2本家準拠)。`DrawSocketPips`でセル下部に小さな丸(空きソケット)を描画。**注意: ルーン/ジェムをソケットへ挿入する機能は無く、あくまで見た目のみの表示。** ツールチップにも「Sockets: N」の行を追加。
- **モッドの全文表示**: `VendorSystem`のショップツールチップは従来「N mods」という件数だけの表示だったが、`InventorySystem`同様、各affixを「+値 効果名」の形で1行ずつ列挙するよう変更。
- **`VendorSystem`の「自分のバッグ」欄にツールチップが無かった問題を解消**: ショップ側にはツールチップがあったが、右側の自分の所持品グリッドには一切無かった。ショップ側と共通の`DrawItemTooltip`(価格引数を省略すると売却額を表示するオーバーロード的仕様)を自分のバッグのホバーにも適用し、名前/レアリティ/スロット/レベル/ソケット/モッド/売却額を表示するようにした(こちらもツールチップはバッグの全セルを描き終えた後にまとめて描画し、直前に直したのと同じ描画順序バグを再発させないようにしている)。

**フォローアップ: 「modが何のステータスが伸びているのか分からない」との指摘を受けて修正、あわせてPoE2のMod仕様を調査し仕様書に反映**。
- **原因**: `ItemFactory`の`affix.label`が"Increased Attack Damage"等の英語のままで、ステータスシートやインベントリの他表示が日本語化されているのに対し不統一だった。また`+12.0 Fire Resistance`のような表示では、それが%なのか固定値なのか区別できなかった(実際は`EquipmentSystem::ApplyAffix`で耐性/クリティカル/Increased系のみ`/100.0f`されて%として扱われ、Life/Mana/ES/Armour/Evasion/Accuracyは固定値として加算される)。
- **対応**: `ItemFactory`の全Prefix/Suffixラベルを日本語化(例: "Increased Attack Damage"→"攻撃ダメージ増加"、"Fire Resistance"→"火耐性")。`ItemUIHelpers::IsPercentAffix(AffixStat)`を新設し、`EquipmentSystem::ApplyAffix`の実装と1:1対応させて%系Statには「%」を付けて表示するよう`InventorySystem`/`VendorSystem`両方のツールチップを修正(例: 「+12.0% 火耐性」「+15.0 生命力」のように固定値/%が見分けられるようになった)。
- **PoE2のMod仕様を調査し仕様書へ反映**([PoE 2 Guide - Item Modifiers Explained](https://mobalytics.gg/poe-2/guides/item-modifiers)、[PoE2 Prefix vs Suffix & Item Level Guide](https://poe2.stratlore.com/en/guides/item-modifiers-item-level-prefix-suffix/)等で調査): `PoE2_仕様書.xlsx`の「07_アイテムMod仕様」(Prefix/Suffix分離・Tier・Weight・Mod Pool・Local/Global区分・タグ・Mod競合・Rare最大6個等)、「08_クラフト仕様」(各Currency Orbの効果)は既に本セッション前半で詳細に記載済みであることを確認(内容は今回の調査結果と整合)。ただし両カテゴリには他の多くのカテゴリにある「実装状況(2026-09時点)」の現状差分メモが無かったため新規に追加: 本実装は仕様書が定義するTier/Weight/Mod Pool/Local-Global区分/タグ/Mod競合/Implicit Modを持たない簡易システムであること、Magicアイテムが本家(Prefix1+Suffix1の計2個)と異なりどちらか1個のみである簡易化、`ItemRarity::Unique`が列挙値として存在しても`RollRarity()`の対象外で実際には生成されないこと、Currencyは8種類のみ(Exalted/Divine/Vaal Orb・Quality変更・Socket/Rune処理・Crafting Bench的機能は未実装)であることを明記した。

**フォローアップ: 「アイテムに何のモッドが付いているかわかりにくい」との指摘を受けて修正**。原因は`CharacterSheetSystem`(Cキー、装備確認の主要画面)の装備欄だけが「スロット名: アイテム名 (レアリティ, N mods)」という件数のみの1行表示で、ホバーしても詳細が一切出ない作りだったこと(InventorySystem/VendorSystemには既にホバーツールチップがあったが、この画面だけ抜けていた)。他の2画面と同じ内容・体裁の`DrawItemTooltip`(名前/レアリティ/スロット/レベル/ソケット/モッド一覧/売却額)を追加し、装備欄の各行にカーソルを乗せると詳細が見られるようにした(装備欄を全行描き終えたあとにまとめてツールチップを描く設計で、直前に修正したのと同じ描画順序バグを再発させないようにしている)。

**さらにフォローアップ: 「まだわかりにくい、PoE2だったら『マナ+10』『火耐性+10%』って感じ」との指摘を受けて表示順を修正**。それまでは`+10.0% 火耐性`のように**値が先・効果名が後**の順で表示しており、本家PoE2の実際の表記(`Fire Resistance +10%`のように**効果名が先・値が後**)と逆だった。あわせて小数点(`+10.0`)も本家は基本整数表示のため、`std::lround`で丸めるよう変更。`ItemUIHelpers::FormatAffixLine(const ItemAffix&)`を新設して「効果名 +値[%]」の順で整形するロジックを一本化し、`InventorySystem`/`VendorSystem`/`CharacterSheetSystem`の3箇所全てで同じ関数を呼ぶよう統一(個別にostringstreamを組み立てていた重複コードも解消)。

**重大バグ修正: モッドの効果名が丸ごと表示されない(空欄になる)問題**。実機のスクリーンショットで「+8」「+12」「+16%」のように**数値だけで効果名が一切表示されていない**のを発見。原因は`CampaignManager::WriteItem`/`ReadItem`(セーブ/ロード)が`ItemAffix`の`stat`/`value`/`tier`/`isPrefix`はシリアライズしていたが**`label`フィールドだけシリアライズし忘れていた**こと。そのため、一度でもゾーン遷移(`AdvanceToNextZone`)でセーブ&ロードを経由したアイテム(装備中/バッグ内問わずほぼ全アイテム)は`label`が空文字列に戻ってしまい、表示は「 +8」(先頭に見えないスペースのみ)になっていた。直前の「効果名 先→値」修正がまさにこの空の`label`を先頭に出す実装だったため、バグが露見した。**根本対応として、表示側が`ItemAffix::label`(保存されない/信頼できない)を一切参照せず、`ItemUIHelpers::AffixLabel(AffixStat)`という新設の変換関数で`stat`(こちらは正しく保存・復元される)から効果名を都度導出するよう変更**。セーブ形式自体は変更していないため、この修正は新規アイテムだけでなく**既存のセーブデータ内の(既に空labelになってしまった)アイテムも次回読み込み時から正しく表示される**。

**フォローアップ(横断監査+継続作業): 「作業を進めて」の指示を受け、今回発見したバグと同系統の問題が他に残っていないか監査**。
- `.substr(0, N)`によるバイト単位切り詰めがコードベース全体に他に無いか`grep`で確認。残っていた2箇所(`CampaignManager.cpp`/`KeyBindings.cpp`の`kv[line.substr(0, eq)]`)は`=`区切りのconfigパース用でASCIIキー名にしか使われておらず問題なし。
- `CharacterStatsComponent`の全フィールドを`WriteStats`/`ReadStats`と突き合わせ、`label`と同様の保存漏れが無いか確認。未保存だったのは`esRegenDelay`/`hitInvincibilityTimer`/`rollCooldownTimer`/`pendingLeech`の4つだが、いずれも戦闘中の一時的なタイマー/蓄積値でありゾーン遷移(=セーブのタイミング)をまたいで保持すべきでない設計上正しい未保存(`increasedAttackDamage`等の恒久ボーナスとは性質が異なる)と判断し、対応不要と結論。
- `InventorySystem`のドラッグ&ドロップ(`TryMoveBagItem`/`HandleDrop`)をコードレビューし、複数マスアイテムの移動・同サイズ入れ替え・範囲外拒否のロジックに不整合が無いことを確認(実機での対話的検証は、本ROADMAP末尾の「既知の制約」に記載の自動入力に関する制約により未実施)。
- セッション全体を通じて段階的に日本語化してきた流れに合わせ、`CharacterSheetSystem`のステータス欄(Lv/XP/Gold/Str/Dex/Int/Atk/Crit/MoveSpd/Evasion/Armour/Accuracy/Res/Leech等)と見出し("Character Sheet"→「キャラクターシート」、"Equipment"→「装備」、"(empty)"→「(未装備)」)を日本語化。インベントリ/商人画面は既に日本語化済みだったため、これで主要3画面の言語が統一された。

**エンドゲーム(ウェイストーン制マップ)を実装**: 実際のPoE2の仕様([Waystones | PoE2 Wiki](https://pathofexile2.wiki.fextralife.com/Waystones)、[PoE2 Waystone Guide](https://poe2path.com/guides/poe2-waystone-system-guide/)等で調査)に基づき、以下を実装。
- **新規アイテム`WaystoneInventoryComponent`/`WaystonePickupComponent`**(`Components/Item/Waystone.h`): ティア1〜15を`std::array<int,15>`のティア別所持数として管理する非装備の消費アイテム。装備アイテムと違い「保持しておいて隠れ家で自分の意思で使う」性質のため、Currency(拾った瞬間に即適用)とは別の仕組みにした。プレイヤーは`EntitySpawner::CreatePlayer`でTier1を1個所持した状態で開始する(本家は幕を終えたクエスト報酬で入手するが、早期に入手できないと検証すら出来ないため簡略化)。
- **ドロップ**: `CollisionSystem::TrySpawnWaystoneDrop`が、現在の幕が`isEndgame`のときだけ動作。**ボスは100%の確率で「使用したウェイストーンの1ティア上」を確定ドロップ**(本家PoE2の仕様通り)。**Rareモンスターは35%の確率で同ティアのウェイストーンをドロップ**(マップ周回の持続用、本家のドロップ機構を簡略化して再現)。拾得は他のドロップ品同様クリック方式(`ItemPickupSystem`に統合)。
- **マップを開く操作**: `CampaignManager::OpenEndgameMap(tier)`を新設。エンドゲームの拠点(`endgame_hub`)にあるポータルは、他の幕のような無条件遷移ではなく、`GameScene::TryOpenEndgameMapFromHub()`が所持ウェイストーンのうち**最もティアが高いもの**を1個消費してそのティアのマップを開く(本家の「常に最高ティアを消化する」定石に合わせた自動選択。個別ティア選択UIは今回は実装していない)。ウェイストーンを1つも持っていない場合は入場を拒否しメッセージ表示。所持数は`GameScene`のHUD(ポータル付近で「T3x2 T1x1」のように表示)と`CampaignManager`のセーブ/ロードで永続化。旧来の「クリアするたびにティアが自動+1され続ける」仕様(セーブされたウェイストーンを消費しない無限ループ)は廃止。
- **ティアによるスケーリング**: 既存の`tierScale`(HP/攻撃力倍率、`ZoneBuilder`)に加えて、`ZoneBuilder::ApplyTierResistance`で全耐性(火/冷/電/カオス)に`+3%×(tier-1)`(上限90%)を追加。「それぞれのレベルでは相手の耐性やレベルなどが違う」という要望に対応。
- **マップ完了条件**: 既存の「ゾーン内の敵が全滅したら次へ進める」判定(`GameScene::Update`)がそのままボス+トラッシュ全滅を要求するため追加実装は不要だったが、「レアエネミー複数」を要求する趣旨に合わせ、エンドゲームのマップでは先頭3体のトラッシュを確率roll無視で強制的にRareレアリティにする(`ZoneBuilder::SpawnTrash`の`forceRare`)よう変更(通常の4%ロールだけでは複数のRareが毎回出るとは限らないため)。
- 実装未検証事項: Act1〜4を実際にクリアしてエンドゲームへ到達し、マップを周回してウェイストーンのドロップ・消費・ティア上昇のループを実機で通しプレイする検証は今回行っていない(ビルド成功のみ確認)。またVendorでのウェイストーン売買は本家に存在するが未実装。

**さらに調整: タブ切り替えにした以上、キャラクターシートとスキルジェムは同じ画面枠を共有するはずなのに、位置・サイズがそれぞれ独自の値(上下積み重ね時代の名残)のままだったため揃っていなかった。両システムの`kPanelX/kPanelY/kPanelW/kPanelH`を完全に同一の値(`(20,20)`、460x680、右のインベントリとは20px開けて左側いっぱいに使う)に統一。またキャラクターシートのステータス欄は、装備欄の開始Y座標を「1行あたり16pxと仮定した概算」で計算していたため、実際のフォントの行送りとズレて装備欄がステータス末尾の文字に重なる不具合があった。`sf::Text::getGlobalBounds()`で実際に描画したステータステキストの高さを測定し、その実測値を基準に装備欄を配置するよう修正して解消。**インベントリが画面右側にドッキングされたことで、中央に配置されたままだったキャラクターシート/スキルジェムパネルと同時に開くと重なってしまう問題があったため、この2つは画面左側へ表示位置を変更した。ただし実際のウィンドウ解像度は1280x720(`Config.h`)で、インベントリ(760幅)が右側の過半を占めるため、当初の`panelX=40`固定(パネル幅620/640のまま)では左に寄せても互いに重なり、かつステータス欄の項目数がボックス高さに対して多すぎて枠外にはみ出す問題が残っていた。そのため改めて、キャラクターシート(460x420、左上`(20,20)`)とスキルジェム(460x250、左下`(20,460)`)を**上下に積み重ねて**幅を縮小し、インベントリ(`panelX=1280-760-20=500`)との間に20pxの余白を確保。あわせてステータス/装備/スキル各行のフォントサイズと行間を詰め、さらに**表示幅に収まらない文字列(アイテム名やスキル名)は`sf::Text`の実測幅を見て末尾を"..."で省略するTruncateヘルパー**を両システムに追加し、項目数や名前の長さに関わらずボックス外にテキストがはみ出さないようにした。
- 通貨/クラフト: Transmutation/Regal/Chaos/Alchemy/Augmentation/Annulment/Chance/Scouring を装備済みアイテムに自動適用 (`CurrencySystem`)。各通貨ごとの対象条件 (`FindEligibleSlot`) を満たす最初の装備スロットへ即座に効果を適用する設計 (所持・選択UIは無し)。`CurrencyType`に`Count`番兵を追加し、ドロップ抽選 (`CollisionSystem`) がハードコードした範囲ではなく`CurrencyType::Count`から動的に算出するよう修正 (新規追加時の抜け漏れ防止)。Artificer's Orb (Socket追加) はSocket/Runeシステム自体が未実装のため、Vaal Orb/Mirror of KalandraはリスクリワードUI設計が必要なため今回は見送り。ビルド未検証(環境制約は下記)だが、`CurrencySystem.h`単体はcl.exe構文チェックでエラー無しを確認済み。
- XP/レベリング: 指数カーブ。レベルアップでランダム自動付与だったパッシブ廃止 (`PassiveSystem`削除)、代わりにレベルアップ毎に1ポイント獲得しパッシブツリーUI (`PassiveTreeSystem`, Pキーで開閉) で手動選択。中央スタートノードから4方向(攻撃/防御/速度・マナ/耐性)へ5ノードずつ伸びるグラフ構造 (`PassiveTreeData`)。矢印キーでノード間移動、Enterで隣接済みノードのみ割り振り可能。`baseStats`/`live`分離によりレベル・XP・パッシブが装備変更で消えないよう修正済み（ゾーン遷移時に`equipment.baseStats`が未復元だったバグも合わせて修正）。
- NPC/Vendor(商人)システム: タウン中央に商人NPCを配置 (`ZoneBuilder::SpawnVendor`)。近づいてBキーで開く`VendorSystem`から、プレイヤーレベルに応じて生成された在庫6点(訪問毎に再生成、購入で減る)をゴールドで購入しインベントリへ追加できる。買値はSellValueの3倍。売却は引き続きインベントリUI側で行う。在庫はTキーでゴールドを払ってリロール可能 (`VendorSystem::TryReroll`、価格は`RerollPrice(playerLevel) = 20 + level*4`)。在庫が売り切れていてもリロールは可能。
- ゴールド経済: 拾った弱い装備を自動売却。
- **アイテム拾得を接近自動から左クリック方式へ変更** (`ItemPickupSystem`): カーソルがアイテム(装備/通貨/スキルジェム)の`kClickRadius`(28px)以内にある間、名前をカーソル脇に色付き(レアリティ/種別に応じた色)で表示。その状態で左クリックすると拾得し、`InputSystem`側は同じフレームでSkill1の左クリック割当を抑制する(カーソルが乗っていなければ従来通りクリックでSkill1を発動)。ホバー判定・拾得処理は`GameScene::Update`で1回だけ計算して両システムへ受け渡す設計。
- **UI文言のUTF-8→sf::Text変換バグを修正**: `sf::Text`のコンストラクタが受け取る`std::string`はANSI/ロケール変換される(`sf::String::fromUtf8`を通さないと文字化けする)仕様だったが、今回追加した`InventorySystem`のDrawTextがこれを見落としていた。`GameScene.cpp`のゾーン名表示(既存)は元々正しく`fromUtf8`を使っていたのを参考に、`InventorySystem`/`PassiveTreeSystem`/`SkillGemSystem`/`CharacterSheetSystem`/`VendorSystem`/`KeyBindSystem`の全DrawTextヘルパーと`GameScene.cpp`の残りのHUDテキスト(hudLine/pickupメッセージ/レベルアップメッセージ)を`fromUtf8`経由に統一。VS2022実機ビルド+起動確認済み。
- キャラクターシートUI (`CharacterSheetSystem`, Cキーで開閉)。
- スキルジェムシステム: PoE2のように5スキルスロットを自由に(コスト無し・いつでも)付け替え可能にした。`SkillGemData`に全スキルのプリセットを定義(既存4種 + 新規War Cry/Nova)、モンスターがドロップする`SkillGemPickupComponent`を拾うと`SkillGemInventoryComponent::unlockedGemIds`に恒久追加(重複取得は無視)。スキルジェムUI (`SkillGemSystem`, Kキーで開閉) で各スロット(E/Q/R/V/F)にUp/Downで移動しLeft/Rightで割り当てジェムをサイクル切替(Emptyも選択可)。開始時は既存4スキル(Spark/Thunder Slam/Lightning Warp/Lightning Ball)を所持、5枠目は空。セーブ/ロード対応(`unlockedGemIds`と5スロットの割り当てgemId)。旧セーブとの互換のため、未保存(空)ならEntitySpawnerのデフォルト構成を維持。
- **Spirit/Auraジェムシステム(新規)**: `SkillBehaviorType::Aura`を追加し、5つの発動スキルスロットとは別に`SpiritGemLoadoutComponent`で2つのSpiritスロットを新設。Auraジェム(Determination: +60 Armour/Spirit50、Discipline: +40 ES/Spirit40)は発動もクールダウンも持たず、装着している間だけ`SkillData::auraEffect`(`ItemAffix`を再利用)を`equipment.baseStats`へ直接適用し続ける(パッシブツリーと全く同じ`EquipmentSystem::ApplyAffix`/`RemoveAffix`機構を再利用、Increased系affixの多重掛け算バグ修正の恩恵もそのまま受ける)。`SpiritAuraSystem::TryAssign`が装備済みAuraの合計消費が`CharacterStatsComponent::maxSpirit`を超える差し替えを拒否する(`currentSpirit`は「消費中の量」として扱う)。UIは既存の`SkillGemSystem`(Kキー)にSpirit行2つを追加する形で統合、Left/Rightでのサイクル対象は選択中スロットの種別(発動スキル/Aura)で自動的にフィルタされる。プレイヤーの`maxSpirit`初期値は100。セーブ/ロード対応(`auraLoadout.slot0/1`、効果自体は`equipment.baseStats`に既に焼き込まれているため表示用の復元のみ)。ビルド未検証、実機での確認が必要。
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
