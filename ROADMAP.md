# ROADMAP

2D ARPG (PoE2-style) — ウェイストーンでマップを開いてクリアするPoE2エンドゲームループの再現を目指す垂直スライス開発のロードマップ(2026-09-17: Act1-4/幕間キャンペーンを廃止し、エンドゲームループのみの構成へ方針転換)。

## 済 (Done)

**エンドゲームマップを拡張(70x70→90x90)+タイルにプロシージャル生成テクスチャを追加**。「マップをもう少し広くして欲しいのとテクスチャを追加してほしい」という指示に対応。
- `CampaignManager::BuildActs`の`endgame_map`ゾーンを70x70→90x90タイルへ拡張。面積比(8100/4900≈1.65倍)に合わせて雑魚敵数も18→30へ増やし、広くなった分だけ敵が間延びして見えないようにした。
- `MapRenderSystem`は従来`TileType`ごとの単色矩形塗りつぶしだった描画を、新設`TileTextureFactory`が起動時に1回だけ生成する256x256のノイズテクスチャ(床=斑点柄/壁=レンガ目地柄/木材=板目柄、`sf::Image::setPixel`で手続き的に描画)を使ったテクスチャ貼り付けに変更した。手描きのタイル素材(スプライトシート等)は用意していない(`Assets/Textures/`には背景/タイトル用の既存画像のみ)ため、コード内で完結する代替手段として採用。1マスごとにタイル座標から決定論的に算出した256x256アトラス内の異なるオフセットを切り出すことで、同種タイルが並んでも同じ絵がスタンプされているようには見えないようにしている。
- ビルド確認済み(`/t:Build`、0エラー)。バックグラウンド起動+スクリーンショットで実際にレンガ壁/斑点床のテクスチャが表示されることを目視確認済み(本セッションでは珍しくスクリーンショットによる視覚確認まで実施)。

**Atlasノードのティア制約を撤廃、マップのティアはドロップしたWaystone自身で決まるように修正**。「まっぷでウェイストーンのレベル決めないで。ウェイストーンのレベルで決まるようにして」というフィードバックに対応。
- 前回実装した「ノードの座標(開始点からの距離)がそのままマップのティアを決め、一致するティアのWaystoneしかソケットへ置けない」仕組み(`AtlasData::TierForCoord`)を撤廃。ノードは進行度(`AtlasData::DistanceFromStart`)を持つのみでティアという概念自体を廃止した。
- `AtlasSystem::TryPlaceWaystone`はカテゴリがWaystoneでありさえすればティア不問でソケットへの配置を受け付け、そのWaystone自身の`waystoneTier`で`CampaignManager::OpenEndgameMap`を呼ぶ。手持ちの好きなティアのWaystoneをどのノードにも使える(高ティアWaystoneを低距離のノードで使う、といった組み合わせも可能)。
- ノードの解放(`AtlasData::MarkCompleted`)はどのティアでクリアしたかに関わらず「そのノードを一度でもクリアしたか」のみで判定される点は変わらない。
- UI表示もティア前提の文言(「Tier N Map」「owned: N」等)を全て「Locked/Unlocked/Cleared」ベースの状態表示へ置き換えた。
- ビルド確認済み(`/t:Build`、0エラー)。バックグラウンド起動でのクラッシュ無し確認も実施。

**マップデバイスを左クリック操作へ変更+Atlasの起動をドラッグ&ドロップ化+アイテムグリッドの自由配置修正**。「エンターで開くのではなく左クリックで開く。また、マップを選択したら1マスの四角が出てきて自分のアイテム欄からドラッグアンドドロップでウェイストーンを入れる。また、アイテム欄今基本的に左上からしか入れられないけど自由に配置できるようにしてほしい」という3点のフィードバックに対応。
- **マップデバイスの操作方式**: Enterキー起動を廃止し、既存のNPC(Vendor/Waystone Vendor/Stash)と同じ「近づくとクリック範囲の輪が表示→左クリックで発動」方式に統一(`GameScene`に`m_hoveringPortal`/`m_clickedOnPortal`を新設、`m_hoveringNpc`/`m_clickedOnNpc`と同じ1フレーム遅延パターン)。HUDの案内文も「Press Enter to...」から「Click to...」へ変更。
- **Atlasのマップ起動をドラッグ&ドロップ化**: 前回実装した「ノード選択→Open Mapボタン」を撤回し、`AtlasSystem`にプレイヤーのバッグ(6x4グリッド)を画面右にドッキング表示、ノード選択後に現れる1マスの「ソケット」へバッグからWaystoneをドラッグ&ドロップした瞬間に判定する方式へ変更。ティア一致で即座に消費してマップを開始、不一致やWaystone以外は理由を表示して拒否(確認ボタン無し、ドロップ=確定)。
- **アイテムグリッドの自由配置**: `StashSystem`のバッグ⇔Stash間ドラッグが常に転送先の「最初の空きマス」へ強制配置しドロップ位置を無視していたバグを修正。`ItemUIHelpers::TryMoveWithinGrid`/`TryMoveBetweenGrids`を新設し`InventorySystem`(重複コード削除)/`StashSystem`(バッグ⇔Stash・Stash内・バッグ内の全パターン)/`AtlasSystem`(バッグ内再配置)で共有、常に実際のドロップ位置(または同サイズアイテムとの入れ替え)へ配置されるようにした。
- ビルド確認済み(`/t:Build`、0エラー)。バックグラウンド起動でのクラッシュ無し確認も実施。ただし実際のマウス操作によるプレイテストは本セッションの制約上未実施。

**PoE2風の全画面Atlas(無限に広がるマップ選択画面)を実装**。直前のバッチで「今回スコープ外」としていたAtlas風マップ選択UIについて、続けて「実装して」との指示を受け実装した。
- `AtlasComponent`(`External/ECS/Components/Progression/Atlas.h`、クリア済みノード座標のみ保持)+`AtlasData`(`External/ECS/Systems/Progression/AtlasData.h`、座標→ティア/隣接/画面座標を都度算出する純粋関数群、ノード自体は無限グリッドのため保存しない)+`AtlasSystem`(`External/ECS/Systems/Progression/AtlasSystem.h`、`PassiveTreeSystem`と同じ全画面パン/ズームUIパターンを踏襲)の3点構成。
- 拠点のポータルEnterキー(`GameScene::TryOpenEndgameMapFromHub`)の挙動を「バッグ内最高ティアWaystoneを即消費してマップへ」から「マップアタempt中でなければAtlas画面を開く」へ変更。Atlas画面でノードを選択し「Open Map」を押すと、そのノードのティア(開始点からのチェビシェフ距離+1、`AtlasData::TierForCoord`)に一致するWaystoneをバッグから1個消費して`CampaignManager::OpenEndgameMap`+新設`SetPendingAtlasNode`を呼ぶ。実際のゾーン遷移は既存の`AdvanceToNextZone`にそのまま乗せる(`AtlasSystem::ConsumeMapStartRequest()`を`GameScene::Update`が毎フレーム確認、trueなら`AdvanceToNextZone()`)。
- ノードは開始ノード(0,0、常時クリア済み扱い)から隣接クリア済みノードを辿って解放される(`AtlasData::IsUnlocked`)。表示範囲は既クリア最遠距離+2まで(`AtlasSystem::VisibleRadius`)で、進むほど画面上のノード数が増えていく「無限に広がる」挙動を実現。一度解放したノードは実際のPoE2同様クリア後も再度選択可能(周回ファーミング用途、再ロックしない)。
- マップクリア時(`GameScene::Update`のゾーンクリア判定)に`CampaignManager::GetPendingAtlasNode`で対象ノードを特定し`AtlasData::MarkCompleted`でクリア済みにする。6回死亡してポータルを使い切り失敗した場合は`CampaignManager::ConsumeMapDeath`がペンディングノードを破棄するのみでノードはクリア済みにならない(再挑戦は可能)。
- セーブ/ロード: `CampaignManager`に`m_savedAtlasNodes`(`atlas.count`/`atlas.node{i}.col/.row`、平文の座標ペアとして保存しAtlasData独自のパック形式には依存しない設計、`m_savedPassiveTree`と同じ思想)を追加。
- ビルド確認済み(`/t:Build`、0エラー)。バックグラウンド起動でのクラッシュ無し確認も実施。ただし実際のマウス操作によるプレイテスト(ノード選択→Waystone消費→マップクリア→隣接ノード解放→再訪、という一連の流れ)は本セッションの制約上未実施。

