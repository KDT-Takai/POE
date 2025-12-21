#include "KeyInput.h"
#include <imgui.h>
#include <string>
#include "../../DebugGui/DebugGui.h"

KeyInput::KeyInput()
{
	nowKeyInput.fill(false);
	beforKeyInput.fill(false);
}

void KeyInput::Update()
{
	// 前回の状態を保存
	beforKeyInput = nowKeyInput;
	// キー入力更新
	for (int i = 0; i < KEY_MAX; i++)
		nowKeyInput[i] = sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(i));
}

// 押した瞬間
bool KeyInput::IsGetKey(sf::Keyboard::Key key) const
{
	return nowKeyInput[static_cast<int>(key)] && !beforKeyInput[static_cast<int>(key)];
}
// 押されている間
bool KeyInput::GetKey(sf::Keyboard::Key key) const
{
	return nowKeyInput[static_cast<int>(key)];
}
// 長押し
bool KeyInput::GetKeyRepeat(sf::Keyboard::Key key) const
{
	return nowKeyInput[static_cast<int>(key)] && beforKeyInput[static_cast<int>(key)];
}

void KeyInput::RenderImGui()
{
    DebugGui::Begin("Key Input###KeyInput", "キー入力##KeyInput");

    // 設定
    const float KEY_SIZE = 30.0f; // 基本のキーサイズ
    const float SPACING = 4.0f;   // キー同士の間隔

    // スタイル調整（キー同士を少し詰めるとキーボードっぽくなる）
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(SPACING, SPACING));

    // ヘルパー関数: キー描画
    // label: 表示文字, key: SFMLキー, widthMult: 横幅の倍率
    auto DrawKey = [&](const char* label, sf::Keyboard::Key key, float widthMult = 1.0f) {

        bool on = false;
        if (key != sf::Keyboard::Key::Unknown) {
            on = GetKey(key);
        }

        // 色設定
        if (on) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
        else    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

        // サイズ計算
        float w = (KEY_SIZE * widthMult) + (SPACING * (widthMult - 1.0f));

        // ボタン描画
        if (ImGui::Button(label, ImVec2(w, KEY_SIZE)))
        {
            // クリック時の処理
        }

        ImGui::PopStyleColor();
        ImGui::SameLine();
    };

    // 改行用ヘルパー
    auto NewRow = [&]() {
        ImGui::NewLine();
    };

    // --- 1. メインキーボードエリア ---
    ImGui::BeginGroup();
    {
        // Row 0: Esc & F-Keys
        DrawKey("ESC", sf::Keyboard::Key::Escape);
        ImGui::SameLine(0, 20); // 少し間隔
        DrawKey("F1", sf::Keyboard::Key::F1);
        DrawKey("F2", sf::Keyboard::Key::F2);
        DrawKey("F3", sf::Keyboard::Key::F3);
        DrawKey("F4", sf::Keyboard::Key::F4);
        ImGui::SameLine(0, 10);
        DrawKey("F5", sf::Keyboard::Key::F5);
        DrawKey("F6", sf::Keyboard::Key::F6);
        DrawKey("F7", sf::Keyboard::Key::F7);
        DrawKey("F8", sf::Keyboard::Key::F8);
        ImGui::SameLine(0, 10);
        DrawKey("F9", sf::Keyboard::Key::F9);
        DrawKey("F10", sf::Keyboard::Key::F10);
        DrawKey("F11", sf::Keyboard::Key::F11);
        DrawKey("F12", sf::Keyboard::Key::F12);

        NewRow();

        // Row 1: Numbers
        // ※SFMLのNum0-9はトップキー、Numpad0-9はテンキー
        DrawKey(" ~ ", sf::Keyboard::Key::Grave); // Tildeがない古いSFMLの場合は省略可
        DrawKey(" 1 ", sf::Keyboard::Key::Num1);
        DrawKey(" 2 ", sf::Keyboard::Key::Num2);
        DrawKey(" 3 ", sf::Keyboard::Key::Num3);
        DrawKey(" 4 ", sf::Keyboard::Key::Num4);
        DrawKey(" 5 ", sf::Keyboard::Key::Num5);
        DrawKey(" 6 ", sf::Keyboard::Key::Num6);
        DrawKey(" 7 ", sf::Keyboard::Key::Num7);
        DrawKey(" 8 ", sf::Keyboard::Key::Num8);
        DrawKey(" 9 ", sf::Keyboard::Key::Num9);
        DrawKey(" 0 ", sf::Keyboard::Key::Num0);
        DrawKey(" - ", sf::Keyboard::Key::Hyphen);
        DrawKey(" = ", sf::Keyboard::Key::Equal);
        DrawKey("Back", sf::Keyboard::Key::Backspace, 2.0f); // 幅2倍

        NewRow();

        // Row 2: Top Alpha (Tab, Q-P)
        DrawKey("Tab ", sf::Keyboard::Key::Tab, 1.5f);
        DrawKey(" Q ", sf::Keyboard::Key::Q);
        DrawKey(" W ", sf::Keyboard::Key::W);
        DrawKey(" E ", sf::Keyboard::Key::E);
        DrawKey(" R ", sf::Keyboard::Key::R);
        DrawKey(" T ", sf::Keyboard::Key::T);
        DrawKey(" Y ", sf::Keyboard::Key::Y);
        DrawKey(" U ", sf::Keyboard::Key::U);
        DrawKey(" I ", sf::Keyboard::Key::I);
        DrawKey(" O ", sf::Keyboard::Key::O);
        DrawKey(" P ", sf::Keyboard::Key::P);
        DrawKey(" [ ", sf::Keyboard::Key::LBracket);
        DrawKey(" ] ", sf::Keyboard::Key::RBracket);
        DrawKey(" | ", sf::Keyboard::Key::Backslash, 1.5f);

        NewRow();

        // Row 3: Mid Alpha (Caps, A-L, Enter)
        DrawKey("Caps", sf::Keyboard::Key::Unknown, 1.7f); // SFMLにCapsがない場合が多いのでダミー
        DrawKey(" A ", sf::Keyboard::Key::A);
        DrawKey(" S ", sf::Keyboard::Key::S);
        DrawKey(" D ", sf::Keyboard::Key::D);
        DrawKey(" F ", sf::Keyboard::Key::F);
        DrawKey(" G ", sf::Keyboard::Key::G);
        DrawKey(" H ", sf::Keyboard::Key::H);
        DrawKey(" J ", sf::Keyboard::Key::J);
        DrawKey(" K ", sf::Keyboard::Key::K);
        DrawKey(" L ", sf::Keyboard::Key::L);
        DrawKey(" ; ", sf::Keyboard::Key::Semicolon);
        DrawKey(" ' ", sf::Keyboard::Key::Apostrophe);
        DrawKey("Enter", sf::Keyboard::Key::Enter, 2.3f);

        NewRow();

        // Row 4: Bot Alpha (Shift, Z-M)
        DrawKey("Shift", sf::Keyboard::Key::LShift, 2.3f);
        DrawKey(" Z ", sf::Keyboard::Key::Z);
        DrawKey(" X ", sf::Keyboard::Key::X);
        DrawKey(" C ", sf::Keyboard::Key::C);
        DrawKey(" V ", sf::Keyboard::Key::V);
        DrawKey(" B ", sf::Keyboard::Key::B);
        DrawKey(" N ", sf::Keyboard::Key::N);
        DrawKey(" M ", sf::Keyboard::Key::M);
        DrawKey(" , ", sf::Keyboard::Key::Comma);
        DrawKey(" . ", sf::Keyboard::Key::Period);
        DrawKey(" / ", sf::Keyboard::Key::Slash);
        DrawKey("Shift", sf::Keyboard::Key::RShift, 2.7f);

        NewRow();

        // Row 5: Modifiers & Space
        DrawKey("Ctrl", sf::Keyboard::Key::LControl, 1.5f);
        DrawKey("Win ", sf::Keyboard::Key::LSystem, 1.3f);
        DrawKey("Alt ", sf::Keyboard::Key::LAlt, 1.3f);
        DrawKey("Space", sf::Keyboard::Key::Space, 6.2f); // スペースキー
        DrawKey("Alt ", sf::Keyboard::Key::RAlt, 1.3f);
        DrawKey("Win ", sf::Keyboard::Key::RSystem, 1.3f);
        DrawKey("Menu", sf::Keyboard::Key::Menu, 1.3f);
        DrawKey("Ctrl", sf::Keyboard::Key::RControl, 1.5f);
    }
    ImGui::EndGroup();

    // --- 2. ナビゲーションキー (矢印など) ---
    ImGui::SameLine(0, 15); // メインとの隙間
    ImGui::BeginGroup();
    {
        // Ins / Home / PgUp
        DrawKey("Ins", sf::Keyboard::Key::Insert);
        DrawKey("Hom", sf::Keyboard::Key::Home);
        DrawKey("PgU", sf::Keyboard::Key::PageUp);
        NewRow();
        // Del / End / PgDn
        DrawKey("Del", sf::Keyboard::Key::Delete);
        DrawKey("End", sf::Keyboard::Key::End);
        DrawKey("PgD", sf::Keyboard::Key::PageDown);

        NewRow(); NewRow(); // 空白

        // Arrows
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + KEY_SIZE + SPACING); // 中央揃え調整
        DrawKey(" ^ ", sf::Keyboard::Key::Up);
        NewRow();
        DrawKey(" < ", sf::Keyboard::Key::Left);
        DrawKey(" v ", sf::Keyboard::Key::Down);
        DrawKey(" > ", sf::Keyboard::Key::Right);
    }
    ImGui::EndGroup();

    // --- 3. テンキー ---
    ImGui::SameLine(0, 15);
    ImGui::BeginGroup();
    {
        // NumLock / / * / -
        DrawKey("Num", sf::Keyboard::Key::Unknown);
        DrawKey(" / ", sf::Keyboard::Key::Divide);
        DrawKey(" * ", sf::Keyboard::Key::Multiply);
        DrawKey(" - ", sf::Keyboard::Key::Subtract);
        NewRow();
        // 7 8 9 +
        DrawKey(" 7 ", sf::Keyboard::Key::Numpad7);
        DrawKey(" 8 ", sf::Keyboard::Key::Numpad8);
        DrawKey(" 9 ", sf::Keyboard::Key::Numpad9);
        // +キー (縦長にしたいがImGui::Buttonでは難しいので通常配置)
        DrawKey(" + ", sf::Keyboard::Key::Add);
        NewRow();
        // 4 5 6
        DrawKey(" 4 ", sf::Keyboard::Key::Numpad4);
        DrawKey(" 5 ", sf::Keyboard::Key::Numpad5);
        DrawKey(" 6 ", sf::Keyboard::Key::Numpad6);
        NewRow(); // +の縦長分を空ける代わりに改行
        // 1 2 3 Enter
        DrawKey(" 1 ", sf::Keyboard::Key::Numpad1);
        DrawKey(" 2 ", sf::Keyboard::Key::Numpad2);
        DrawKey(" 3 ", sf::Keyboard::Key::Numpad3);
        DrawKey("Ent", sf::Keyboard::Key::Unknown); // NumpadEnterがない場合がある
        NewRow();
        // 0 .
        DrawKey(" 0 ", sf::Keyboard::Key::Numpad0, 2.0f);
        DrawKey(" . ", sf::Keyboard::Key::Unknown);
    }
    ImGui::EndGroup();

    ImGui::PopStyleVar(); // ItemSpacing解除

    ImGui::Separator();

    // --- 4. リスト表示 (既存のロジック) ---
    if (DebugGui::TreeNode("Active Keys List", "アクティブキーリスト"))
    {
        // ヘルパー（前のコードと同じものを使用）
        auto KeyToString = [](int k) -> std::string {
            int keyA = static_cast<int>(sf::Keyboard::Key::A);
            int keyZ = static_cast<int>(sf::Keyboard::Key::Z);
            int keyNum0 = static_cast<int>(sf::Keyboard::Key::Num0);
            int keyNum9 = static_cast<int>(sf::Keyboard::Key::Num9);

            if (k >= keyA && k <= keyZ) return std::string(1, 'A' + (k - keyA));
            if (k >= keyNum0 && k <= keyNum9) return "Num" + std::to_string(k - keyNum0);

            // ... (その他の特殊キー変換は必要に応じて記述) ...

            return std::to_string(k);
            };

        int count = 0;
        int keyCount = static_cast<int>(sf::Keyboard::KeyCount);
        for (int i = 0; i < keyCount; i++) {
            if (nowKeyInput[i])
            {
                if (count > 0) {
                    ImGui::SameLine();
                }
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[%s]", KeyToString(i).c_str());
                count++;
            }
        }
        if (count == 0)
        {
            ImGui::TextDisabled("(None)");
        }
        DebugGui::TreePop();
    }
    DebugGui::End();
}