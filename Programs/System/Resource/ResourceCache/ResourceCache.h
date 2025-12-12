#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <type_traits> // 型名取得用

// リソース管理用のクラス
template <typename T>
class ResourceCache {
private:
    std::unordered_map<std::string, std::shared_ptr<T>> m_resources;
public:
    // 先読み込み用メソッド
    bool load(const std::string& filename) {
        if (m_resources.find(filename) != m_resources.end())
        {
            return true;
        }
        auto resource = std::make_shared<T>();
        bool success = false;
        if constexpr (std::is_same_v<T, sf::Music> || std::is_same_v<T, sf::Font>) {
            success = resource->openFromFile(filename);
        }
        else {
            success = resource->loadFromFile(filename);
        }
        if (!success)
        {
            spdlog::error("ResourceCache: Failed to load '{}'", filename);
            return false;
        }

        m_resources[filename] = resource;
        spdlog::info("ResourceCache: Pre-loaded '{}'", filename);
        return true;
    }
    // リソースを取得
    std::shared_ptr<T> get(const std::string& filename) {
        if (!load(filename))
        {
            return nullptr;
        }
        return m_resources[filename];
    }

    // 特定リソース解放
    void unload(const std::string& filename) {
        auto it = m_resources.find(filename);
        if (it != m_resources.end())
        {
            m_resources.erase(it);
            spdlog::info("ResourceCache: Unloaded '{}'", filename);
        }
    }

    // 全解放
    void unloadAll() {
        m_resources.clear();
        spdlog::info("ResourceCache: All resources cleared.");
    }
};