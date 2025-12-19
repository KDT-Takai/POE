#pragma once
#include "../Language/Language.h"
#include <imgui.h>
#include <string>
#include <cstdarg>
#include <cstdio>

// ImGui専用のラッパー
class DebugGui {
public:
    // 文字列表示
    static void Text(const char* en, const char* jp, ...) {
        // Languageクラスからどっちを表示するか聞く
        const char* fmt = Language::Str(en, jp);

        va_list args;
        va_start(args, jp);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        ImGui::TextUnformatted(buffer);
    }

    // ウィンドウ
    static bool Begin(const char* en, const char* jp, bool* p_open = nullptr) {
        return ImGui::Begin(Language::Str(en, jp), p_open);
    }

    static void End() { ImGui::End(); }

    // ボタン
    static bool Button(const char* en, const char* jp) {
        return ImGui::Button(Language::Str(en, jp));
    }

    // 設定変更用のチェックボックスなど
    static void ShowLanguageSelector() {
        if (ImGui::RadioButton("English", Language::Get() == Language::Type::English)) {
            Language::Set(Language::Type::English);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton((const char*)u8"日本語", Language::Get() == Language::Type::Japanese)) {
            Language::Set(Language::Type::Japanese);
        }
    }
};