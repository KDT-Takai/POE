#pragma once
#include "../../Registry/Registry.h"
#include "Components/Item/ItemPickup.h"
#include "Components/Item/Equipment.h"
#include "Components/Item/Inventory.h"
#include "Components/Item/Currency.h"
#include "Components/Item/SkillGem.h"
#include "Components/Physics/Transform/Transform.h"
#include "Components/Control/PlayerInput/PlayerInput.h"
#include "Components/Stats/CharacterStats/CharacterStats.h"
#include "EquipmentSystem.h"
#include "CurrencySystem.h"
#include "ItemFactory.h"
#include "../Skill/SkillGemData.h"
#include <vector>
#include <string>
#include <random>
#include <algorithm>

class ItemPickupSystem {
public:
    std::string lastMessage;
    float messageTimer = 0.0f;

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;

        Entity player = 0;
        bool found = false;
        for (auto e : registry.View<PlayerInputComponent, TransformComponent, EquipmentComponent, InventoryComponent, CharacterStatsComponent>()) {
            player = e;
            found = true;
            break;
        }
        if (!found) return;

        auto& playerTrans = registry.GetComponent<TransformComponent>(player);
        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        std::vector<Entity> toRemove;
        for (auto e : registry.View<ItemPickupComponent, TransformComponent>()) {
            auto& pickupTrans = registry.GetComponent<TransformComponent>(e);
            float dx = pickupTrans.position.x - playerTrans.position.x;
            float dy = pickupTrans.position.y - playerTrans.position.y;
            if (dx * dx + dy * dy < 40.0f * 40.0f) {
                auto& pickup = registry.GetComponent<ItemPickupComponent>(e);
                if (inventory.items.size() < InventoryComponent::kCapacity) {
                    inventory.items.push_back(pickup.item);
                    lastMessage = "Picked up: " + pickup.item.baseName;
                } else {
                    int goldValue = ItemFactory::SellValue(pickup.item);
                    stats.gold += goldValue;
                    lastMessage = "Inventory full - sold " + pickup.item.baseName + " for " + std::to_string(goldValue) + " gold";
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

        for (auto e : registry.View<SkillGemPickupComponent, TransformComponent>()) {
            auto& pickupTrans = registry.GetComponent<TransformComponent>(e);
            float dx = pickupTrans.position.x - playerTrans.position.x;
            float dy = pickupTrans.position.y - playerTrans.position.y;
            if (dx * dx + dy * dy < 40.0f * 40.0f) {
                auto& gemPickup = registry.GetComponent<SkillGemPickupComponent>(e);
                const GemDefinition* def = SkillGemData::Find(gemPickup.gemId);
                std::string gemName = def ? def->skill.name : "Unknown Gem";

                if (registry.HasComponent<SkillGemInventoryComponent>(player)) {
                    auto& gemInventory = registry.GetComponent<SkillGemInventoryComponent>(player);
                    auto& ids = gemInventory.unlockedGemIds;
                    if (std::find(ids.begin(), ids.end(), gemPickup.gemId) == ids.end()) {
                        ids.push_back(gemPickup.gemId);
                        lastMessage = "Learned skill gem: " + gemName + " (press K to equip)";
                    } else {
                        lastMessage = "Already known: " + gemName;
                    }
                    messageTimer = 2.5f;
                }
                toRemove.push_back(e);
            }
        }

        for (auto e : toRemove) {
            if (registry.IsValid(e)) registry.DestroyEntity(e);
        }
    }
};
