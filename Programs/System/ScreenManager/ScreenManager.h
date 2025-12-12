#pragma once
#include <iostream>
#include <unordered_map>
#include <functional>
#include <string>
#include <memory>
#include "ScreenBase.h"
#include "../Singleton/Singleton.h"

using ScreenFactory = std::function<std::unique_ptr<ScreenBase>()>;

class ScreenManager : public Singleton<ScreenManager> {
    friend class Singleton<ScreenManager>;
protected:
    ScreenManager();

    std::unique_ptr<ScreenBase> screen;
    std::unique_ptr<ScreenBase> next;
    bool changeFlag = false;

    // 現在のシーン名
    std::string currentScreenName;
    // シーンファクトリ
    std::unordered_map<std::string, ScreenFactory> registeredScreens;
    // シーン登録
    void RegisterScreen(const std::string& name, ScreenFactory factory);

public:
    void Update();
    void Render(sf::RenderTarget& target);
    void RenderImGui(const sf::Texture* renderTexture);

    void ChangeScreen(const std::string& screenName);

    // 現在のシーン
    const ScreenBase* GetCurrentScreen() const { return screen.get(); }

    // 現在のシーン名を取得
    const std::string& GetCurrentScreenName() const { return currentScreenName; }

    // 登録されている全シーン取得
    const std::unordered_map<std::string, ScreenFactory>& GetRegisteredScreens() const {
        return registeredScreens;
    }

    // シーンを登録する外部インターフェース
    template <typename T>
    void RegisterScreen() {
        RegisterScreen(T::GetName(), []() { return std::make_unique<T>(); });
    }
};
