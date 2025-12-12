#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <type_traits>

// リソース管理用のクラス
template <typename T>
// リーソースキャッシュクラス
class ResourceCache {
private:
    std::unordered_map<std::string, std::shared_ptr<T>> resources;
public:
    // 先読み込み用メソッド
    bool load(const std::string& filename) {
        if (resources.find(filename) != resources.end())
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

        resources[filename] = resource;
        spdlog::info("ResourceCache: Pre-loaded '{}'", filename);
        return true;
    }
    // リソースを取得
    std::shared_ptr<T> get(const std::string& filename) {
        if (!load(filename))
        {
            return nullptr;
        }
        return resources[filename];
    }

    // 特定リソース解放
    void unload(const std::string& filename) {
        auto it = resources.find(filename);
        if (it != resources.end())
        {
            resources.erase(it);
            spdlog::info("ResourceCache: Unloaded '{}'", filename);
        }
    }

    // 全解放
    void unloadAll() {
        resources.clear();
        spdlog::info("ResourceCache: All resources cleared.");
    }
};