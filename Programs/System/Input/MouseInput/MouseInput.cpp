#include "MouseInput.h"
#include <imgui.h>
#include "../../DebugGui/DebugGui.h"

MouseInput::MouseInput() {
	nowMouseInput.fill(false);
	beforMouseInput.fill(false);
	mouseX = 0;
	mouseY = 0;
}

void MouseInput::Update(const sf::RenderWindow& window) {
	// 前回の状態を保存
	beforMouseInput = nowMouseInput;
	// マウス入力更新
	for (int i = 0; i < MOUSE_BUTTON_MAX; i++)
		nowMouseInput[i] = sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(i));
	// マウス座標更新
	sf::Vector2i pos = sf::Mouse::getPosition(window);
	mouseX = pos.x;
	mouseY = pos.y;
}

// 押した瞬間
bool MouseInput::IsGetMouse(sf::Mouse::Button button) const {
	return nowMouseInput[static_cast<int>(button)] && !beforMouseInput[static_cast<int>(button)];
}
// 押されている間
bool MouseInput::GetMouse(sf::Mouse::Button button) const {
	return nowMouseInput[static_cast<int>(button)];
}
// 長押し
bool MouseInput::GetMouseRepeat(sf::Mouse::Button button) const {
	return nowMouseInput[static_cast<int>(button)] && beforMouseInput[static_cast<int>(button)];
}
sf::Vector2i MouseInput::GetMousePoint() {
	return sf::Vector2i{ mouseX, mouseY };
}

void MouseInput::RenderImGui()
{
    DebugGui::Begin("Mouse Input", "マウス入力");

    // 座標表示
    DebugGui::Text("Screen Position", "現在地");
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "X: %d  Y: %d", mouseX, mouseY);

    ImGui::Separator();
    ImGui::Spacing();

    // マウスのビジュアル描画
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos(); // 基準点
    float startX = p.x + 20;
    float startY = p.y;
    // 設定
    float w = 60.0f;     // マウス幅
    float h = 90.0f;     // マウス高さ
    float btnH = 35.0f;  // ボタン部分高さ
    float gap = 2.0f;    // ボタン隙間

    // 色
    ImU32 colBody = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImU32 colActive = ImGui::GetColorU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImU32 colOff = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 colBorder = ImGui::GetColorU32(ImGuiCol_Border);

    // 左クリック
    {
        bool on = GetMouse(sf::Mouse::Button::Left);
        ImVec2 tl(startX, startY);
        ImVec2 br(startX + w / 2 - gap, startY + btnH);

        draw->AddRectFilled(tl, br, on ? colActive : colOff, 5.0f);
        draw->AddRect(tl, br, colBorder, 5.0f);
        draw->AddText(ImVec2(tl.x + 5, tl.y + 10), ImGui::GetColorU32(ImGuiCol_Text), "L");
    }

    // 右クリック
    {
        bool on = GetMouse(sf::Mouse::Button::Right);
        ImVec2 tl(startX + w / 2 + gap, startY);
        ImVec2 br(startX + w, startY + btnH);

        draw->AddRectFilled(tl, br, on ? colActive : colOff, 5.0f);
        draw->AddRect(tl, br, colBorder, 5.0f);
        draw->AddText(ImVec2(tl.x + 5, tl.y + 10), ImGui::GetColorU32(ImGuiCol_Text), "R");
    }

    // ホイール
    {
        bool on = GetMouse(sf::Mouse::Button::Middle);
        float mw = 10.0f;
        float mh = 25.0f;

        ImVec2 tl(startX + (w - mw) / 2, startY + 5);
        ImVec2 br(tl.x + mw, tl.y + mh);

        draw->AddRectFilled(tl, br, on ? colActive : ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, 1)), 3.0f);
        draw->AddRect(tl, br, colBorder, 3.0f);
    }

    // ボディ
    {
        ImVec2 tl(startX, startY + btnH + gap);
        ImVec2 br(startX + w, startY + h);
        // 描画前にクリッピング範囲を設定（安全）
        draw->PushClipRect(tl, br, true);
        draw->AddRectFilled(tl, br, colBody, 10.0f); // 全角丸にしてから…
        draw->PopClipRect(); // その下だけ見るようになる
        // 枠線（クリップなし）
        draw->AddRect(tl, br, colBorder, 10.0f);
    }
    // スペース確保
    ImGui::Dummy(ImVec2(w + 40, h + 20));
    DebugGui::End();
}