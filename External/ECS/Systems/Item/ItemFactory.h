#pragma once
#include "Components/Item/Item.h"
#include <random>
#include <algorithm>

class ItemFactory {
public:
    static ItemComponent GenerateItem(EquipSlot slot, ItemRarity rarity, int itemLevel, std::mt19937& rng) {
        ItemComponent item;
        item.slot = slot;
        item.rarity = rarity;
        item.itemLevel = itemLevel;
        item.baseName = BaseNameFor(slot);
        item.baseValue = BaseValueFor(slot, itemLevel);

        int prefixCount = 0, suffixCount = 0;
        switch (rarity) {
        case ItemRarity::Normal: prefixCount = 0; suffixCount = 0; break;
        case ItemRarity::Magic: {
            std::uniform_int_distribution<int> pick(0, 1);
            prefixCount = pick(rng); suffixCount = 1 - prefixCount;
            break;
        }
        case ItemRarity::Rare: {
            std::uniform_int_distribution<int> pick(1, 3);
            prefixCount = pick(rng); suffixCount = pick(rng);
            break;
        }
        default: break;
        }

        auto prefixPool = PrefixPool();
        auto suffixPool = SuffixPool();
        std::shuffle(prefixPool.begin(), prefixPool.end(), rng);
        std::shuffle(suffixPool.begin(), suffixPool.end(), rng);

        std::uniform_real_distribution<float> variance(0.85f, 1.15f);
        for (int i = 0; i < prefixCount && i < static_cast<int>(prefixPool.size()); ++i) {
            ItemAffix affix = prefixPool[i];
            affix.value *= (1.0f + itemLevel * 0.03f) * variance(rng);
            item.affixes.push_back(affix);
        }
        for (int i = 0; i < suffixCount && i < static_cast<int>(suffixPool.size()); ++i) {
            ItemAffix affix = suffixPool[i];
            affix.value *= (1.0f + itemLevel * 0.03f) * variance(rng);
            item.affixes.push_back(affix);
        }

        return item;
    }

    // Waystone-category item (see Item.h) -- itemLevel is repurposed purely to feed
    // SellValue's existing formula (no separate Waystone sell-price formula needed).
    // rarity reuses RollRarity purely to gate how many map mods roll (Normal=0/Magic=1/
    // Rare=2-3), the same convention gear's prefix/suffix counts already use.
    static ItemComponent GenerateWaystone(int tier, std::mt19937& rng) {
        ItemComponent item;
        item.category = ItemCategory::Waystone;
        item.waystoneTier = tier;
        item.itemLevel = tier * 6;
        item.rarity = RollRarity(rng);
        item.baseName = "Waystone (Tier " + std::to_string(tier) + ")";

        int modCount = 0;
        switch (item.rarity) {
        case ItemRarity::Normal: modCount = 0; break;
        case ItemRarity::Magic: modCount = 1; break;
        case ItemRarity::Rare:
        case ItemRarity::Unique: {
            std::uniform_int_distribution<int> pick(2, 3);
            modCount = pick(rng);
            break;
        }
        }

        auto pool = WaystoneModPool();
        std::shuffle(pool.begin(), pool.end(), rng);
        std::uniform_real_distribution<float> variance(0.85f, 1.15f);
        for (int i = 0; i < modCount && i < static_cast<int>(pool.size()); ++i) {
            WaystoneMod mod = pool[i];
            mod.value *= (1.0f + tier * 0.02f) * variance(rng);
            item.waystoneMods.push_back(mod);
        }
        return item;
    }

    // An Uncut Gem (SkillGem-category item, see Item.h) -- level/kind are rolled, but
    // which specific skill/support/spirit it becomes is chosen later by the player
    // (right-click in InventorySystem to identify, see GemIdentifySystem). Consumed in
    // place on identification rather than replaced with a new item, so it keeps whatever
    // bag slot it was picked up into.
    static ItemComponent GenerateUncutSkillGem(int level, GemPickupKind kind) {
        ItemComponent item;
        item.category = ItemCategory::SkillGem;
        item.skillGemIdentified = false;
        item.skillGemIsSupport = (kind == GemPickupKind::Support);
        item.skillGemUncutKind = kind;
        item.skillGemLevel = level;
        item.itemLevel = level;
        item.rarity = ItemRarity::Magic;
        item.baseName = std::string("Uncut ") +
            (kind == GemPickupKind::Support ? "Support" : kind == GemPickupKind::Spirit ? "Spirit" : "Skill") + " Gem";
        return item;
    }

    // bonusPercent shifts both thresholds proportionally (e.g. a Waystone's "Increased
    // Item Rarity" map mod, see CollisionSystem::TrySpawnItemDrop) -- 0 (the default)
    // matches every other existing caller exactly.
    static ItemRarity RollRarity(std::mt19937& rng, float bonusPercent = 0.0f) {
        float scale = 1.0f + bonusPercent / 100.0f;
        std::uniform_real_distribution<float> roll(0.0f, 1.0f);
        float r = roll(rng);
        if (r < 0.08f * scale) return ItemRarity::Rare;
        if (r < 0.30f * scale) return ItemRarity::Magic;
        return ItemRarity::Normal;
    }

    static EquipSlot RollSlot(std::mt19937& rng) {
        std::uniform_int_distribution<int> pick(0, static_cast<int>(EquipSlot::Count) - 1);
        return static_cast<EquipSlot>(pick(rng));
    }