**Waystoneをインベントリの通常アイテム化+MODシステム+ポータル/デス制へ改修(レベルカーブも修正)**。「修正点。まず行けるレベルは2lvは5から。3lvは15lvからにしよう。あと、ウェイストーンは普通のアイテムと同じようにアイテム欄で所持する。なのでアイテムがいっぱいの時は買えない。また...ウェイストーンにはPOE2と同じくMODをつける。例えば元素耐性が下がったり。あと、マップデバイスで開くと最大6個のポータルが出る。要するに最大6回死ぬことが可能。それ以上死ぬとマップは閉じてしまう」という指示に対応(A/B/D/Eを実装、C=Atlasツリー風の無限マップ選択画面は今回のスコープ外、下記参照)。
- **①レベルカーブの修正**: 前回実装した`MaxTierForLevel(level) = clamp(level, 1, 15)`(1:1対応)を撤回し、`WaystoneVendorSystem::RequiredLevelForTier`をテーブル方式(`{1,5,15,22,29,36,43,50,57,64,71,78,85,92,99}`)へ変更。指定された2点(Tier2=Lv5, Tier3=Lv15)を固定アンカーとし、Tier4以降はTier15がLv99に収まるよう+7/レベルの線形カーブで補間した(この後半カーブはユーザー未確認の暫定値、要フィードバック)。
- **②Waystoneをインベントリアイテム化**: `WaystoneInventoryComponent`(ティア別カウント配列)/`WaystonePickupComponent`を全廃し、`ItemComponent`に`category`(`Gear`/`Waystone`)判別フィールドを追加(`slot`/`affixes`はGear専用、`waystoneTier`/`waystoneMods`はWaystone専用として同じ構造体を共用、既存の全アイテムグリッド/売買/拾得/保存パイプラインにそのまま乗せるため)。`ItemUIHelpers::ItemGridSize`にアイテム受け取りオーバーロードを追加し、`VendorSystem`/`InventorySystem`/`StashSystem`/`ItemPickupSystem`の全呼び出し箇所を追従。Waystoneは1x1マスとしてバッグ/Stash/Vendorに現れ、`WaystoneVendorSystem::TryBuy`はバッグに空きが無いと「Inventory full」で購入を拒否する(拾う時と同じ`FindBagFreeSpace`判定を共有)。装備防止(`QuickEquip`)・比較ツールチップ・ソケット数計算(`SocketCount`)にも`category==Gear`ガードを追加(Waystoneが武器スロットへ装備可能に見えてしまう欠陥を実装中に発見し修正)。
- **③WaystoneMod(PoE2風マップMOD)の新設**: `WaystoneModStat`(モンスター強化系: 増加体力/増加ダメージ/増加元素耐性、プレイヤー弱体系: 元素耐性低下、報酬系: アイテムレアリティ/数量増加)+`WaystoneMod{stat, value, label}`を新設(プレイヤーへの直接バフである`ItemAffix`とは意図的に別型、マップの敵/報酬を変質させるものであり装備効果ではないため)。`ItemFactory::GenerateWaystone`がレアリティに応じ0-3個のMODをロール。`ZoneBuilder::Build`が`mapMods`引数を受け取りモンスター系MODを`ApplyMapMods`で反映、プレイヤー系MOD(耐性低下)は`GameScene`側でCombatゾーン進入時に適用、報酬系MODは`CollisionSystem::TrySpawnItemDrop`のドロップ率/レアリティ抽選に反映(`ItemFactory::RollRarity`へ`bonusPercent`引数を追加)。
- **④マップデバイスのポータル/デス制(最大6回死亡まで許容)**: `CampaignManager`に「マップアタempt」状態(`m_mapAttemptActive`/`m_mapDeathsRemaining`/`m_activeMapMods`、ゾーン/エンティティ状態と同様に非永続)を新設。`OpenEndgameMap`はWaystone消費時にdeathsRemaining=6で開始、`GameScene`の死亡処理で`ConsumeMapDeath()`を呼び残機を減算、0を切るとマップ強制クローズ(ハブへ送還)。既にアタempt中にマップデバイスへ再入場する場合はWaystoneを消費せず無料で再入場する(`HasActiveMapAttempt()`で分岐)。実装中に「`OpenEndgameMap`が`m_zoneIndex=1`を直接代入した直後、同じ呼び出し内の`CompleteCurrentZoneAndAdvance`のトグル計算で即座に0へ戻ってしまい、マップへ実際には移動できない」という潜在バグを発見・修正(直接代入を削除しトグルのみに依存)。
- 旧`External/ECS/Components/Item/Waystone.h`は全コード箇所の移行完了を確認の上で削除、`Game-SFML.vcxproj`からも除去。
- ビルド確認済み(`/t:Build`、0エラー)。バックグラウンド起動でのクラッシュ無し確認も実施。ただし実際のマウス操作によるプレイテスト(購入→満杯拒否→マップ挑戦→MOD反映→6回死亡でクローズ、という一連の流れ)は本セッションの制約上未実施。
- **(このバッチでは)スコープ外としていたが、直後の指示で追加実装済み**: 指示にあった「マップデバイスで開くと...パッシブツリーのような感じで無限に広がるマップをひたすら進めていく」というAtlas風の無限マップ選択UIは、当初は独立した規模の新機能になると判断し見送ったが、ユーザーから続けて「実装して」との指示を受け、このすぐ上の「PoE2風の全画面Atlas」エントリで実装済み。

**Waystone Tier1を無料化し、購入可能な上限をキャラクターレベルに連動**。「ウェイストーンlv1は無料にしよう あと自分のレベルで行けるウェイストーンの上限解放にしたい」という指示に対応。
- `WaystoneVendorSystem::PriceForTier`をTier1のみ0ゴールドに変更(最初のマップに挑戦する手段が無い詰みを防ぐ)。
- `MaxTierForLevel(level) = clamp(level, 1, 15)`を新設し、Tier NはキャラクターLv Nで解放されるようにした(本作にAtlas進行のような別の解放システムが無いため、レベルと1:1対応させる最も単純な設計)。未解放のTierも一覧からは消さず赤字+「[Requires LvN]」表示にし(既存のSkillGemSystem等と同じ「理由を必ず表示する」方針)、クリックしても購入できない(`TryBuy`が同じ判定を再チェック、表示とロジックが乖離しないよう共有関数`IsUnlocked`を使用)。
- ビルド確認済み(`/t:Build`、0エラー)。起動確認(クラッシュなし)も実施。

**町のサイズ縮小+Stashを4タブ構成へ拡張**。「まず町がひろすぎるのと、スタッシュタブは4ページ用意しよう」というフィードバックに対応。
- 町(エンドゲームハブ)のマップサイズを`CampaignManager`の`MakeTown`で30x30→16x12タイルへ縮小(1920x1920px→1024x768px)。`ZoneBuilder::PlaceTownNpcs`のNPC同士/スポーン地点・ポータルとの回避距離もタイルサイズ×3→×2、×4→×2.5へ詰めて、小さくなった町でも自然に配置されるようにした。
- `StashComponent`を単一の10x8グリッド(`items`)から、独立した10x8グリッド4ページ(`tabs`、`kTabCount=4`)へ変更。`StashSystem`の左パネル上部にタブ切替ボタン(1-4)を追加し、アクティブなタブのグリッドとバッグの間でドラッグ移動する(タブ間の直接移動は今回は無し、いったんバッグを経由する)。
- `CampaignManager`の永続化形式も`stash.count`/`stash.item{i}.*`から`stash.tab{t}.count`/`stash.tab{t}.item{i}.*`(t=0-3)へ変更。
- ビルド確認済み(`/t:Build`、0エラー)。起動確認(クラッシュなし)も実施。

**町(エンドゲームハブ)にNPCを複数配置し、Waystone専売NPCとStash(アイテム保管庫)を新設**。「町を豪華にしていこう。NPCを複数設置。現在のアイテムを売るタイプと、ウェイストーンだけを売るタイプ、あとスタッシュタブを用意しよう」という指示を受けて実装。
- `ZoneBuilder::SpawnVendor`(単一位置)を`PlaceTownNpcs`へ汎用化し、`MapGenerator::GetWalkablePositions`から互いに距離を取った3箇所(アイテムVendor/Waystone Vendor/Stash)を選んで配置するように変更(`TownNpcKind`+座標、新規`Programs/Game/GameScene/Zone/TownNpc.h`)。`GameScene`側も単数の`m_hasVendor/m_vendorPos/m_playerNearVendor`等を`m_townNpcs`(vector)+`m_nearNpcIndex`へ汎用化し、近接リング表示・クリック判定・該当UIの開閉(既存の`vendorSystem`に加え`waystoneVendorSystem`/`stashSystem`)を3種共通ロジックで処理するようにした。
- 新規`WaystoneVendorSystem`: Waystoneはアイテムではなくティア別カウント(`WaystoneInventoryComponent`)なので、既存の装備アイテム前提の`VendorSystem`とは別実装にした。全15ティアを一覧表示し、ゴールド(`20+tier*20`)のみで購入(Buyback無し、アンロック制ではなく価格でティアをゲート)。
- 新規`StashSystem`+`StashComponent`(`External/ECS/Components/Item/Stash.h`、10x8=80マス): バッグ(`InventoryComponent`)とは別の常設ストレージ。左にStashグリッド・右にバッグを並べ、双方向ドラッグでアイテムを移動する(VendorSystemの「バッグ→左パネルへドラッグ」の見た目を踏襲)。`ItemUIHelpers::BagRegionFree/FindBagFreeSpace/NormalizeBagPlacement`はバッグの6x4グリッドに固定実装だったため、cols/rows引数(デフォルトはバッグのまま)を追加してStashの10x8グリッドでも再利用できるよう汎用化した。
- Stashの中身は`CampaignManager`に`stash.count`/`stash.item{i}.*`として永続化(マップ移動・死亡をまたいで保持、既存の`inventory.*`と全く同じ書式)。
- ビルド確認済み(`/t:Build`、0エラー)。起動確認(クラッシュなし)も実施。ただし実際のマウス操作によるプレイテスト(町でNPC3体に話しかけて売買/保管が正しく動くか)は本セッションの制約上未実施。

