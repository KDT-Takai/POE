#pragma once
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Foundation.h>       // 接続・切断取得とか こっちのほうが推奨らしい
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Power.h>    // デバイスの充電表示したいよね
#include <mutex>

//using namespace winrt::Windows::Gaming::Input;

class PadInput {
public:
    PadInput();
    ~PadInput();
    // 毎フレーム呼び出す更新処理
    void Update();
    // 接続状態の確認
    bool IsConnected() const;
    // ボタン入力判定
    // 今押されているか (Hold)
    bool IsPress(winrt::Windows::Gaming::Input::GamepadButtons button) const;
    // 今この瞬間に押されたか (Trigger / Down)
    bool IsTrigger(winrt::Windows::Gaming::Input::GamepadButtons button) const;
    // 今この瞬間に離されたか (Release / Up)
    bool IsRelease(winrt::Windows::Gaming::Input::GamepadButtons button) const;
    // アナログ入力 (0.0f ～ 1.0f)
    float GetLeftTrigger() const;
    float GetRightTrigger() const;
    // スティック入力 (-1.0f ～ 1.0f)
    // デッドゾーン処理済み
    float GetLeftStickX() const;
    float GetLeftStickY() const;
    float GetRightStickX() const;
    float GetRightStickY() const;
    // バイブレーション
    void SetVibration(float leftMotor, float rightMotor);

    // Debug用の描画 ImGui
    void RenderImGui();
private:
    // イベントハンドラ
    void OnGamepadAdded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Gaming::Input::Gamepad const& gamepad);
    void OnGamepadRemoved(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Gaming::Input::Gamepad const& gamepad);
    // デッドゾーン計算用ヘルパー
    float ApplyDeadzone(float value, float deadzone) const;
private:
    winrt::Windows::Gaming::Input::Gamepad m_gamepad = nullptr;
    // 現在のフレームと1つ前のフレームの状態
    winrt::Windows::Gaming::Input::GamepadReading m_currentReading;
    winrt::Windows::Gaming::Input::GamepadReading m_previousReading;
    // イベント登録解除用トークン
    winrt::event_token m_addedToken;
    winrt::event_token m_removedToken;
    // スレッド競合を防ぐためのMutex（イベントは別スレッドで来るため）
    mutable std::mutex m_mutex;
    // デッドゾーンの閾値 (通常 0.1 ～ 0.2 くらいが適切)
    const float DEADZONE_THRESHOLD = 0.2f;
};