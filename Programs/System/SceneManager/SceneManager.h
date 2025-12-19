#pragma once
#include <iostream>
#include <unordered_map>
#include <functional>
#include <string>
#include <memory>
#include "SceneBase.h"
#include "../Singleton/Singleton.h"

using SceneFactory = std::function<std::unique_ptr<SceneBase>()>;

class SceneManager : public Singleton<SceneManager> {
    friend class Singleton<SceneManager>;
protected:
    SceneManager();

    std::unique_ptr<SceneBase> Scene;
    std::unique_ptr<SceneBase> next;
    bool changeFlag = false;

    // 現在のシーン名
    std::string currentSceneName;
    // シーンファクトリ
    std::unordered_map<std::string, SceneFactory> registeredScenes;
    // シーン登録
    void RegisterScene(const std::string& name, SceneFactory factory);

public:
    void Update();
    void Render(sf::RenderTarget& target);
    void RenderImGui(const sf::Texture* renderTexture);

    void ChangeScene(const std::string& SceneName);

    // 現在のシーン
    const SceneBase* GetCurrentScene() const { return Scene.get(); }

    // 現在のシーン名を取得
    const std::string& GetCurrentSceneName() const { return currentSceneName; }

    // 登録されている全シーン取得
    const std::unordered_map<std::string, SceneFactory>& GetRegisteredScenes() const {
        return registeredScenes;
    }

    // シーンを登録する外部インターフェース
    template <typename T>
    void RegisterScene() {
        RegisterScene(T::GetName(), []() { return std::make_unique<T>(); });
    }
};