**Act1-4/幕間のストーリーキャンペーンを廃止し、ウェイストーン制エンドゲームループのみのゲームへ作り直した**。ユーザーから「今回のゲームはウェイストーンを入れてマップを開く→クリアを目指す、PoE2のエンドゲームだけを再現しよう」という方針転換の指示を受け、`AskUserQuestion`でスコープを確認したところ「Actキャンペーンを廃止し、エンドゲームループだけのゲームに作り直す」ことが確定した。調査の結果、エンドゲームループの仕組み自体(拠点→ウェイストーン消費→マップ生成→クリア→拠点、`CampaignManager::OpenEndgameMap`/`CompleteCurrentZoneAndAdvance`、`ZoneBuilder`のTierスケーリング)は既存実装で完成していたため、実質的な変更は「非エンドゲームの6幕(Act1/Act2/幕間I/Act3/Act4/幕間II)を`CampaignManager::BuildActs`から削除し、エンドゲームAct1つだけを残す」という削減作業のみで済んだ。
- `CampaignManager::BuildActs`から6幕分の`ActDefinition`ブロック(合計20超のゾーン定義)を削除し、エンドゲーム(`地図の狭間`)のみを構築するように変更。プレイヤーは`EntitySpawner`が最初から持たせるTier1ウェイストーンで即座にマップへ挑戦できる。
- 幕(Act)クリアに紐づいていた耐性ペナルティ機能(`m_pendingResPenaltyNotice`/`m_actsClearedForResPenalty`/`ConsumePendingResPenaltyNotice`、幕クリア毎に全耐性-10%)は、Act進行自体が無くなり二度と発動しない死んだコードになるため完全に削除した。`CampaignManager::CompleteCurrentZoneAndAdvance`もハブ↔マップの単純なトグルのみに簡素化。
- `GameScene.cpp`の「タウンのポータルでEnter」処理と「Press Enterヒント」表示にあった`CurrentAct().isEndgame`の分岐(非エンドゲーム時の`AdvanceToNextZone`直接呼び出し/「Press Enter to proceed」表示)は、常にエンドゲームになったため到達不能コードとして削除し、常時`TryOpenEndgameMapFromHub()`/Waystone案内表示のみに一本化。
- `AI/DECISIONS.md`の「進行構造(Act/エンドゲーム)」を書き換え、耐性ペナルティの決定事項を削除。`AI/STRUCTURE.md`のキャンペーン/エンドゲーム説明も更新。
- ビルド確認済み(`/t:Build`、0エラー)。マップの種類は現状「歪んだ地図」1種類のみで、本家のような多数のMapタイプ(Crypt/Foundry等)の再現は今回のスコープ外(将来の拡張候補として`AI/DECISIONS.md`に明記)。

**Permanent Minion生存中にゾーンクリア判定が成立しないバグを修正**。`PoE2_仕様書.xlsx`の実装調査中に発見。`GameScene::Update`のゾーンクリア判定(`View<CharacterStatsComponent>()`のうち`PlayerTag`を持たないエンティティが1体でもいれば「敵が残っている」とみなす実装)が、`AllyTagComponent`(Permanent Minion)を除外していなかったため、自分の召喚ミニオンが生存している間は敵を全滅させてもゾーンクリアと判定されなかった。判定条件へ`!registry->HasComponent<AllyTagComponent>(entity)`を追加して修正。

**POE2型ジェムシステム完全実装指示への対応: 依存関係調査+4件の実バグ修正+Permanent Minion新規実装+デバッグツール**。ユーザーから「周辺システムが存在している前提を置かず、まずプロジェクト全体を調査してから足りない基盤も含めて実装せよ」という大規模指示(Skill/Support/Spirit Gem・Uncut Gem・Spirit予約・Support互換性・Skill Bar等、82項目)を受けた。実コードを調査した結果、ジェムシステム本体(Skill/Support/Spirit Gemの分離、Uncut Gem→インベントリ→Gemcutting→装着、Spirit予約/解放、Save/Load)は既にPoE2準拠で接続・動作していることを確認(前回までの複数回のセッションで実装済み)。全82項目を無条件に実装するとご指示自体の§77(重複システムの併設禁止)に反する規模になるため、調査結果を`System/Status/Used By/Missing Feature/Action`形式の依存関係表としてユーザーに提示し、実在する4件のバグ+新規機能2件に絞って承認を得た上で実装した(Trigger/Meta Skillの拡張余地確保は対象外)。
- **①武器要件の発動時再チェック**: 従来`SkillGemSystem::AssignGem`でスロット割当時にのみ武器装備チェックがあり、装着後に武器を外しても発動を止める仕組みが無かった(=UIだけ制限して実処理は素通りする、ご指示§16で明示的に禁止されているパターン)。中央関数`SkillActivation::CanUseSkill`(新規`External/ECS/Systems/Skill/SkillActivationSystem.h`)を新設し、生存/有効/クールダウン/マナ/武器要件を一箇所で判定、`SkillSystem::Update`の実発動ゲートをこれに差し替えた(UIと実戦の判定式が乖離しないよう一元化、ご指示§63)。
- **②MaxSpiritを装備から加算可能な汎用ステータスへ拡張+安全な再評価**: 従来`maxSpirit`は`EntitySpawner`が100固定で以後変化しない値だったため、「MaxSpirit低下時にReservationを安全に再評価する」ご指示§7の対象ケースが実質存在しなかった。`AffixStat::FlatSpirit`を新設し`EquipmentSystem::ApplyAffix/RemoveAffix`・`ItemFactory::SuffixPool`(装備サフィックスとしてロール可能)へ接続、既存の汎用Modifierパイプラインに乗せた。その上で`SpiritAuraSystem::ReevaluateReservations`を新設(予約合計を再計算し、Maxを超えていれば末尾スロットから自動OFF、Effect/Minion/Spirit予約が矛盾なく解放される)、`GameScene::Update`から毎フレーム呼び出す設計にした(装備変更/レベルアップ/パッシブ振り直し等、MaxSpiritを変えうる全箇所を個別にフックせず単一の安全弁として機能させる)。
- **③Support Category重複禁止**: 従来は同一スキルへ「Added Damage」と「Elemental Focus」のように同系統の乗算サポートを無制限に重ね掛けできた。`SupportGemDefinition::category`(`SupportCategory`: DamageMult/Speed/Utility/AreaMod/Duration)を新設し、`SkillGemSystem::AssignSupport`で同一スキル内の同カテゴリ重複装着を拒否(キャラクター全体の制限ではなく、あくまで同一スキル内の制限、ご指示§34)。ピッカーUIにも赤字で"category conflict"を表示。
- **④中央CanUseSkillとUI/Combatの一致**: ①の`SkillActivation::CanUseSkill`がUI/Combat共有の単一ソースとなるよう設計(将来HUD等を追加した際も同じ結果を参照できる)。
- **⑤Permanent Minion(Spirit Gemの新カテゴリ、新規実装)**: 調査の結果、プレイヤー側の永続召喚システムが全く存在しない(敵側の`Summoner`/`EnemySummonSystem`のみ)ことが判明したため新規実装。`SkillBehaviorType::Minion`を新設しAura同様Spiritスロットへ登録→ON/OFFで動作させる(`SkillGemData::IsSpiritBehavior`でAura/Minion共通のSpiritカテゴリ判定に統一)。ON時に`EntitySpawner::CreateMinion`が召喚体を生成、新規`External/ECS/Systems/Chara/MinionSystem.h`が追従/近接AI・死亡検知・リスポーンタイマー(`SpiritGemLoadoutComponent.minionEntity/minionRespawnTimer`)を管理する。ミニオン死亡時はSkillをActiveのまま維持しSpiritも解放しない(リスポーンタイマー経過後に再召喚、OFFにした時だけEntity破棄+Spirit解放、ご指示§12で明示的に要求されているセマンティクス)。本プロジェクトの`CollisionSystem`は「PlayerInputComponentを持つか否か」だけで敵味方を区別する設計(陣営システムが無い)だったため、新設`AllyTagComponent`をプレイヤー自身の弾のダメージ判定・敵の接触ダメージ判定の両方から除外する最小限の変更のみ追加(ミニオン対敵の戦闘自体は`MinionSystem`が自己完結で処理し、`CollisionSystem`本体の的探索ロジックはいじらない設計とした)。サンプルジェムとして`Summon Skeleton`(id16)を追加。
- **⑥ジェムデバッグツール(新規実装)**: 調査の結果、ジェム/ステータス用のデバッグツールが存在しない(`DebugManager`はシーン/カメラ/ログ/パフォーマンスのみ)ことが判明したため新規実装。`GameScene::RenderGemDebugTools`(F1デバッグメニュー内、Registryアクセスが必要なため`GameScene::RenderImGui`側に実装)に、Uncut Skill/Support/Spirit Gemを指定レベルでスポーン、STR/DEX/INT・MaxSpirit(base)・現在マナの直接編集、Jeweller's Orb付与、Spirit予約内訳・最終スキル計算値・サポート適合判定(スロット1基準)の表示ボタン/パネルを追加。実戦/UIと同じ計算関数(`SpiritAuraSystem`/`SkillTags`等)を参照するため表示と実際の挙動が乖離しない。
- 各段階でビルド確認しながら進め、最終的に`/t:Build`で0エラーを確認。バックグラウンド起動での起動確認(`game.log`/`crash.dmp`両方で異常なし)も実施。ただし実際のマウス/キーボード操作によるプレイテスト(ドロップ→拾得→Gem Cutting→装着→Spirit ON→ミニオン戦闘→死亡→リスポーンの一連の流れ)は本セッションの制約上未実施。
- **今回スコープ外(ご指示に含まれるが未対応)**: Trigger/Meta Persistent Skillの拡張余地確保、Weapon Set(既存の単一武器スロット簡略化を維持)、Support GemのTier表示、Lineage Support Gem、構造化ReasonCode/ReasonTextを持つ`CanSupport()`戻り値(現状はメッセージ文字列のみ)、UI側でのCanUseSkill活用箇所の追加(現状SkillBarに相当するHUDが無いため呼び出し元が`SkillSystem`のみ)。

