#pragma once

// マップ内に出現する帰還用ポータルの目印。(1)プレイヤーがスキル欄の隣のボタンを
// クリックして詠唱生成する、(2)レア敵/ボスを倒した場所に自動で出現する、の2経路がある
// (GameScene::TryStartPortalChannel / CollisionSystem::TrySpawnReturnPortalOnNotableKill)。
// クリックするとタウンへ帰還する(GameScene::ReturnToHubViaPortal、CampaignManager::
// ReturnToLastTown()を呼ぶだけなのでマップアタempt自体は継続=再入場可能なまま)。
struct MapReturnPortalTag {};
