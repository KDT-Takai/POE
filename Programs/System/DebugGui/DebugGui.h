#pragma once
#include "../Language//Language.h"
#include <imgui.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
// 文字コード用やで
#include <Windows.h>

class DebugGui {
private:
    // Shift-JISをUTF-8に変換する関数
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
    // テキスト
    static void Text(const char* en, const char* jp, ...) {
        // 言語選択
        bool isJp = (Language::Get() == Language::Type::Japanese);
        const char* fmt = isJp ? jp : en;
        // フォーマット処理 (printf)
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
//            return ImGui::Begin(SjisToUtf8(title).c_str(), p_open, flags);
            std::string title = SjisToUtf8(jp) + "###" + en;
            return ImGui::Begin(title.c_str(), p_open, flags);
        }
        else {
            return ImGui::Begin(title, p_open, flags);
        }
    }
    // 終了
    static void End() {
        ImGui::End();
    }
    // ボタン
    static bool RadioButton(const char* label, bool active) {
        return ImGui::RadioButton(SjisToUtf8(label).c_str(), active);
    }
    // ボタン
    static bool Button(const char* en, const char* jp, const ImVec2& size = ImVec2(0, 0)) {
        bool isJp = (Language::Get() == Language::Type::Japanese);

        if (isJp) {
            // 表示は日本語、内部IDは英語 ("敵出現###Spawn Enemy")
            // これにより言語を切り替えてもボタンのIDが変わらず、動作が安定します
            std::string text = SjisToUtf8(jp) + "###" + en;
            return ImGui::Button(text.c_str(), size);
        }
        else {
            return ImGui::Button(en, size);
        }
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
    // スライダー (Float)
    static bool SliderFloat(const char* en, const char* jp, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
        bool isJp = (Language::Get() == Language::Type::Japanese);
        if (isJp) {
            std::string label = SjisToUtf8(jp) + "###" + en;
            return ImGui::SliderFloat(label.c_str(), v, v_min, v_max, format, flags);
        }
        else {
            return ImGui::SliderFloat(en, v, v_min, v_max, format, flags);
        }
    }
    // セパレーター付きテキスト
    static void SeparatorText(const char* en, const char* jp) {
        bool isJp = (Language::Get() == Language::Type::Japanese);

        if (isJp) {
            ImGui::SeparatorText(SjisToUtf8(jp).c_str());
        }
        else {
            ImGui::SeparatorText(en);
        }
    }
    // ツリーノード
    static bool TreeNode(const char* en, const char* jp) {
        bool isJp = (Language::Get() == Language::Type::Japanese);

        if (isJp) {
            std::string text = SjisToUtf8(jp) + "###" + en;
            return ImGui::TreeNode(text.c_str());
        }
        else {
            return ImGui::TreeNode(en);
        }
    }
    // ツリーノード終了 (中身は ImGui::TreePop と同じですが、名前空間を揃えるために用意)
    static void TreePop() {
        ImGui::TreePop();
    }
    // テキスト入力
    static bool InputText(const char* en, const char* jp, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0) {
        bool isJp = (Language::Get() == Language::Type::Japanese);
        if (isJp) {
            std::string text = SjisToUtf8(jp) + "###" + en;
            return ImGui::InputText(text.c_str(), buf, buf_size, flags);
        }
        else {
            return ImGui::InputText(en, buf, buf_size, flags);
        }
    }
    // 折りたたみヘッダー
    static bool CollapsingHeader(const char* en, const char* jp, ImGuiTreeNodeFlags flags = 0) {
        bool isJp = (Language::Get() == Language::Type::Japanese);

        if (isJp) {
            std::string text = SjisToUtf8(jp) + "###" + en;
            return ImGui::CollapsingHeader(text.c_str(), flags);
        }
        else {
            return ImGui::CollapsingHeader(en, flags);
        }
    }
    // ドラッグ数値入力
    static bool DragFloat(const char* en, const char* jp, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
        bool isJp = (Language::Get() == Language::Type::Japanese);

        if (isJp) {
            std::string text = SjisToUtf8(jp) + "###" + en;
            return ImGui::DragFloat(text.c_str(), v, v_speed, v_min, v_max, format, flags);
        }
        else {
            return ImGui::DragFloat(en, v, v_speed, v_min, v_max, format, flags);
        }
    }
    // ドラッグ数値入力
    static bool DragFloat2(const char* en, const char* jp, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
        bool isJp = (Language::Get() == Language::Type::Japanese);

        if (isJp) {
            std::string text = SjisToUtf8(jp) + "###" + en;
            return ImGui::DragFloat2(text.c_str(), v, v_speed, v_min, v_max, format, flags);
        }
        else {
            return ImGui::DragFloat2(en, v, v_speed, v_min, v_max, format, flags);
        }
    }
    // カラー編集
    static bool ColorEdit4(const char* en, const char* jp, float* col, ImGuiColorEditFlags flags = 0) {
        bool isJp = (Language::Get() == Language::Type::Japanese);

        if (isJp) {
            std::string text = SjisToUtf8(jp) + "###" + en;
            return ImGui::ColorEdit4(text.c_str(), col, flags);
        }
        else {
            return ImGui::ColorEdit4(en, col, flags);
        }
    }
    static bool ColorEdit3(const char* en, const char* jp, float* col, ImGuiColorEditFlags flags = 0) {
        bool isJp = (Language::Get() == Language::Type::Japanese);

        if (isJp) {
            std::string text = SjisToUtf8(jp) + "###" + en;
            return ImGui::ColorEdit3(text.c_str(), col, flags);
        }
        else {
            return ImGui::ColorEdit3(en, col, flags);
        }
    }
};