#pragma once
#include "../../Registry/Registry.h"
#include "Components/Item/ItemPickup.h"
#include "Components/Item/Equipment.h"
#include "Components/Item/Currency.h"
#include "Components/Physics/Transform/Transform.h"
#include "Components/Control/PlayerInput/PlayerInput.h"
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "EquipmentSystem.h"
#include "CurrencySystem.h"
#include <vector>
#include <string>
#include <random>

class ItemPickupSystem {
public:
    std::string lastMessage;
    float messageTimer = 0.0f;

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;

        Entity player = 0;
        bool found = false;
        for (auto e : registry.View<PlayerInputComponent, TransformComponent, EquipmentComponent, CharacterStatsComponent>()) {
            player = e;
            found = true;
            break;
        }
        if (!found) return;

        auto& playerTrans = registry.GetComponent<TransformComponent>(player);
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        std::vector<Entity> toRemove;
        for (auto e : registry.View<ItemPickupComponent, TransformComponent>()) {
            auto& pickupTrans = registry.GetComponent<TransformComponent>(e);
            float dx = pickupTrans.position.x - playerTrans.position.x;
            float dy = pickupTrans.position.y - playerTrans.position.y;
            if (dx * dx + dy * dy < 40.0f * 40.0f) {
                auto& pickup = registry.GetComponent<ItemPickupComponent>(e);
                bool equipped = EquipmentSystem::TryEquip(stats, equipment, pickup.item);
                if (equipped) {
                    lastMessage = "Equipped: " + pickup.item.baseName;
                } else {
                    int goldValue = SellValue(pickup.item);
                    stats.gold += goldValue;
                    lastMessage = "Sold " + pickup.item.baseName + " for " + std::to_string(goldValue) + " gold";
                }
                messageTimer = 2.5f;
                toRemove.push_back(e);
            }
        }
        for (auto e : registry.View<CurrencyPickupComponent, TransformComponent>()) {
            auto& pickupTrans = registry.GetComponent<TransformComponent>(e);
            float dx = pickupTrans.position.x - playerTrans.position.x;
            float dy = pickupTrans.position.y - playerTrans.position.y;
            if (dx * dx + dy * dy < 40.0f * 40.0f) {
                auto& currency = registry.GetComponent<CurrencyPickupComponent>(e);
                static std::random_device rd;
                static std::mt19937 rng(rd());
                lastMessage = CurrencySystem::ApplyCurrency(equipment, stats, currency.type, stats.level, rng);
                messageTimer = 2.5f;
                toRemove.push_back(e);
            }
        }

        for (auto e : toRemove) {
            if (registry.IsValid(e)) registry.DestroyEntity(e);
        }
    }

private:
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
};
