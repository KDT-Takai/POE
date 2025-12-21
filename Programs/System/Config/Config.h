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

#include <Windows.h>
#include <imgui.h>
namespace Gui {
    inline void Text(const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        // Shift-JIS -> UTF-16 (Unicode)
        int size_needed = MultiByteToWideChar(CP_ACP, 0, buffer, -1, NULL, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_ACP, 0, buffer, -1, &wstr[0], size_needed);

        // UTF-16 -> UTF-8 (ImGui用)
        size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
        std::string strUtf8(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &strUtf8[0], size_needed, NULL, NULL);

        // ImGuiに渡す
        ImGui::TextUnformatted(strUtf8.c_str());
    }
}