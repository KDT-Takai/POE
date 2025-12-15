#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <typeindex>
#include <set>

#include "Entity.h"
// コンポーネントのインターフェース
struct IComponentPool {
    virtual ~IComponentPool() = default;
    virtual void OnEntityDestroyed(Entity entity) = 0;
};

// コンポーネントプール アクセス早くする
template <typename T>
class ComponentPool : public IComponentPool {
public:
    std::vector<T> data;
    // EntityID -> Vector Index のマップ (Sparse Setの簡易版)
    std::unordered_map<Entity, size_t> entityToIndex;
    std::unordered_map<size_t, Entity> indexToEntity;

    void Insert(Entity entity, T component) {
        if (entityToIndex.find(entity) != entityToIndex.end())
        {
            // 既に持っていれば上書き
            data[entityToIndex[entity]] = component;
            return;
        }
        size_t index = data.size();
        entityToIndex[entity] = index;
        indexToEntity[index] = entity;
        data.push_back(component);
    }

    void Remove(Entity entity) {
        if (entityToIndex.find(entity) == entityToIndex.end())
        {
            return;
        }

        // 削除時は最後の要素を持ってきて穴埋めする (Swap & Pop)
        // これによりメモリの連続性を保つ
        size_t removedIndex = entityToIndex[entity];
        size_t lastIndex = data.size() - 1;
        Entity lastEntity = indexToEntity[lastIndex];

        data[removedIndex] = data[lastIndex];

        entityToIndex[lastEntity] = removedIndex;
        indexToEntity[removedIndex] = lastEntity;

        entityToIndex.erase(entity);
        indexToEntity.erase(lastIndex);
        data.pop_back();
    }

    T& Get(Entity entity) {
        return data[entityToIndex[entity]];
    }

    bool Has(Entity entity) {
        return entityToIndex.find(entity) != entityToIndex.end();
    }

    void OnEntityDestroyed(Entity entity) override {
        if (Has(entity)) Remove(entity);
    }
};