**ジェム取得・ドロップ仕様をPoE2準拠へ修正(Uncut Gemインベントリ経路・サポートジェムもドロップ制へ)**。前回の3点修正(スピリットON/OFF分離等)の直後に、ユーザーから続けて詳細な仕様書(PoE2ジェムシステム仕様書 44-61章「ジェム取得・ドロップ仕様」)を提示してもらい、実装との差分をすり合わせて修正した。中心となる差分は「拾った瞬間に鑑定UIが開く」実装が、仕様書の「ドロップ→拾得→インベントリへ格納→プレイヤーが明示的にUncut Gemを使用→Gem Cutting画面」というワンクッション挟むフローと異なっていた点、および「サポートジェムは要件さえ満たせば誰でも自由に装着可能」という前回の簡略化が、仕様書ではSupport GemもUncut Support Gemのドロップ+鑑定を経る点と食い違っていた点。
- **`SkillGemPickupComponent`を`isSpirit: bool`から`GemPickupKind`(Skill/Support/Spirit の3種)へ変更**: `CollisionSystem`のドロップ抽選を2択から3択(60% Skill / 20% Support / 20% Spirit)へ。Support Gemも他の2種と同じくドロップ+鑑定が必須になった(前回の「誰でも自由に装着できる」簡略化を撤回)。
- **Uncut Gemは拾った瞬間に鑑定せず、`SkillGemInventoryComponent.pendingUncutGems`(簡易インベントリ、上限`kPendingCapacity`=8)へ格納**: `ItemPickupSystem`は拾得時に`GemIdentifySystem`を直接開かなくなり(そのため`Update()`から`GemIdentifySystem&`引数を削除)、代わりに`PendingUncutGem{level, kind}`をキューへpush(満杯時は「storage full」表示で拒否)。プレイヤーは`SkillGemSystem`(Gキー)画面上部に新設した「Uncut Gems」チップ一覧から明示的にクリックして`GemIdentifySystem`(Gem Cutting)を開く。
- **`GemIdentifySystem`をキューindex駆動に全面書き換え**: 旧`Open(int level, bool isSpirit, Entity pickupEntity)`(ワールド上のピックアップentityを直接destroyする方式)を`Open(int pendingIndex, int level, GemPickupKind kind)`へ変更。候補リストは3種で分岐(Skill/Spiritは`SkillGemData`をAura判定でフィルタし既存より高レベルなら鑑定可、Supportは`SupportGemData`から未所持のもののみ)。鑑定確定時は`pendingUncutGems`から該当indexを`erase`する(キャンセル時はキューに残したままパネルを閉じるだけ)。
- **`SkillGemSystem`のサポートピッカーを所持ベースへ変更**: `SupportOptions`は`SupportGemData`の全カタログではなく`gemInventory.ownedGems`内の`isSupport==true`エントリのみを(Tags適合フィルタと組み合わせて)列挙するよう変更。
- **セーブ/ロード**: `CampaignManager`に`m_savedPendingUncutGems`(`pendingUncutGems.count`/`pendingUncutGems.gem{i}.level`/`.kind`)を追加。`ownedGems`とは独立に復元する(鑑定前の最初の1個だけを持った状態でセーブしても、既所持ジェムが0件のせいで消えないように)。
- 段階的にビルド確認しながら進め、最終的に`/t:Rebuild`で0エラーを確認済み。Lineage Support Gem(通常のUncut Support Gemから生成できない特殊サポート)/Uncut GemのTier表示/Gem CuttingのSkill Icon・Tags等のリッチな表示項目/Act1クエスト報酬導線は、仕様書には記載があるが対応するゲームシステム・UIが本作に無いため今回は対象外とした(未実装のまま)。実機での対話的な操作確認は本セッションの制約により未実施。

**スキルジェム仕様の乖離を修正(スピリットON/OFF分離・サポートのTags適合フィルタ・武器要件)**。「スキルジェムの仕様が違う」との指摘を受け、ユーザーから詳細な仕様書(PoE2ジェムシステム仕様書 21-43章)を提示してもらい、既存実装との差分をすり合わせた上で、実現可能な3点(スピリットON/OFF分離・サポートのTags適合フィルタ・武器要件)に絞って修正した(ターゲティング方式/Charge/Weapon Set/Trigger Skill等、対応するゲームシステム自体が無い篇は対象外とユーザーに確認済み)。
- **スピリットジェムの登録とON/OFFを分離**: 従来はスロットへセットした瞬間にSpiritを予約し効果を即発動していたが、仕様書21.2/30.1-30.4に合わせ「登録(`SpiritAuraSystem::TryRegister`、効果なし)」と「ON/OFF切替(`TryToggleActive`、ここで初めてSpirit予約+効果適用/解除)」の2段階に分離。`SpiritGemLoadoutComponent`に`active[5]`を追加し、`SkillGemSystem`の各スピリット行にON/OFFトグルボタンを新設。登録済みジェムを別のものへ差し替える際、ONだったら自動的に先にOFF化してから差し替える(古い効果が残留しない)。セーブ形式にも`auraLoadout.active{i}`を追加(`CampaignManager`)。
- **サポートジェムをSkill Tagsで適合フィルタ**: 従来は全8種のサポートがどのスキルにも無条件で装着可能だったが、仕様書29.2「Skill Tags確認→Support条件と比較→使用可能Supportのみ表示」に合わせ、`SkillTag`ビットマスク(Attack/Spell/Melee/Projectile/AreaEffect/Duration/Movement/Physical/Elemental/Chaos)を新設。各スキルのタグは`behaviorType`/`element`から機械的に導出(`SkillTags::TagsFor`、個別ジェムを手動タグ付けしない、既存の`GemAttribute`導出と同じ方針)。各サポートに`requiredTag`(適合に必要な単一タグ、`None`なら誰でも装着可)を設定し直し(例: Faster Attacks→Attack必須、Increased Area/Concentrated Effect→AreaEffect必須、Elemental Focus→Elemental必須、Added Damage/Efficiency→無条件)、ピッカーリストは適合するサポートのみ表示する。
- **Attack系スキルの武器要件**: 仕様書21.2/24「対応武器未装備→使用不可」に合わせ、`SkillTag::Attack`を持つスキル(Melee/GroundSlam/Projectile系)をスキルスロットへ登録する際、武器スロット(`EquipSlot::Weapon`)が空だと拒否するチェックを`SkillGemSystem::AssignGem`に追加。本作は武器種別(Bow/Mace等)を区別しない単一の武器スロットのみのため、「対応武器」ではなく「武器スロットが埋まっているか」で簡略化した。



**クラッシュ診断機構を新規追加(`game.log`/`crash.dmp`)**。ユーザーから「町から次のゾーンへ移動したタイミングでエラーダイアログが出て落ちた」と報告を受けたが、間欠的で再現しないことがあり、かつ原因コードを`SkillGemSystem`/`GemIdentifySystem`/`CollisionSystem`のジェムドロップ処理/`ZoneBuilder`の突進アーケタイプ/`CampaignManager`のセーブ復元処理など広範囲にわたって精査したが確実な原因を特定できなかった。そのため`Programs/System/Main/Main.cpp`に恒久的な診断機構を追加:
- `spdlog`のデフォルトロガーをコンソールのみから`game.log`ファイル出力へ変更(warning以上は即flush、クラッシュでコンソールが消えてもログが残る)。
- `main()`を`try/catch`で囲み、C++例外(`EntityObject::GetComponent<T>()`がスローする`std::runtime_error("Component not found")`等)が捕捉されずに落ちる場合、その`what()`を`game.log`に記録してから再送出する。
- `SetUnhandledExceptionFilter`でWindows SEH例外(アクセス違反等、`/EHsc`のC++ try/catchでは捕捉できない種類のクラッシュ)も捕捉し、例外コード/発生アドレスを記録した上で`crash.dmp`(Visual Studio/WinDbgで開いて正確なクラッシュ箇所を特定可能なミニダンプ)を書き出す。
- これにより次回同じエラーで落ちた際、ダイアログの文字を手打ちで共有してもらう必要がなくなり、`game.log`と`crash.dmp`を見れば原因箇所を直接特定できる。

