#pragma once
#include <vector>
#include <cstdint>
#include <limits>
#include <utility>

#include "Entity.h"

struct IComponentPool {
    virtual ~IComponentPool() = default;
    virtual void OnEntityDestroyed(Entity entity) = 0;
};

// Sparse set: sparse[entity] -> dense index, O(1) insert/remove/get, no hashing
template <typename T>
class ComponentPool : public IComponentPool {
    static constexpr size_t kInvalid = (std::numeric_limits<size_t>::max)();

    std::vector<size_t> sparse;
    std::vector<Entity> denseEntities;
    std::vector<T> data;

    void EnsureSparse(Entity entity) {
        if (entity >= sparse.size()) sparse.resize(static_cast<size_t>(entity) + 1, kInvalid);
    }

public:
    T& Insert(Entity entity, T component) {
        EnsureSparse(entity);
        size_t idx = sparse[entity];
        if (idx != kInvalid) {
            data[idx] = std::move(component);
            return data[idx];
        }
        idx = data.size();
        sparse[entity] = idx;
        denseEntities.push_back(entity);
        data.push_back(std::move(component));
        return data.back();
    }

    void Remove(Entity entity) {
        if (entity >= sparse.size() || sparse[entity] == kInvalid) return;

        size_t removedIndex = sparse[entity];
        size_t lastIndex = data.size() - 1;
        Entity lastEntity = denseEntities[lastIndex];

        data[removedIndex] = std::move(data[lastIndex]);
        denseEntities[removedIndex] = lastEntity;
        sparse[lastEntity] = removedIndex;

        data.pop_back();
        denseEntities.pop_back();
        sparse[entity] = kInvalid;
    }

    T& Get(Entity entity) { return data[sparse[entity]]; }

    bool Has(Entity entity) const {
        return entity < sparse.size() && sparse[entity] != kInvalid;
    }

    const std::vector<Entity>& Entities() const { return denseEntities; }
    std::vector<T>& Data() { return data; }
    size_t Size() const { return data.size(); }

    void OnEntityDestroyed(Entity entity) override { Remove(entity); }
};
