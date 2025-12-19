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