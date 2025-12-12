// Config.h
#pragma once
#include <string>
#include "../Input/InputManager.h"

// ---------------------------------
// Window settings ウィンドウの設定
// ---------------------------------
constexpr unsigned int WINDOW_WIDTH = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;
constexpr unsigned int FRAMERATE_LIMIT = 120;
const std::string WINDOW_TITLE = "SFML Application";

// いつかInputAlias.hに移動
namespace Input {
    inline KeyInput& Key() { return InputManager::Instance().GetKeyInput(); }
    inline MouseInput& Mouse() { return InputManager::Instance().GetMouseInput(); }
}