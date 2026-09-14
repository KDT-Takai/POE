#pragma once
#include <memory>
#include <algorithm>
#include <vector>
#include "Core/Entity.h"
#include "../Core/ComponentPool.h"

class EntityObject;

class ComponentTypeId {
    inline static size_t s_counter = 0;
public:
    template <typename T>
    static size_t Get() {
        static const size_t id = s_counter++;
        return id;
    }
};

class Registry {
private:
    std::vector<uint8_t> m_alive;
    std::vector<Entity> m_freeList;
    Entity m_nextEntityId = 0;

    std::vector<std::unique_ptr<IComponentPool>> m_pools;

    template <typename T>
    ComponentPool<T>& GetPool() {
        size_t id = ComponentTypeId::Get<T>();
        if (id >= m_pools.size()) m_pools.resize(id + 1);
        if (!m_pools[id]) m_pools[id] = std::make_unique<ComponentPool<T>>();
        return static_cast<ComponentPool<T>&>(*m_pools[id]);
    }

public:
    Entity CreateEntityID() {
        Entity id;
        if (!m_freeList.empty()) {
            id = m_freeList.back();
            m_freeList.pop_back();
        } else {
            id = m_nextEntityId++;
            if (id >= m_alive.size()) m_alive.resize(static_cast<size_t>(id) + 1, 0);
        }
        m_alive[id] = 1;
        return id;
    }

    Entity CreateEntity() { return CreateEntityID(); }

    EntityObject CreateEntityObject();

    bool IsValid(Entity entity) const {
        return entity < m_alive.size() && m_alive[entity];
    }

    void DestroyEntity(Entity entity) {
        if (!IsValid(entity)) return;
        for (auto& pool : m_pools) {
            if (pool) pool->OnEntityDestroyed(entity);
        }
        m_alive[entity] = 0;
        m_freeList.push_back(entity);
    }

    template <typename T>
    T& AddComponent(Entity entity, T component) {
        return GetPool<T>().Insert(entity, std::move(component));
    }

    template <typename T>
    void RemoveComponent(Entity entity) {
        GetPool<T>().Remove(entity);
    }

    template <typename T>
    T& GetComponent(Entity entity) {
        return GetPool<T>().Get(entity);
    }

    template <typename T>
    bool HasComponent(Entity entity) {
        return GetPool<T>().Has(entity);
    }

    std::vector<Entity> GetEntities() const {
        std::vector<Entity> result;
        result.reserve(m_alive.size());
        for (Entity e = 0; e < m_alive.size(); ++e) {
            if (m_alive[e]) result.push_back(e);
        }
        return result;
    }

    template<typename First, typename... Rest>
    std::vector<Entity> View() {
        auto& firstPool = GetPool<First>();
        std::vector<Entity> entities;
        entities.reserve(firstPool.Size());
        for (Entity entity : firstPool.Entities()) {
            if ((HasComponent<Rest>(entity) && ...)) {
                entities.push_back(entity);
            }
        }
        return entities;
    }
};