#pragma once
#include <memory>
#include <algorithm>
#include <typeindex>
#include <set>
#include <unordered_map>
#include "ECS.h" // äÓñ{å^Çì«Ç›çûÇﬁ

class EntityObject;

class Registry {
private:
    Entity m_nextEntityId = 0;
    std::set<Entity> m_entities;
    std::unordered_map<std::type_index, std::shared_ptr<IComponentPool>> m_componentPools;

    template <typename T>
    std::shared_ptr<ComponentPool<T>> GetPool() {
        std::type_index typeIndex(typeid(T));
        if (m_componentPools.find(typeIndex) == m_componentPools.end()) {
            m_componentPools[typeIndex] = std::make_shared<ComponentPool<T>>();
        }
        return std::static_pointer_cast<ComponentPool<T>>(m_componentPools[typeIndex]);
    }

public:
    Entity CreateEntityID() {
        Entity id = m_nextEntityId++;
        m_entities.insert(id);
        return id;
    }

    Entity CreateEntity() { return CreateEntityID(); }

    EntityObject CreateEntityObject();

    bool IsValid(Entity entity) const {
        return m_entities.find(entity) != m_entities.end();
    }

    void DestroyEntity(Entity entity) {
        for (auto& pair : m_componentPools) {
            pair.second->OnEntityDestroyed(entity);
        }
        m_entities.erase(entity);
    }

    template <typename T>
    void AddComponent(Entity entity, T component) {
        GetPool<T>()->Insert(entity, component);
    }

    template <typename T>
    void RemoveComponent(Entity entity) {
        GetPool<T>()->Remove(entity);
    }

    template <typename T>
    T& GetComponent(Entity entity) {
        return GetPool<T>()->Get(entity);
    }

    template <typename T>
    bool HasComponent(Entity entity) {
        return GetPool<T>()->Has(entity);
    }

    const std::set<Entity>& GetEntities() const { return m_entities; }

    template<typename T>
    std::vector<Entity> View() {
        std::vector<Entity> entities;
        auto pool = GetPool<T>();
        for (auto const& [index, entity] : pool->indexToEntity) {
            entities.push_back(entity);
        }
        return entities;
    }
};