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

    void SetWindow(sf::RenderWindow* window) { m_window = window; }
	sf::RenderWindow* GetWindow() const { return m_window; }
    sf::Vector2f GetMouseWorldPosition() const;

    // Mouse wheel has no polled state in SFML (unlike keys/buttons), so Application::
    // ProcessEvents accumulates this frame's MouseWheelScrolled events here and resets
    // it before polling the next frame's events; game systems just read the total.
    void ResetMouseWheelDelta() { m_mouseWheelDelta = 0.0f; }
    void AddMouseWheelDelta(float delta) { m_mouseWheelDelta += delta; }
    float GetMouseWheelDelta() const { return m_mouseWheelDelta; }
private:
    std::unique_ptr<KeyInput> keyInput;
    std::unique_ptr<MouseInput> mouseInput;
    std::unique_ptr<PadInput> padInput;

    sf::RenderWindow* m_window = nullptr;
    float m_mouseWheelDelta = 0.0f;
};
