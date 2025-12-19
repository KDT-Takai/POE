#pragma once
#include <string>

// どこからでも参照できる「言語設定」
class Language {
public:
    enum class Type { English, Japanese };

private:
        static inline Type s_Current = Type::English;

public:
    // 設定変更
    static void Set(Type type) { s_Current = type; }
    static Type Get() { return s_Current; }

    // 翻訳取得関数 (SFMLの sf::Text などで使う)
    // ライブラリ依存がないので、物理演算クラスや敵クラスの中でも安心して使える
    static const char* Str(const char* en, const char* jp) {
        return (s_Current == Type::English) ? en : jp;
    }
};

/*
Language 言語状態・切替の単一責任
    LanguageManager（Singleton or Service）

Localization 翻訳データ管理（拡張用）
    TextID → 各言語文字列

DebugGui ImGui ラッパ
    Text(TextID)
    Text(const char* en, const char* jp)
*/