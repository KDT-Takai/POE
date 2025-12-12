#pragma once
#include "Registry.h"
#include <spdlog/spdlog.h>

class EntityObject {
private:
    Entity m_entityHandle;
    Registry* m_registry;

public:
    // コンストラクタ
    EntityObject() : m_entityHandle(-1), m_registry(nullptr) {}
    EntityObject(Entity handle, Registry* registry)
        : m_entityHandle(handle), m_registry(registry) {
    }

    // IDを取得
    Entity GetID() const { return m_entityHandle; }

    // 有効か確認
    bool IsValid() const { return m_registry != nullptr && m_entityHandle != -1; }

    // コンポーネント操作
    template<typename T>
    T& AddComponent(T component) {
        // moveを使うことで無駄なコピーを避ける
        m_registry->AddComponent<T>(m_entityHandle, std::move(component));
        return m_registry->GetComponent<T>(m_entityHandle);
    }

    template<typename T>
    T& GetComponent() {
        // コンポーネントを持っていない場合のspdlogエラー表示
        if (!HasComponent<T>()) {
            spdlog::error("Entity {} does not have component '{}'", m_entityHandle, typeid(T).name());
            throw std::runtime_error("Component not found");    // ついでにthrowで落とす
        }
        return m_registry->GetComponent<T>(m_entityHandle);
    }

    template<typename T>
    bool HasComponent() {
        return m_registry->HasComponent<T>(m_entityHandle);
    }

    void Destroy() {
        if (IsValid()) {
            m_registry->DestroyEntity(m_entityHandle);
        }
    }

    explicit operator bool() const { return IsValid(); }
};