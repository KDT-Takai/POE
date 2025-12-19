#pragma once
#include "../Language//Language.h"
#include <imgui.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#include <Windows.h> // 文字コード変換に必要

class DebugGui {
private:
    // --- 内部用: Shift-JIS を UTF-8 に変換する関数 ---
    static std::string SjisToUtf8(const std::string& sjis) {
        if (sjis.empty()) return "";

        // 1. Shift-JIS -> UTF-16 (ワイド文字)
        int size_needed = MultiByteToWideChar(CP_ACP, 0, sjis.c_str(), -1, NULL, 0);
        if (size_needed <= 0) return sjis; // 失敗したらそのまま返す
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_ACP, 0, sjis.c_str(), -1, &wstr[0], size_needed);

        // 2. UTF-16 -> UTF-8
        size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
        std::string utf8(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &utf8[0], size_needed, NULL, NULL);

        // 末尾のヌル文字が含まれることがあるので削除（std::stringの仕様による調整）
        if (!utf8.empty() && utf8.back() == '\0') {
            utf8.pop_back();
        }

        return utf8;
    }

public:
    // --- ラッパー関数群 ---

    static void Text(const char* en, const char* jp, ...) {
        // 1. 言語選択
        bool isJp = (Language::Get() == Language::Type::Japanese);
        const char* fmt = isJp ? jp : en;

        // 2. フォーマット処理 (printf)
        va_list args;
        va_start(args, jp);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        // 文字コード変換 (日本語のときだけ変換する)
        if (isJp) {
            // バッファの中身(S-JIS)をUTF-8に変換して表示
            //std::string utf8Str = SjisToUtf8(buffer);
            //ImGui::TextUnformatted(utf8Str.c_str());
            ImGui::TextUnformatted(SjisToUtf8(buffer).c_str());
        }
        else {
            // 英語ならそのまま表示 (ASCIIはUTF-8互換なので変換不要)
            ImGui::TextUnformatted(buffer);
        }
    }

    // ウィンドウ (タイトルも変換が必要)
    static bool Begin(const char* en, const char* jp, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) {
        bool isJp = (Language::Get() == Language::Type::Japanese);
        const char* title = isJp ? jp : en;

        if (isJp) {
            // 日本語なら変換して渡す
            // ※ウィンドウIDが変わるのを防ぐため、本来は "Title###ID" のID部分は固定するのがベストだが
            // ここでは簡易的に変換だけ行う
            return ImGui::Begin(SjisToUtf8(title).c_str(), p_open, flags);
        }
        else {
            return ImGui::Begin(title, p_open, flags);
        }
    }

    static void End() {
        ImGui::End();
    }
    // ボタン
    static bool RadioButton(const char* label, bool active) {
        return ImGui::RadioButton(SjisToUtf8(label).c_str(), active);
    }
    static bool RadioButton(const char* en, const char* jp, bool active) {
        bool isJp = (Language::Get() == Language::Type::Japanese);
        const char* label = isJp ? jp : en;

        if (isJp) {
            return ImGui::RadioButton(SjisToUtf8(label).c_str(), active);
        }
        else {
            return ImGui::RadioButton(label, active);
        }
    }

    // ラジオボタン
    static bool RadioButton(const char* en, const char* jp, int* v, int v_button) {
        bool isJp = (Language::Get() == Language::Type::Japanese);
        const char* label = isJp ? jp : en;

        if (isJp) {
            return ImGui::RadioButton(SjisToUtf8(label).c_str(), v, v_button);
        }
        else {
            return ImGui::RadioButton(label, v, v_button);
        }
    }

    // チェックボックス
    static bool Checkbox(const char* en, const char* jp, bool* v) {
        bool isJp = (Language::Get() == Language::Type::Japanese);
        const char* label = isJp ? jp : en;

        if (isJp) {
            return ImGui::Checkbox(SjisToUtf8(label).c_str(), v);
        }
        else {
            return ImGui::Checkbox(label, v);
        }
    }
};