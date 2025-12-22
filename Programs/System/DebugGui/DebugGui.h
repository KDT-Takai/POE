#pragma once
#include "../Language//Language.h"
#include <imgui.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
// 文字コード用やで
#include <Windows.h>

//#define TEST 2

class DebugGui {
private:
    // CP_ACP
    static constexpr UINT CP_SJIS = 932;
    // Shift-JISをUTF-8に変換する関数
    static std::string SjisToUtf8(const std::string& sjis) {
        if (sjis.empty()) return "";
        // Shift-JISからUTF-16
        int size_needed = MultiByteToWideChar(CP_SJIS, 0, sjis.data(), static_cast<int>(sjis.size()), NULL, 0);
        if (size_needed <= 0) return sjis; // 失敗したらそのまま返す
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_SJIS, 0, sjis.data(), static_cast<int>(sjis.size()), &wstr[0], size_needed);

        // UTF-16からUTF-8
        size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
        std::string utf8(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &utf8[0], size_needed, NULL, NULL);
        // 末尾の削除
        if (!utf8.empty() && utf8.back() == '\0') {
            utf8.pop_back();
        }
        return utf8;
    }
    // UTF-8をSJISに変換
    static std::string Utf8ToSjis(const std::string& utf8) {
        if (utf8.empty()) return "";
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), NULL, 0);
        if (size_needed <= 0) return utf8;
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), &wstr[0], size_needed);

        size_needed = WideCharToMultiByte(CP_SJIS, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string sjis(size_needed, 0);
        WideCharToMultiByte(CP_SJIS, 0, wstr.data(), (int)wstr.size(), &sjis[0], size_needed, NULL, NULL);
        if (!sjis.empty() && sjis.back() == '\0') sjis.pop_back();
        return sjis;
    }
    // 日本語設定なら "日本語表示###EnglishID" を返す
    // 英語設定なら "EnglishID" を返す
    //static std::string GetLabelStr(const char* en, const char* jp) {
    //    if (Language::Get() == Language::Type::Japanese) {
    //        return SjisToUtf8(jp) + "###" + en;
    //    }
    //    return en;
    //}
    static std::string GetLabelStr(const char* en, const char* jp) {
        if (Language::Get() == Language::Type::Japanese) {
            // 日本語表示###EnglishID
            return SjisToUtf8(jp) + "###" + en;
        }
        // 英語表示###EnglishID (見た目は "English" だが IDは "English" と明示される)
        // ※通常は en だけでも同じIDになりますが、念には念を入れるならこう書く
        return std::string(en) + "###" + en;
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


#if TEST == 1 // わけがわからい
    // ウィンドウ (タイトルも変換が必要)
    static bool Begin(const char* id, const char* jp_label, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
    {
        // 表示名 + ### + ID
        return ImGui::Begin(GetLabelStr(id, jp_label).c_str(), p_open, flags);
    }
#elif TEST == 2 // キー一つだけのやつ
    // ウィンドウ (タイトルも変換が必要)
    static bool Begin(const char* en, const char* jp, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) {
        // 日本語表示の場合:
        // 表示用テキスト(jp) + "###" + 識別用ID(en)
        // これにより、言語を変えてもウィンドウの位置やサイズ(iniファイル設定)が共有されます。
        // 英語の場合:
        // そのまま渡す (enの中に "Name###ID" が入っていてもImGuiが処理してくれる)
        return ImGui::Begin(GetLabelStr(en, jp).c_str(), p_open, flags);
    }
#else // もともとのやつ
    // ウィンドウ (タイトルも変換が必要)
    static bool Begin(const char* en, const char* jp, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) {
        // return ImGui::Begin(SjisToUtf8(title).c_str(), p_open, flags);
        return ImGui::Begin(GetLabelStr(en, jp).c_str(), p_open, flags);
    }
#endif // Test

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
        // 表示は日本語、内部IDは英語 ("敵出現###Spawn Enemy")
        // これにより言語を切り替えてもボタンのIDが変わらず、動作が安定します
        return ImGui::Button(GetLabelStr(en, jp).c_str(), size);
    }
    static bool RadioButton(const char* en, const char* jp, bool active) {
        return ImGui::RadioButton(GetLabelStr(en, jp).c_str(), active);
    }
    // ラジオボタン
    static bool RadioButton(const char* en, const char* jp, int* v, int v_button) {
        return ImGui::RadioButton(GetLabelStr(en, jp).c_str(), v, v_button);
    }
    // チェックボックス
    static bool Checkbox(const char* en, const char* jp, bool* v) {
        return ImGui::Checkbox(GetLabelStr(en, jp).c_str(), v);
    }
    // スライダー (Float)
    static bool SliderFloat(const char* en, const char* jp, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
        return ImGui::SliderFloat(GetLabelStr(en, jp).c_str(), v, v_min, v_max, format, flags);
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
        return ImGui::TreeNode(GetLabelStr(en, jp).c_str());
    }
    // ツリーノード終了 (中身は ImGui::TreePop と同じですが、名前空間を揃えるために用意)
    static void TreePop() {
        ImGui::TreePop();
    }
    // テキスト入力
    static bool InputText(const char* en, const char* jp, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0) {
        // ※安全性強化: ImGuiはUTF-8バッファを要求するため、SJISのbufをそのまま渡すと文字化けします。
        // ここで変換用のテンポラリバッファを使います。

        // 1. SJIS(buf) -> UTF-8(temp)
        std::string utf8Val = SjisToUtf8(buf);
        std::vector<char> utf8Buf(buf_size * 3 + 1, 0); // UTF-8はサイズが増えるので余裕を持つ
        if (!utf8Val.empty()) {
            strncpy_s(utf8Buf.data(), utf8Buf.size(), utf8Val.c_str(), _TRUNCATE);
        }

        // 2. ImGui処理 (IDは共通化されたものを使用)
        bool changed = ImGui::InputText(GetLabelStr(en, jp).c_str(), utf8Buf.data(), utf8Buf.size(), flags);

        // 3. 変更があった場合のみ UTF-8(temp) -> SJIS(buf) に書き戻す
        if (changed) {
            std::string sjisVal = Utf8ToSjis(utf8Buf.data());
            strncpy_s(buf, buf_size, sjisVal.c_str(), _TRUNCATE);
        }

        return changed;
    }
    // 折りたたみヘッダー
    static bool CollapsingHeader(const char* en, const char* jp, ImGuiTreeNodeFlags flags = 0) {
        return ImGui::CollapsingHeader(GetLabelStr(en, jp).c_str(), flags);
    }
    // ドラッグ数値入力
    static bool DragFloat(const char* en, const char* jp, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
        return ImGui::DragFloat(GetLabelStr(en, jp).c_str(), v, v_speed, v_min, v_max, format, flags);
    }
    // ドラッグ数値入力
    static bool DragFloat2(const char* en, const char* jp, float v[2], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
        return ImGui::DragFloat2(GetLabelStr(en, jp).c_str(), v, v_speed, v_min, v_max, format, flags);
    }
    // カラー編集
    static bool ColorEdit4(const char* en, const char* jp, float* col, ImGuiColorEditFlags flags = 0) {
        return ImGui::ColorEdit4(GetLabelStr(en, jp).c_str(), col, flags);
    }
    static bool ColorEdit3(const char* en, const char* jp, float* col, ImGuiColorEditFlags flags = 0) {
        return ImGui::ColorEdit3(GetLabelStr(en, jp).c_str(), col, flags);
    }
};