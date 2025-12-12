#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <array>
#include "../Singleton/Singleton.h"
#include "InputUtils/InputUtils.h" // 入力ユーティリティ
#include "KeyInput/KeyInput.h"   // キーボード
#include "MouseInput/MouseInput.h" // マウス
#include "PadInput/PadInput.h"   // ゲームパッド

class InputManager : public Singleton<InputManager> {
    friend class Singleton<InputManager>;
protected:
    InputManager();
public:
    
    void Update(const sf::RenderWindow& window);
    void RenderImGui();

    KeyInput& GetKeyInput() { return *keyInput; }
    MouseInput& GetMouseInput() { return *mouseInput; }
    PadInput& GetPadInput() { return *padInput; }

private:
    std::unique_ptr<KeyInput> keyInput;
    std::unique_ptr<MouseInput> mouseInput;
    std::unique_ptr<PadInput> padInput;
};