    static int SellValue(const ItemComponent& item) {
        float multiplier = 1.0f;
        switch (item.rarity) {
        case ItemRarity::Magic: multiplier = 2.0f; break;
        case ItemRarity::Rare: multiplier = 4.0f; break;
        case ItemRarity::Unique: multiplier = 8.0f; break;
        default: break;
        }
        return static_cast<int>((item.itemLevel * 2.0f + item.affixes.size() * 3.0f) * multiplier) + 1;
    }

    static ItemAffix RollRandomAffix(int itemLevel, std::mt19937& rng) {
        auto prefixPool = PrefixPool();
        auto suffixPool = SuffixPool();
        std::vector<ItemAffix> combined;
        combined.insert(combined.end(), prefixPool.begin(), prefixPool.end());
        combined.insert(combined.end(), suffixPool.begin(), suffixPool.end());

        std::uniform_int_distribution<int> pick(0, static_cast<int>(combined.size()) - 1);
        ItemAffix affix = combined[pick(rng)];

        std::uniform_real_distribution<float> variance(0.85f, 1.15f);
        affix.value *= (1.0f + itemLevel * 0.03f) * variance(rng);
        return affix;
    }

    // Rolls from just the prefix or just the suffix pool (e.g. Orb of Augmentation,
    // which must add the missing affix side rather than either at random).
    static ItemAffix RollAffixOfType(bool isPrefix, int itemLevel, std::mt19937& rng) {
        auto pool = isPrefix ? PrefixPool() : SuffixPool();
        std::uniform_int_distribution<int> pick(0, static_cast<int>(pool.size()) - 1);
        ItemAffix affix = pool[pick(rng)];

        std::uniform_real_distribution<float> variance(0.85f, 1.15f);
        affix.value *= (1.0f + itemLevel * 0.03f) * variance(rng);
        return affix;
    }

private:
    static std::string BaseNameFor(EquipSlot slot) {
        switch (slot) {
        case EquipSlot::Weapon: return "Sword";
        case EquipSlot::BodyArmour: return "Armour";
        case EquipSlot::Helmet: return "Helmet";
        case EquipSlot::Gloves: return "Gloves";
        case EquipSlot::Boots: return "Boots";
        case EquipSlot::Ring1:
        case EquipSlot::Ring2: return "Ring";
        case EquipSlot::Amulet: return "Amulet";
        case EquipSlot::Belt: return "Belt";
        default: return "Item";
        }
    }

    static float BaseValueFor(EquipSlot slot, int itemLevel) {
        switch (slot) {
        case EquipSlot::Weapon: return 5.0f + itemLevel * 1.5f;
        case EquipSlot::BodyArmour: return 8.0f + itemLevel * 2.0f;
        case EquipSlot::Helmet:
        case EquipSlot::Gloves:
        case EquipSlot::Boots: return 4.0f + itemLevel * 1.0f;
        default: return 0.0f;
        }
    }

    static std::vector<ItemAffix> PrefixPool() {
        return {
            { AffixStat::IncreasedAttackDamage, 15.0f, 1, true, "攻撃ダメージ増加" },
            { AffixStat::FlatArmour, 10.0f, 1, true, "アーマー" },
            { AffixStat::FlatEvasion, 10.0f, 1, true, "回避力" },
            { AffixStat::FlatES, 8.0f, 1, true, "エナジーシールド" },
        };
    }

    static std::vector<ItemAffix> SuffixPool() {
        return {
            { AffixStat::FireRes, 12.0f, 1, false, "火耐性" },
            { AffixStat::ColdRes, 12.0f, 1, false, "冷気耐性" },
            { AffixStat::LightningRes, 12.0f, 1, false, "電気耐性" },
            { AffixStat::ChaosRes, 6.0f, 1, false, "カオス耐性" },
            { AffixStat::FlatLife, 15.0f, 1, false, "生命力" },
            { AffixStat::FlatMana, 15.0f, 1, false, "マナ" },
            { AffixStat::CritChance, 2.0f, 1, false, "クリティカル率" },
            { AffixStat::MoveSpeed, 5.0f, 1, false, "移動速度" },
            { AffixStat::FlatAccuracy, 20.0f, 1, false, "命中率" },
            { AffixStat::BlockChance, 4.0f, 1, false, "ブロック率" },
            { AffixStat::FlatSpirit, 10.0f, 1, false, "スピリット" },
        };
    }

    // PoE2-style map modifiers: some make the map harder (monster life/damage/resistance,
    // player resistance debuff), some make it more rewarding (item rarity/quantity) --
    // applied when the Waystone is consumed to open a map, see ZoneBuilder/GameScene.
    static std::vector<WaystoneMod> WaystoneModPool() {
        return {
            { WaystoneModStat::MonsterIncreasedLife, 30.0f, "" },
            { WaystoneModStat::MonsterIncreasedDamage, 25.0f, "" },
            { WaystoneModStat::MonsterIncreasedElementalResistance, 20.0f, "" },
            { WaystoneModStat::PlayerReducedElementalResistance, -12.0f, "" },
            { WaystoneModStat::IncreasedItemRarity, 20.0f, "" },
            { WaystoneModStat::IncreasedItemQuantity, 20.0f, "" },
        };
    }
};