**スキルジェムシステムをPoE2準拠へ全面刷新(ジェムレベル/鑑定/サポートジェム/ソケット/ステータス要件)**。「スキルジェム（レベル１～２０）を敵がドロップ...」という大規模な仕様変更依頼を受け、実装前にEnterPlanModeで既存アーキテクチャ(`SkillGemData`/`SkillGemSystem`/`SpiritAuraSystem`/セーブ形式)を調査した上でユーザーに設計上の分岐点(ジェムレベルの意味/サポートジェムの内容/統合スロット数/ソケット拡張アイテムの実装方法)を確認し、承認を得てから実装した。実際のPoE2仕様([Game8: How to Increase Support Gem Slots](https://game8.co/games/Path-of-Exile-2/archives/489077)等で調査、初期2ソケット・Jeweller's Orbで最大5まで拡張)も踏まえている。
- **ジェムの個体管理へ移行**: 従来`SkillGemInventoryComponent.unlockedGemIds`は単なる所持フラグ(`vector<int>`)だったが、`OwnedGemInstance`(`gemId`/`isSupport`/`level`/`maxSockets`/`supportGemIds[5]`)を持つ`vector<OwnedGemInstance>`(`ownedGems`)へ全面移行。1つのgemIdにつき所持インスタンスは常に1つ(同じジェムを再鑑定するとレベルが上書きされるだけで重複しない)。
- **未鑑定ジェムのドロップ→鑑定フロー**: モンスタードロップは具体的なスキルではなく「レベル1-20+スキル/スピリット種別」だけをロールした未鑑定ジェム(`SkillGemPickupComponent`)。レベルは3乗で低レベル側に偏らせた分布(`CollisionSystem`)でロールし、19-20が稀になるようにした。拾ってクリックすると新設の`GemIdentifySystem`(`External/ECS/Systems/UI/GemIdentifySystem.h`)が開き、その種別に合う未所持(または今回の方が高レベルな)候補一覧から選んで初めて`ownedGems`に入る。「Leave on ground」でキャンセルすればピックアップ自体はその場に残る。
- **レベルスケーリング**: `SkillGemScaling.h`に`ItemFactory`のitemLevel慣習(`1+level*係数`の乗算)を踏襲した関数群を新設。レベルが上がるほど①要件(STR/DEX/INT、`RequiredStat`)②マナ/Spirit消費(`ScaledMpCost`/`ScaledSpiritCost`)③ダメージ(`ScaledDamage`)が全て増加する(「レベルが高いほどステータス要求が高く、マナ消費が激しく、火力は出る」という要望通り)。
- **サポートジェム(新規カテゴリ)**: `SupportGemData.h`にPoE2を参考にした8種(Added Damage/Brutality/Faster Attacks/Efficiency/Increased Area/Increased Duration/Concentrated Effect/Elemental Focus)を新設。ダメージ/クールダウン/マナ消費/範囲/持続時間への乗算のみで完結させ実装を単純化。スキルジェムはドロップ制だが、サポートジェムは要件(STR/DEX/INT)さえ満たせば誰でも自由にどのソケットへも装着できる設計にした(ドロップの仕組みを二重に作る手間を省いた簡略化)。
- **ソケット(2〜5、Jeweller's Orbで拡張)**: スキルジェムは初期2ソケットを持ち(`OwnedGemInstance::maxSockets`)、新規`CurrencyType::JewellersOrb`(既存の`regretOrbs`と同じ「所持数として蓄積し、対象を選んでから消費する」パターン、`CharacterStatsComponent::jewellersOrbs`)を`SkillGemSystem`画面上のボタンで消費して1つずつ最大5まで拡張する。スピリット(Aura)ジェムにはソケットを持たせない(効果がフラットなステータス加算のみで、サポートの乗算効果と噛み合わないため)。
- **`SkillGemSystem`(Gキー)の全面改修**: スキル5枠+スピリット5枠(2→5に拡張)=計10枠の統合プール。同じスキルの複数スロット重複セットを禁止。割当時にレベル・要件(STR/DEX/INT)を判定し未達成なら拒否(`SpiritAuraSystem::TryAssign`の既存の「Spirit不足で拒否」パターンを流用・拡張)。各スキル行にレベル表示+サポートジェムソケット(クリックでサポート専用ピッカーに切替)+ソケット拡張ボタンを追加。ピッカーリストでは要件未達成の候補を赤字表示。
- **セーブ/ロード**: `CampaignManager`の`m_savedUnlockedGems`(`vector<int>`)を`m_savedOwnedGems`(`vector<OwnedGemInstance>`)へ、`m_savedAuraLoadout`を`array<int,2>`→`array<int,5>`へ変更。旧セーブ(`ownedGems.count`キー自体が存在しない)は`GetI`のデフォルト値で自然に空リストへフォールバックし、既存の「空なら`EntitySpawner`の初期装備のまま」ガードパターンでそのまま救済される。`GameScene`の復元処理はカタログの単純コピーではなく`SkillGemScaling::BuildEquippedSkillData`(レベルスケーリング+サポート適用込み)を通すよう統一し、セーブされたキャラのスキルが寸分違わず復元されるようにした。
- **`EntitySpawner`のクリーンアップ**: 従来Spark/Thunder Slam/Lightning Warp/Lightning Ballの値を`SkillGemData.h`と完全に重複したハードコードで持っていた技術的負債を解消し、`SkillGemScaling::BuildEquippedSkillData`経由でカタログから組み立てるよう統一。
- 実装は段階的にビルド確認しながら進め、最終的にソリューション全体の`/t:Rebuild`で0エラーを確認済み。ただし実機での対話的な操作確認(ドロップ→鑑定→装着→ソケット→戦闘でのダメージ反映、セーブ&ロード往復)は本セッションの制約(安全な自動入力手段が無い)により未実施。次回以降、実際にプレイしての確認が必要。

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

**キャラクターシートをPoE2本家の実際の構成に合わせて全面刷新**: 「ステータスの表示に装備はいらない(Iキーで見れる)、ステータス表示自体も本家PoE2と違う、調べて全く同じにして」との指摘を受け、本家の構成を調査([PoE 2 Guide: Character Sheet Explained](https://mobalytics.gg/poe-2/guides/character-sheet)、[Stats & Attributes | PoE2 Wiki](https://pathofexile2.wiki.fextralife.com/Stats+&+Attributes)等)。本家の実際の構成は「概要(Lv/クラス/場所)→属性(STR/DEX/INT、アバター右側)→主要ステータス(Life/Mana/Spirit/耐性)→詳細な防御内訳(Energy Shield/Armour/Evasion/Block等)→その他(移動速度等)」で、**装備欄は無く**(インベントリ側で確認する設計)、**攻撃力/クリティカル等の攻撃系ステータスも一切表示されない**(本家はスキルジェムごとに個別の攻撃力パネルを持つ設計のため、キャラクターシート自体はビルドの土台となる防御・リソース系ステータスに特化している)ことが判明。これに合わせて全面書き換え:
- **装備欄を完全削除**(該当のホバー処理/ツールチップ描画関数も含め未使用コードを削除)。
- **Atk/Crit Chance/Crit Multiplierを削除**(本家同様キャラクターシートには攻撃系ステータスを置かない方針に変更)。
- **表示順を本家の区分(概要→属性→主要ステータス→詳細防御→その他)に合わせて並び替え**。
- 本家の7大ステータス(Life/Mana/Spirit/Energy Shield/Armour/Evasion/Block)のうち**Spiritが従来まったく表示されていなかった**(データ自体は`CharacterStatsComponent::maxSpirit`/`currentSpirit`として既に存在)ため新規に追加。
- Gold(ゴールド)は本家のキャラクターシートには存在しない情報のため削除(Vendor画面で確認可能)。
- 本家に存在するBlock(ブロック率)・Charges(チャージ)・ダメージ回避/変換の内訳セクションは、対応するゲームシステム自体が本実装に無いため今回は追加していない(将来実装する場合の拡張余地として`ROADMAP`に記録)。

**キャラクターシートの情報量をPoE2本家に合わせて拡張**: 「ライフ/ES/マナ/スピリットの量やアーマー/回避/ブロック/各耐性/DEX/INT/STR、ライフの詳細(最大値・秒間回復量合計)、ESも最大値と秒間回復、をPoE2を参考に増やして」との指摘。本家の実際の防御ステータス欄は最大値だけでなく秒間回復量も併記する仕様([Character screen | PoE Wiki](https://pathofexile.fandom.com/wiki/Character_screen)、[PoE 2 Guide: Character Sheet Explained](https://mobalytics.gg/poe-2/guides/character-sheet)等で確認)と分かったため、以下を追加:
- **生命力/マナに秒間自動回復量を併記**: 既存の`CharacterStatsComponent::healthRegen`/`manaRegen`(いずれも実装済みの固定値、`SkillSystem::Update`が`currentHP += healthRegen * dt`のように毎フレーム適用)をそのまま表示に追加しただけで、新規ロジックは無い。
- **エナジーシールドに秒間回復量を併記**: `StatusEffectSystem`の実装(被弾で`esRegenDelay=3.0f`にリセットされ、0まで減った後は`maxES * 0.33f`/秒で回復)に合わせて「回復 X/秒, 被弾から3秒後」と表記。
- **ブロック率を新規追加**: 本実装には盾やブロック付与装備スロットが無く、ブロックを増加させる手段が一切無いため実質常に0%になるが、本家同様「実データとして持つ値をそのまま表示する」設計に合わせて`CharacterStatsComponent::blockChance`(初期値0.0f)を新設し表示に追加した(ハードコードした0%ではなく、将来ブロック源を実装した際にそのまま繋ぎ込める)。`CampaignManager::WriteStats`/`ReadStats`にも同時にシリアライズを追加(`label`と同じ保存漏れバグを繰り返さないため)。
- Charges(帯電/激怒/勇敢チャージ)・ダメージ回避/変換の内訳は引き続き対応システム自体が無いため保留。

**キャラクターシートの見た目を本家PoE2の実際のパネル構造に合わせて再構築**: 「ステータス表示をPoE2と同じにして」との指摘を受け、それまでの「1つの`std::ostringstream`をひたすら改行で積む」実装をやめ、本家の character panel が区分ごとに見出し+区切り線で仕切られ、属性(STR/DEX/INT)は横並び、耐性は元素ごとに色分けされたバーで上限(75%)への充填率を可視化する、という構造を再現した。
- `section()`/`line()`/`gap()`ラムダで見出し(区切り線付き)・本文行・余白を統一的に描画するよう再構成(以前の改行だらけのostringstreamを廃止)。
- 属性STR/DEX/INTを横1行に並べ、パッシブツリーと同じ配色(STR=赤/DEX=緑/INT=青)を適用。
- 耐性4種(火/冷気/電気/カオス)に本家準拠の元素配色(火=オレンジ/冷気=水色/電気=黄/カオス=マゼンタ)を適用し、各行の下に`DrawBar()`(新設ヘルパー)で75%上限に対する充填率バーを描画。
- 見出し(概要/属性/主要ステータス/詳細な防御/その他)を金色テキスト+区切り線で表示し、本家の区分けされたパネル構造に近づけた。

**フォローアップ: 耐性表示をバーから数値に戻し、幕クリアによる耐性ペナルティを新規実装**。「耐性の表示は数値でいい」との指摘を受け、前回追加した75%上限バー(`DrawBar`)を撤去し元の数値表示(色分けは維持)に戻した。加えて「マップのレベル(アクトが進むと)耐性が強制的に下がり装備の要求度が上がる」との要望を受け、PoE2本家の実際の仕様([PoE 2 Guide: Resistances Explained](https://mobalytics.gg/poe-2/guides/resistances)等で確認: 幕を1つクリアするごとに全属性耐性(カオス耐性は対象外)へ-10%の永続ペナルティが課され、全6幕クリアで最大-60%になる)をそのまま実装した。
- `CampaignManager::CompleteCurrentZoneAndAdvance`の`m_actIndex++`(=幕クリア)のタイミングで`m_savedEquipment.baseStats`の`fireRes`/`iceRes`/`lightningRes`を-10%する処理を追加。本実装は非エンドゲームの幕が丁度6つ(Act1/Act2/幕間I/Act3/Act4/幕間II)あるため、本家と同じく全クリアで-60%になる。
- `equipment.baseStats`に加算する方式を選んだのは、`EquipmentSystem::RecalculateStats`が毎回`live = equipment.baseStats`から再構築するため(既存のレベルアップ/パッシブツリー加点と同じパターン、`AI/DECISIONS.md`参照)。セーブ/ロードの`WriteStats`/`ReadStats`は既存の`fireRes`等のフィールドをそのまま使うため追加シリアライズは不要。
- ペナルティは装備の耐性ロールで打ち消す設計(本家と同じ「幕を進めるほど耐性持ちの装備が必要になる」体験)のため、上限クランプ等は設けていない。

**フォローアップ: ブロック率を実データとして機能させる**。「他に進める作業ある?」に対しユーザーが選択した項目。これまで`CharacterStatsComponent::blockChance`は表示欄だけあって常に0%で終わる張りぼてだったため、実際に機能する経路を追加した。
- `AffixStat::BlockChance`を新設し、`ItemFactory::SuffixPool`にサフィックスとして追加(本家PoE2はブロック率は盾専用だが、本実装には盾スロット自体が存在せず、CritChance/MoveSpeed等も既に「どの装備にも乗る汎用サフィックス」として実装済みのため、既存の設計方針に合わせた)。
- `EquipmentSystem::ApplyAffix`/`RemoveAffix`にBlockChanceのケースを追加(耐性と同じく上限75%)。
- `CombatMath::ApplyDamage`の先頭で`RollBlock(target.blockChance)`をロールし、成功時はそのヒットのダメージを丸ごと無効化(PoE2本家同様、部分軽減ではなく完全回避)。命中判定(`RollHit`)の後・軽減計算の前という順序も本家に合わせた。呼び出し側(`CollisionSystem`)は`dealt > 0.0f`で被弾演出/出血等のailment発生可否を判定する既存ロジックのままで、ブロック時は自然に「ダメージ0」として扱われるため変更不要だった。
- **副次的に発見した実バグを修正**: `EquipmentSystem::RemoveAffix`の耐性4種が`(std::max)(0.0f, live.xxxRes - affix.value/100)`と0未満にならないようクランプしていたが、今回追加した幕クリア耐性ペナルティ(基礎値がマイナスになりうる)と組み合わさると、パッシブ再割り振り(respec)やオーラ解除で耐性付与効果を除去する際に**マイナスのベースライン耐性ごと0へ戻ってしまう**(本来のペナルティが消える)回帰バグになるところだった。0クランプを撤去し単純な減算に変更して修正。

**フォローアップ: Fire/Chaos属性のスキルジェムを新規追加(スキルバリエーション拡充)**。既存の10種のスキルジェムを見直したところ、Physical(Thunder Slam/War Cry/Cleave)・Lightning(Spark/Lightning Warp/Lightning Ball)・Cold(Nova/Frost Bolt)は揃っている一方、`DamageElement`にはFire/Chaosも存在するのにこの2属性でダメージを与えるアクティブスキルが1つも無かった(耐性システムはFire/Chaos含む4属性想定なのに、プレイヤー側の攻撃手段が2属性抜けている状態)。`SkillGemData::BuildGems()`にid10「Fireball」(AreaEffect、既存のNovaと同じ規約でdamage=100% atk/range=160、Fire属性)とid11「Chaos Bolt」(Projectile、既存のFrost Boltと同じ規約でdamage=130% atk、Chaos属性)を追加。ドロップ抽選(`CollisionSystem`)・ジェム管理UI(`SkillGemSystem`)ともに`SkillGemData::Gems()`のサイズから動的に扱う設計のため、追加のみで両方に自動反映される(ハードコードされた個数/IDリストが無いことを確認済み)。新規の`SkillBehaviorType`やコンポーネントは不要で、既存の`AreaEffect`/`Projectile`挙動をそのまま再利用。

**フォローアップ: 召喚モンスターの属性/見た目が召喚元と無関係になっていたバグを修正**。Fire/Chaosスキルジェムの追加で敵側の属性一貫性も見直したところ、`EnemySummonSystem`が`EntitySpawner::CreateEnemy`で生成した召喚体(`addStats`)にHP/攻撃力はスケーリングして引き継ぐ一方、`contactDamageType`(デフォルトPhysical)と`CircleComponent`の色(デフォルト赤)をそのままにしていたのを発見。結果、例えば幕間(Chaos属性ゾーン)の召喚術士アーケタイプが呼び出す召喚体は、見た目も攻撃属性も常にデフォルトのPhysical/赤のままで、召喚元モンスターのゾーンテーマと無関係になっていた。召喚元の`stats.contactDamageType`と`CircleComponent.color`を召喚体へコピーするよう修正。

**パッシブツリー(Pキー)を全画面化し、実際に分岐するツリー構造へ全面刷新**。「パッシブツリーは全体の画面にしてできるだけ多く分岐もあるようにして」との指摘。従来は700x560固定の小パネルに、中心から4方向(攻撃/防御/速度・マナ/耐性)へ一直線に5ノードずつ伸びるだけ(分岐なし、単なる直線)の21ノードのツリーだった。
- **全画面化**: `PassiveTreeSystem::ComputeLayout`を、ウィンドウ中央の固定700x560パネルから、`kMargin`(30px)だけ余白を残した画面全体を使う方式に変更。ツリーは`PassiveTreeData::MaxRadius()`(全ノード中で中心から最も遠い距離)を基準に、ヘッダー/フッターを除いた利用可能領域へ収まる`scale`を毎フレーム動的計算して描画・当たり判定の両方に適用(`NodeScreenPos`)。ノードの描画半径とクリック判定の余白は画面ピクセル単位で固定し、ツリーが縮小されても文字・クリックしやすさは保つ。
- **実際に分岐するツリー構造**: `PassiveTreeData`を、直線チェーンのみだった旧実装から、任意の分岐(1ノードが複数の子ノードを持つ=フォーク)を表現できる再帰的な`StepNode`木構造+`AddChain`ビルダーへ全面書き換え。4方向それぞれが「幹2ノード→3方向へフォーク→各アームがさらに2ノードの後に再度2方向へフォーク(一部アームのみ)」という、実際に選択を迫られる分岐構造を持つ。結果、ノード総数は21→53(スタート含む)に増加し、分岐点(1ノードから2つ以上の子への分かれ道)は方向ごとに最大2箇所、計8箇所以上存在する。既存の`CanRemoveWithoutDisconnecting`(BFSで孤立ノード検出)・`IsAllocated`等のロジックは`neighbors`リストベースの汎用実装のままなので、分岐構造が複雑になっても変更不要だった。
- 新設した`AffixStat::BlockChance`ノードを防御方向の2箇所に追加したため、`PassiveTreeSystem`内の(`ItemUIHelpers`とは別に持つ)ローカルな`IsPercentStat`にも`BlockChance`ケースを追加(無いと新ノードのツールチップで%記号が付かない表示崩れになるところだった)。

**フォローアップ: パッシブツリーにマウスドラッグでのパン+ホイールでのズームを追加**。「マウスクリックで移動やマウスホイールでズームできたりするようにして」との指摘(53ノードへ拡張した直後、全画面化しても密集する分岐が見づらい箇所への対応)。
- **ホイールズーム**: `InputManager`にマウスホイールの蓄積量を新設(`ResetMouseWheelDelta`/`AddMouseWheelDelta`/`GetMouseWheelDelta`)。SFMLのホイールはポーリングでなくイベント(`sf::Event::MouseWheelScrolled`)でしか取れないため、`Application::ProcessEvents`で毎フレーム先頭にリセットしてから蓄積し、ゲーム側は`GetMouseWheelDelta()`で読むだけで良い形にした(既存の`KeyInput`/`MouseInput`のポーリングAPIと同じ感覚で使える)。ズームはカーソル位置を基準に行う(カーソル直下の座標が画面上で動かないよう`m_panOffset`を逆算)、PoE本家のツリーズームと同じ挙動。
- **左ドラッグでパン**: `MouseInput`に「離した瞬間」を返すAPIが無いため、`GetMouse`(押されている間)の前フレームとの差分で離した瞬間を自前検出。押下時点から`kDragThreshold`(6px)以上動いたら「ドラッグ」とみなしてパン、動かなければ「クリック」としてノード選択/Confirm/Cancelボタンを従来通り処理する(=わずかに手ブレしても誤ってノード選択が外れたりしない)。
- **表示クリッピング**: パン/ズームでノードがヘッダーやフッターのボタン領域に被って描画されないよう、ツリー本体(接続線+ノード)の描画だけツリー領域にビューポートを絞った`sf::View`を一時的に適用(ヘッダー/フッター/ボタンは従来通りデフォルトビューで描画)。
- パネルを開き直すたびにパン/ズームは初期状態(全体表示)にリセットされる(`Toggle()`)。

**フォローアップ: パッシブツリーのノードにホバーツールチップを追加**。「パッシブの円にカーソルがあったら情報の表示をして」との指摘。従来はクリックして選択しないと効果内容(効果名/値/割り振り済みか)が footer に表示されなかったが、53ノードに増えた今、1つずつクリックして確認するのは非効率なため、カーソルを乗せるだけでマウス追従のツールチップ(`DrawNodeTooltip`)を表示するようにした。未割り振りノードは隣接ノードが割り振り済みかどうかも見て「Click to allocate」/「(Not reachable yet)」を出し分ける(クリックしても`TryAllocate`が拒否される理由が事前に分かるようにした)。ドラッグ(パン)中はカーソル追従でちらつくため非表示にする。既存のクリック選択時のfooter情報表示は変更せず併存させている(確定前の最終確認用として残す)。

**フォローアップ: ズームアウト時にノード円が重なるバグを修正**。「ズームしたときにパッシブの円のサイズが固定なのでズームアウトしたときに重なってしまう」との指摘。ノード間隔は`layout.scale`(`baseScale * m_zoom`)で縮小されるのに対し、円の半径`kNodeRadius`/`kStartNodeRadius`は画面ピクセル固定だったため、ズームアウトするほど間隔に対して円が相対的に大きくなり重なっていた。`NodeRadiusPx()`を新設し、半径も`m_zoom`に比例して縮小/拡大するように変更(`kMinNodeRadiusPx`4px 〜 `kMaxNodeRadiusPx`22pxでクランプし、極端なズームでも消えたり肥大化しすぎたりしないようにした)。クリック判定(`HitTestNode`)の当たり半径も同じ関数を使うため見た目と一致する。

**フォローアップ: ノード円を少し小さく調整**。「もう少し円小さくていい」との指摘を受け、`kNodeRadius`10→7px・`kStartNodeRadius`12→9px、クランプ範囲も`kMinNodeRadiusPx`4→3px・`kMaxNodeRadiusPx`22→16pxへ縮小(比率は維持)。クリック判定の当たり半径は変えていない(見た目だけ小さくし、クリックのしやすさは維持)。

**Escapeキーで全メニューを閉じられるように統一**。「基本的に何かしらのメニューはESCで抜けれるようにしといて」との指摘。従来は各パネルに個別の開閉キー(I/C/G/P/O/Vendorクリック)しかなく、Escapeで閉じる手段が無かった。`GameScene::Update`に、`awaitingRebind`でない時にEscapeが押されたら6つの全メニュー(CharacterSheet/Inventory/SkillGem/PassiveTree/Vendor/KeyBind)を`Close()`する処理を追加。`KeyBindSystem`は既にEscapeを「キー再割り当て待ち状態のキャンセル」に内部使用しており(`m_awaitingKey`時のみ)、これはパネル自体を閉じるものではないため、`!awaitingRebind`ガードで衝突を避けた(既存の他の開閉キー判定と同じガード)。Escapeは`Shared/NAMING.md`/`AI/DECISIONS.md`が定める「メニュー内の固定キー(Up/Down/Enter/Backspace等)」の仲間として扱い、`GameAction`(リバインド可能操作)には追加していない。

**フォローアップ: Escapeキー追加に伴う横断監査で予約キー漏れを発見・修正**。「作業を進めて」の指示を受け、直前のEscape対応を監査。Escapeを`GameScene::Update`の固定キーとして新設したが、`KeyBindSystem::IsReserved`の予約キー一覧にEscapeを追加し忘れていたことを発見。これは以前(Oキー)で一度発生した同系統の不具合(既知の制約に記録済み)で、未修正のままだと例えばSkill1をEscapeへ再割り当てした場合、Escape押下でスキル発動と全メニュー閉じが同時に起きてしまうところだった。`kReserved`配列にEscapeを追加(9→10要素)して修正。

**フォローアップ: スキルジェムUIに属性表示を追加**。「作業を進めて」の指示を受けた監査。Fire/Chaosスキル(Fireball/Chaos Bolt)を新規追加したものの、`SkillGemSystem`(Gキー)の装備スロット行・ジェム選択リストのどちらにも属性(元素)を表示する箇所が無く、`UISystem`のHUDアイコンも挙動タイプ別の色分け(Melee=赤/Dash=シアン/その他は一律緑)のみで元素を区別していないことに気付いた。これでは新規追加した2属性がプレイヤーから見分けられず、追加した意味が薄れてしまう。`SkillGemSystem`にダメージを与える挙動タイプ(Melee/Projectile/AreaEffect/Spark/GroundSlam/LightningWarp/LightningBall)かどうかを判定する`DealsElementalDamage()`と英語の属性名を返す`ElementName()`を追加し、装備スロット行(例: `[E] Fireball  (cd 5.0s, 28 mp, Fire)`)とジェム選択リストの両方に属性名を追記。Dash/Buff/Auraは`element`フィールドが未使用のデフォルト値(Physical)のままなので意図的に対象外(Determinationオーラが「Physical」と表示されるような誤解を避けるため)。

**モンスター/スキルの種類を拡充**。「モンスター/スキルの種類を増やす」を選択。
- **スキル4種を追加(`SkillGemData.h`)**: 実装済みだが使われていなかった`SkillBehaviorType::Dash`(`SkillSystem`/`UISystem`のシアン色アイコンは既に対応済みだった)を使う「Flame Dash」(通常ロールとは別枠のダッシュ、ダメージ無し)。防御系オーラをArmour(Determination)/ES(Discipline)の2種からEvasion「Grace」を追加し3種へ。Fire/Chaosをそれぞれもう1種追加し、Cold(2種)と同数の2種に揃えつつ、Melee(Fire「Immolate」)・AreaEffect(Chaos「Soul Rend」)の元素バリエーションも増やした。全て`SkillGemData::Gems()`のサイズから動的に扱われる既存の仕組み(ドロップ抽選/ジェム選択リスト)にそのまま乗る。
- **新規モンスターアーケタイプ「突進(Charger)」を追加**: 既存の接触/遠距離/範囲/召喚の4アーケタイプに次ぐ5つ目。プレイヤーが射程内に入ると足を止めてテレグラフ(赤く点滅)し、終了時に捕捉した方向へ`chargeSpeedMultiplier`(3.5倍速)で直進する。`EnemyAreaAttackSystem`等と異なり専用のダメージ適用処理は持たず、突進中に接触すれば既存の`CollisionSystem`の通常接触ダメージがそのままヒットとして扱われる(新規ダメージ計算コードが不要で実装が小さく済む)。新規`ChargerComponent`(`Components/Chara/Charger.h`)+`EnemyChargeSystem`(状態遷移専用、`EnemyAISystem`側は移動のみ担当という既存の関心分離パターンを踏襲)、`ZoneBuilder::SpawnTrash`のアーケタイプ抽選に15%枠で追加(通常近接55%→40%へ圧縮、他は変更なし)。

**フォローアップ: `PoE2_仕様書.xlsx`を最新化**。「他に進める作業ある?」に対しユーザーが選択した項目。直近で追加したブロック率(汎用サフィックス化)・幕クリア耐性ペナルティの実装は、本家の仕様(Blockは盾専用/Act1-3+Cruel再攻略の6章構成)と意図的に異なる簡略実装のため、既存の「実装状況(2026-09時点)」行の慣例(セクション末尾へ`大分類`ごとに実装との乖離を1行で追記)に倣い、2行を新規追加(No 462: 02_プレイヤーキャラクター仕様、No 463: 12_マップ・ワールド)。前者はResistances/Block/Spell Suppressionの差分、後者はAct構成(6つの固有エリアの一本道 vs 本家のNormal+Cruel再攻略)と、両者を跨ぐ「章クリア時-10%×6章=-60%」という数値上の対応関係を記載。

**フォローアップ: 耐性ペナルティ発生時のHUD通知を追加**。「他に進める作業ある?」に対しユーザーが選択した項目。それまでは幕クリアで耐性が-10%されても画面上に何も表示されず気付きにくかったため、`CampaignManager`に`ConsumePendingResPenaltyNotice(int&)`を新設(ペナルティ適用時に立てる`m_pendingResPenaltyNotice`フラグを1回だけ消費し、通算ペナルティ%を返す)。`GameScene`のプレイヤー生成直後(`CompleteCurrentZoneAndAdvance`でシーンが張り替わった後の新ゾーン読み込み時)でこれを呼び、trueが返れば`itemPickupSystem->lastMessage`に「幕クリア: 全耐性 -10% (通算 -X%)」を4秒間(通常のアイテム取得トースト2.5秒より長め、重要な情報のため)表示する。このフラグはディスクへ永続化しない(同一セッション内、幕クリア直後の1回だけ意味を持つ一時通知のため)。

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

1. **実機での目視検証** — 今セッション後半 (アイテム/装備システム以降、インベントリUI・パッシブツリーUI・Vendor含む) は分離テスト・ビルド成功のみでの検証。実際にプレイしての確認が必要。**特にスキルジェムシステム全面刷新(ジェムレベル/鑑定UI/サポートジェムソケット/セーブロード往復)は変更規模が大きいため優先的に実機確認したい。**
2. **サウンド** — 効果音・BGMが `test.mp3` 以外未整備 (アセット不足によりブロック中)。
3. **スキル/モンスターバリエーション拡充(継続)** — スキルジェムの自由付け替え、War Cry/Nova、モンスターの遠距離攻撃/範囲攻撃/召喚アーケタイプを実装済み(下記「済」参照)。さらにジェム種類・モンスター行動パターンを増やす余地あり。

## 既知の制約

- ~~自作ECSは高速化したが、まだ簡易実装 (Increased/More修飾はダメージ計算上簡略化されている)。~~ → 修正済み。`CharacterStatsComponent`に`increasedAttackDamage`/`increasedMoveSpeed`の合算専用フィールドを追加し、`EquipmentSystem::ApplyAffix`(装備)・`PassiveTreeSystem`経由の呼び出し(パッシブ)とも同系統%は加算するだけに変更、`RecalculateStats`の最後で1回だけ`atk`/`moveSpeed`に掛ける方式(PoEのIncreased/Reduced合算方式)へ修正。あわせて`leechRateCap`が0.02(2%/秒)になっていた数値ミスを0.20(本家PoE2準拠)へ修正。いずれもビルド未検証(環境制約は下記)だが、`EquipmentSystem.h`単体はcl.exe構文チェックでエラー無しを確認済み。あわせて`CampaignManager`の`WriteStats`/`ReadStats`に新設2フィールドのシリアライズを追加(これを忘れるとセーブ&ロードでパッシブの%系ボーナスが消える回帰バグになるところだった)。`CampaignManager.cpp`は`/utf-8`込みでcl.exe構文チェック済み。**追記: その後の本セッションでソリューション全体のフルビルドを何十回も成功させており(MSBuild、0エラー)、この変更もビルド検証済みと言える。**
- `KeyBindSystem::IsReserved`にOキー(キーバインド画面自体の固定トグルキー)が含まれておらず、何らかのアクションをOへ再割り当てすると同時に両方トリガーされる不具合を発見・修正。予約キー一覧に追加。
- `InventorySystem`同様のUB(`std::clamp(idx,0,size-1)`をsize==0で呼ぶ)が他のUI(SkillGemSystem/CharacterSheetSystem/KeyBindSystem/VendorSystem)には無いことを確認済み(VendorSystemは元々empty時に0へフォールバックする実装済みだった)。
- **旧Todo「原因不明の間欠的クラッシュ」は解消を確認**: 上記の`InventorySystem::ClampSelection`(size==0でのUB)が有力な原因候補だったが、その後の本セッション内で`InventorySystem`をPoE2本家準拠の占有グリッド式ドラッグ&ドロップへ全面刷新した際に、選択インデックス方式のUI自体を廃止(ヒットテスト方式に置き換え)したため、`m_selectedIndex`/`ClampSelection`はコード上に存在しなくなった。`grep`で該当シンボルが0件であることを確認済み。結果的にバグの原因コードごと消滅したため、Todoから除去し既知の制約側にこの経緯だけ記録する。VendorSystemは選択インデックス方式のまま残っているが、`count == 0`ガードで安全(上記)。
- **`MovementSystem`を削除(実プレイに影響するバグを発見)**: プレイヤーの移動は`InputSystem`が既に入力→正規化8方向ベクトル→`VelocityComponent`まで一貫して計算していたが、その後段で`MovementSystem`が`input.moveLeft`/`moveRight`のみを見てX速度を`±moveSpeed`(非正規化)で再上書きし、Y速度はInputSystemの正規化済みの値を残す、という中途半端な二重処理になっていた。結果、斜め移動時にX成分だけ非正規化された値になり、実効速度が正規化時より最大約1.22倍速くなる(例: moveSpeed=50なら理論上50のところ実測≈61.2)バグが常時発生していた。`PhysicsSystem`は`VelocityComponent`を持つ全エンティティの位置積分を汎用的に行うため`MovementSystem`はプレイヤー専用の冗長な処理でしかなく、他に依存箇所も無かったため、ファイルごと削除(`GameScene.h/.cpp`・`Game-SFML.vcxproj`からも参照除去)。**追記: その後の多数のフルビルド成功によりビルド検証済み(`MovementSystem`関連ファイルが存在しないことも確認済み)。実機での移動フィーリング再確認のみ引き続き未実施。**
- **Spark/Thunder Slam(GroundSlam)/Lightning Ballのダメージ計算式を修正**: この3スキルだけ`SkillSystem::ActivateSkill`が`proj.damage = stats.atk`(atk全量そのまま)としており、`SkillGemData`側の`damage`値(spark=25/slam=120/ball=40、いずれも他スキルと同じ「% of atk」の想定値)が完全に無視されていた。Melee/Projectile/AreaEffect/LightningWarpは`stats.atk * (skill.damage/100.0f)`と正しく%スケーリングしており計算式が不統一だった。ユーザー確認の上、この3スキルも同じ%スケーリング式に統一(体感火力は下がる: 例えばSparkは1発あたり従来の25%に低下)。`SkillGemData.h`に「% of atk」の注記を追記。**追記: その後の多数のフルビルド成功によりビルド検証済み(3スキルとも`stats.atk * (skill.damage / 100.0f)`式に統一済みであることも再確認)。実機でのバランス確認のみ引き続き未実施。**
- ユーザーの他アプリ (Apex Legends / Citra) 使用中は、キーボード状態をグローバルにポーリングする入力方式 (`sf::Keyboard::isKeyPressed`) の都合上、安全な自動入力での実機テストができない制約がある。
