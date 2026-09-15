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
#include "../UI/ItemUIHelpers.h"
#include <vector>
#include <string>
#include <random>
#include <algorithm>

// Ground items (Item/Currency/SkillGem pickups) are collected by clicking them, not
// by walking near them. FindNearestPickup resolves what the cursor is over; GameScene
// calls it once per frame to: (1) decide whether to suppress the left-click-casts-
// Skill1 binding while hovering a pickup, (2) draw a name tooltip near the cursor,
// and (3) tell Update() which entity (if any) a fresh click should collect.
class ItemPickupSystem {
public:
    static constexpr Entity kInvalidEntity = static_cast<Entity>(-1);
    static constexpr float kClickRadius = 28.0f;

    std::string lastMessage;
    float messageTimer = 0.0f;

    // Closest ground pickup within radius of worldPos, or kInvalidEntity if none.
    static Entity FindNearestPickup(Registry& registry, sf::Vector2f worldPos, float radius) {
        Entity best = kInvalidEntity;
        float bestDistSq = radius * radius;

        auto consider = [&](Entity e) {
            const auto& trans = registry.GetComponent<TransformComponent>(e);
            float dx = trans.position.x - worldPos.x;
            float dy = trans.position.y - worldPos.y;
            float distSq = dx * dx + dy * dy;
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                best = e;
            }
        };

        for (auto e : registry.View<ItemPickupComponent, TransformComponent>()) consider(e);
        for (auto e : registry.View<CurrencyPickupComponent, TransformComponent>()) consider(e);
        for (auto e : registry.View<SkillGemPickupComponent, TransformComponent>()) consider(e);
        return best;
    }

    // Display name + color for a hover tooltip. Returns false if e isn't a pickup
    // (already collected this frame, or an invalid/unrelated entity).
    static bool GetPickupDisplay(Registry& registry, Entity e, std::string& outName, sf::Color& outColor) {
        if (!registry.IsValid(e)) return false;
        if (registry.HasComponent<ItemPickupComponent>(e)) {
            const auto& item = registry.GetComponent<ItemPickupComponent>(e).item;
            outName = item.baseName;
            outColor = ItemUIHelpers::RarityColor(item.rarity);
            return true;
        }
        if (registry.HasComponent<CurrencyPickupComponent>(e)) {
            outName = CurrencySystem::CurrencyLabel(registry.GetComponent<CurrencyPickupComponent>(e).type);
            outColor = sf::Color(255, 160, 220);
            return true;
        }
        if (registry.HasComponent<SkillGemPickupComponent>(e)) {
            int gemId = registry.GetComponent<SkillGemPickupComponent>(e).gemId;
            const GemDefinition* def = SkillGemData::Find(gemId);
            outName = def ? def->skill.name : "Unknown Gem";
            outColor = sf::Color(255, 90, 220);
            return true;
        }
        return false;
    }

    // clickedPickup: the entity a fresh left-click landed on this frame (kInvalidEntity
    // if the click didn't hit anything, or nothing was clicked at all).
    void Update(Registry& registry, float dt, Entity clickedPickup) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!registry.IsValid(clickedPickup)) return;

        Entity player = kInvalidEntity;
        for (auto e : registry.View<PlayerInputComponent, EquipmentComponent, InventoryComponent, CharacterStatsComponent>()) {
            player = e;
            break;
        }
        if (player == kInvalidEntity) return;

        auto& equipment = registry.GetComponent<EquipmentComponent>(player);
        auto& inventory = registry.GetComponent<InventoryComponent>(player);
        auto& stats = registry.GetComponent<CharacterStatsComponent>(player);

        if (registry.HasComponent<ItemPickupComponent>(clickedPickup)) {
            auto& pickup = registry.GetComponent<ItemPickupComponent>(clickedPickup);
            if (inventory.items.size() < InventoryComponent::kCapacity) {
                inventory.items.push_back(pickup.item);
                lastMessage = "Picked up: " + pickup.item.baseName;
            } else {
                int goldValue = ItemFactory::SellValue(pickup.item);
                stats.gold += goldValue;
                lastMessage = "Inventory full - sold " + pickup.item.baseName + " for " + std::to_string(goldValue) + " gold";
            }
            messageTimer = 2.5f;
            registry.DestroyEntity(clickedPickup);
        } else if (registry.HasComponent<CurrencyPickupComponent>(clickedPickup)) {
            auto& currency = registry.GetComponent<CurrencyPickupComponent>(clickedPickup);
            static std::random_device rd;
            static std::mt19937 rng(rd());
            lastMessage = CurrencySystem::ApplyCurrency(equipment, stats, currency.type, stats.level, rng);
            messageTimer = 2.5f;
            registry.DestroyEntity(clickedPickup);
        } else if (registry.HasComponent<SkillGemPickupComponent>(clickedPickup)) {
            auto& gemPickup = registry.GetComponent<SkillGemPickupComponent>(clickedPickup);
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
            registry.DestroyEntity(clickedPickup);
        }
    }
};
