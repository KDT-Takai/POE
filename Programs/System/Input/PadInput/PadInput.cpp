#include "PadInput.h"
#include <cmath>
#include <imgui.h>

PadInput::PadInput()
{
    // 初期化：すべての値をゼロにする
    m_currentReading = {};
    m_previousReading = {};

    // イベントリスナーの登録
    // コントローラーが接続されたとき
    m_addedToken = winrt::Windows::Gaming::Input::Gamepad::GamepadAdded({ this, &PadInput::OnGamepadAdded });
    // コントローラーが切断されたとき
    m_removedToken = winrt::Windows::Gaming::Input::Gamepad::GamepadRemoved({ this, &PadInput::OnGamepadRemoved });

    // 既に接続されているパッドがあれば取得
    if (winrt::Windows::Gaming::Input::Gamepad::Gamepads().Size() > 0)
    {
        m_gamepad = winrt::Windows::Gaming::Input::Gamepad::Gamepads().GetAt(0);
    }
}

PadInput::~PadInput()
{
    // イベントリスナーの解除
    winrt::Windows::Gaming::Input::Gamepad::GamepadAdded(m_addedToken);
    winrt::Windows::Gaming::Input::Gamepad::GamepadRemoved(m_removedToken);
}

void PadInput::Update()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_gamepad)
    {
        // 前回の状態を保存
        m_previousReading = m_currentReading;
        // 現在の状態を取得
        m_currentReading = m_gamepad.GetCurrentReading();
    }
    else
    {
        // パッドがない場合は入力をクリア
        m_currentReading = {};
        m_previousReading = {};
    }
}

bool PadInput::IsConnected() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_gamepad != nullptr;
}

// --- ボタン判定ロジック ---
// ビット演算を使って判定します

bool PadInput::IsPress(winrt::Windows::Gaming::Input::GamepadButtons button) const
{
    // 現在のフレームでビットが立っているか
    return (m_currentReading.Buttons & button) == button;
}

bool PadInput::IsTrigger(winrt::Windows::Gaming::Input::GamepadButtons button) const
{
    // 現在ON かつ 前回OFF
    return ((m_currentReading.Buttons & button) == button) &&
        ((m_previousReading.Buttons & button) != button);
}

bool PadInput::IsRelease(winrt::Windows::Gaming::Input::GamepadButtons button) const
{
    // 現在OFF かつ 前回ON
    return ((m_currentReading.Buttons & button) != button) &&
        ((m_previousReading.Buttons & button) == button);
}

// --- トリガーとスティック ---

float PadInput::GetLeftTrigger() const
{
    return static_cast<float>(m_currentReading.LeftTrigger);
}

float PadInput::GetRightTrigger() const
{
    return static_cast<float>(m_currentReading.RightTrigger);
}

float PadInput::GetLeftStickX() const
{
    return ApplyDeadzone(static_cast<float>(m_currentReading.LeftThumbstickX), DEADZONE_THRESHOLD);
}

float PadInput::GetLeftStickY() const
{
    return ApplyDeadzone(static_cast<float>(m_currentReading.LeftThumbstickY), DEADZONE_THRESHOLD);
}

float PadInput::GetRightStickX() const
{
    return ApplyDeadzone(static_cast<float>(m_currentReading.RightThumbstickX), DEADZONE_THRESHOLD);
}

float PadInput::GetRightStickY() const
{
    return ApplyDeadzone(static_cast<float>(m_currentReading.RightThumbstickY), DEADZONE_THRESHOLD);
}

// --- ユーティリティ ---

float PadInput::ApplyDeadzone(float value, float deadzone) const
{
    // 値がデッドゾーン内なら0を返す
    if (std::abs(value) < deadzone)
    {
        return 0.0f;
    }
    return value;
}

void PadInput::SetVibration(float leftMotor, float rightMotor)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_gamepad)
    {
        winrt::Windows::Gaming::Input::GamepadVibration vibration;
        vibration.LeftMotor = leftMotor;   // 低周波（ドンドン）
        vibration.RightMotor = rightMotor; // 高周波（ブブブ）
        m_gamepad.Vibration(vibration);
    }
}

