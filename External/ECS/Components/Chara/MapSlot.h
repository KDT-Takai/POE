#pragma once

// このマップアタempト内でのスポーン枠番号。トラッシュ/Rareはループのインデックス
// (0..trashCount-1、freeSlotsのシャッフル順自体がマップシードで決定論的なので同じ
// 枠は常に同じ座標に対応する)、ボスは-1固定。死亡すると`CampaignManager::
// NotifyEnemySlotDead`へ記録され、同じマップアタempト中に再構築(町へ戻って再入場等)
// してもその枠だけ湧かなくなる(ZoneBuilder::Build、"同じ個体で"という指示対応)。
// 生存中に離脱した場合はcurrentHPも`CampaignManager::SetEnemySlotHp`で記録され、
// 再構築時にそのHPから再開する(EnemySummonSystemが戦闘中に追加召喚する雑魚には
// 付与しない、ロースター外の一時的な増援のため)。
struct MapSlotComponent {
    int slotIndex = -1;
};