void PadInput::RenderImGui()
{
    ImGui::Begin("Pad Input");
    // 接続状態
    if (IsConnected())
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: CONNECTED");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: DISCONNECTED");
        ImGui::TextDisabled("Connect a controller to see details.");
        ImGui::End();
        return;
    }
    ImGui::Separator();
    // アナログスティックとトリガー
    if (ImGui::CollapsingHeader("Analog Inputs", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 描画用リスト取得
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        const float stickRadius = 40.0f; // 円の半径
        const float dotRadius = 2.0f;    // ドット

        // 色設定
        const ImU32 circleBgColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
        const ImU32 outlineColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        const ImU32 crossColor = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f);
        const ImU32 dotColor = ImGui::GetColorU32(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

        // スティック描画用のヘルパーラムダ式
        auto DrawStickViz = [&](const char* label, float x_in, float y_in) {
            ImGui::BeginGroup();

            // ラベルを中央寄せっぽく表示
            float groupWidth = stickRadius * 2.0f;
            float textWidth = ImGui::CalcTextSize(label).x;
            if (groupWidth > textWidth) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (groupWidth - textWidth) * 0.5f);
            ImGui::Text("%s", label);

            // 描画開始位置（左上）を取得
            ImVec2 topLeft = ImGui::GetCursorScreenPos();
            // 円の中心座標を計算
            ImVec2 center = ImVec2(topLeft.x + stickRadius, topLeft.y + stickRadius);
            // ImGuiに見えない領域を確保させる
            ImGui::Dummy(ImVec2(groupWidth, stickRadius * 2));
            // 背景の円と枠線
            draw_list->AddCircleFilled(center, stickRadius, circleBgColor);
            draw_list->AddCircle(center, stickRadius, outlineColor);
            // 十字キー
            draw_list->AddLine(ImVec2(center.x - stickRadius, center.y), ImVec2(center.x + stickRadius, center.y), crossColor);
            draw_list->AddLine(ImVec2(center.x, center.y - stickRadius), ImVec2(center.x, center.y + stickRadius), crossColor);
            // スティック位置のドット
            ImVec2 dotPos = ImVec2(center.x + (x_in * stickRadius), center.y + (-y_in * stickRadius));
            draw_list->AddCircleFilled(dotPos, dotRadius, dotColor);
            // 数値表示のズレ防止 "%+.2f" で常に符号(+/-)を表示し、文字数を固定化
            char coordsBuffer[32];
            sprintf_s(coordsBuffer, "X:%+.2f Y:%+.2f", x_in, y_in);
            // 数値を中央寄せ
            float coordsWidth = ImGui::CalcTextSize(coordsBuffer).x;
            if (groupWidth > coordsWidth) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (groupWidth - coordsWidth) * 0.5f);
            ImGui::TextDisabled("%s", coordsBuffer);
            ImGui::EndGroup();
            };
        // 描画処理
        DrawStickViz("Left Stick", GetLeftStickX(), GetLeftStickY());
        ImGui::SameLine(0, 40);
        DrawStickViz("Right Stick", GetRightStickX(), GetRightStickY());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        // トリガー
        ImGui::Text("Triggers");
        float lt = GetLeftTrigger();
        float rt = GetRightTrigger();
        ImGui::Text("LT"); ImGui::SameLine();
        ImGui::ProgressBar(lt, ImVec2(-1, 15), "");
        ImGui::Text("RT"); ImGui::SameLine();
        ImGui::ProgressBar(rt, ImVec2(-1, 15), "");
    }

    // ボタン
    if (ImGui::CollapsingHeader("Buttons", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos(); // 描画の基準点 (左上)
        // 設定
        float startX = p.x + 20.0f;
        float startY = p.y;
        // 色定義
        ImU32 colNone = ImGui::GetColorU32(ImGuiCol_FrameBg);       // 未入力時の色
        ImU32 colActive = ImGui::GetColorU32(ImVec4(0, 1, 0, 1));   // 入力時 (緑)
        ImU32 colText = ImGui::GetColorU32(ImGuiCol_Text);          // 文字色
        ImU32 colBorder = ImGui::GetColorU32(ImGuiCol_Border);      // 枠線
        // 円形ボタン描画
        auto DrawCircleBtn = [&](float x, float y, const char* label, winrt::Windows::Gaming::Input::GamepadButtons btn) {
            bool on = IsPress(btn);
            ImVec2 center(startX + x, startY + y);
            float radius = 12.0f;
            // 背景
            draw_list->AddCircleFilled(center, radius, on ? colActive : colNone);
            draw_list->AddCircle(center, radius, colBorder);
            // 文字 (中央揃え)
            ImVec2 txtSz = ImGui::CalcTextSize(label);
            draw_list->AddText(ImVec2(center.x - txtSz.x * 0.5f, center.y - txtSz.y * 0.5f), colText, label);
            };

        // 長方形ボタン描画 (L/Rバンパー, Menu用) 
        auto DrawRectBtn = [&](float x, float y, float w, float h, const char* label, winrt::Windows::Gaming::Input::GamepadButtons btn) {
            bool on = IsPress(btn);
            ImVec2 tl(startX + x, startY + y);
            ImVec2 br(startX + x + w, startY + y + h);
            draw_list->AddRectFilled(tl, br, on ? colActive : colNone, 4.0f);
            draw_list->AddRect(tl, br, colBorder, 4.0f);
            ImVec2 txtSz = ImGui::CalcTextSize(label);
            draw_list->AddText(ImVec2(tl.x + (w - txtSz.x) * 0.5f, tl.y + (h - txtSz.y) * 0.5f), colText, label);
            };

        // 十字キー描画
        auto DrawDPad = [&](float x, float y) {
            float sz = 20.0f;   // ブロック1個のサイズ
            float padX = startX + x;
            float padY = startY + y;

            // 各方向の矩形定義 (中心からのオフセット)
            struct Dir { float ox, oy; winrt::Windows::Gaming::Input::GamepadButtons btn; };
            Dir dirs[] = {
                { 0, -1, winrt::Windows::Gaming::Input::GamepadButtons::DPadUp },   // 上
                { 0,  1, winrt::Windows::Gaming::Input::GamepadButtons::DPadDown }, // 下
                {-1,  0, winrt::Windows::Gaming::Input::GamepadButtons::DPadLeft }, // 左
                { 1,  0, winrt::Windows::Gaming::Input::GamepadButtons::DPadRight },// 右
                { 0,  0, winrt::Windows::Gaming::Input::GamepadButtons::None }      // 中心(飾り)
            };

            for (auto& d : dirs)
            {
                bool on = (d.btn != winrt::Windows::Gaming::Input::GamepadButtons::None) && IsPress(d.btn);

                ImVec2 tl(padX + d.ox * sz - (sz * 0.5f), padY + d.oy * sz - (sz * 0.5f));
                ImVec2 br(tl.x + sz, tl.y + sz);

                draw_list->AddRectFilled(tl, br, on ? colActive : colNone, 2.0f);
                draw_list->AddRect(tl, br, colBorder, 2.0f);
            }
            };

        // 実際の描画配置

        // 上部 (LB / RB)
        DrawRectBtn(0, 0, 60, 20, "LB", winrt::Windows::Gaming::Input::GamepadButtons::LeftShoulder);
        DrawRectBtn(140, 0, 60, 20, "RB", winrt::Windows::Gaming::Input::GamepadButtons::RightShoulder);

        // 中央 (View / Menu)
        DrawRectBtn(65, 10, 30, 15, "V", winrt::Windows::Gaming::Input::GamepadButtons::View);
        DrawRectBtn(105, 10, 30, 15, "M", winrt::Windows::Gaming::Input::GamepadButtons::Menu);

        // 左側 (十字キー) - 座標(30, 60)を中心
        DrawDPad(30, 60);

        // 右側 (ABXY) - 座標(170, 60)を中心
        // XBox配置: Y(上), A(下), X(左), B(右)
        DrawCircleBtn(170, 40, "Y", winrt::Windows::Gaming::Input::GamepadButtons::Y);
        DrawCircleBtn(170, 80, "A", winrt::Windows::Gaming::Input::GamepadButtons::A);
        DrawCircleBtn(150, 60, "X", winrt::Windows::Gaming::Input::GamepadButtons::X);
        DrawCircleBtn(190, 60, "B", winrt::Windows::Gaming::Input::GamepadButtons::B);

        // スティック押し込み (LSB / RSB)
        // 十字キーとABXYの下あたりに配置
        DrawRectBtn(10, 95, 40, 15, "LSB", winrt::Windows::Gaming::Input::GamepadButtons::LeftThumbstick);
        DrawRectBtn(150, 95, 40, 15, "RSB", winrt::Windows::Gaming::Input::GamepadButtons::RightThumbstick);

        // 描画エリアの確保 (これがないと次の要素が重なる)
        ImGui::Dummy(ImVec2(200, 120));
    }

    // 振動テスト
    if (ImGui::CollapsingHeader("Vibration Test"))
    {
        static float motors[2] = { 0.0f, 0.0f }; // 0:Left(Low), 1:Right(High)
        ImGui::SliderFloat("Low Freq (L)", &motors[0], 0.0f, 1.0f);
        ImGui::SliderFloat("High Freq (R)", &motors[1], 0.0f, 1.0f);
        if (ImGui::Button("Vibrate", ImVec2(100, 0)))
        {
            SetVibration(motors[0], motors[1]);
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop", ImVec2(100, 0)))
        {
            SetVibration(0.0f, 0.0f);
        }
    }
    ImGui::End();
}

// イベントハンドラ
void PadInput::OnGamepadAdded(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Gaming::Input::Gamepad const& gamepad)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // まだパッドを持っていなければ、これをメインにする
    if (!m_gamepad)
    {
        m_gamepad = gamepad;
    }
}

void PadInput::OnGamepadRemoved(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Gaming::Input::Gamepad const& gamepad)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // メインパッドが抜かれた場合
    if (m_gamepad == gamepad)
    {
        m_gamepad = nullptr;
        // 他に接続されているパッドがあれば割り当てる（フェイルオーバー）
        if (winrt::Windows::Gaming::Input::Gamepad::Gamepads().Size() > 0)
        {
            m_gamepad = winrt::Windows::Gaming::Input::Gamepad::Gamepads().GetAt(0);
        }
    }
}