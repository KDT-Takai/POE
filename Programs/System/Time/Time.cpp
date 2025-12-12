#include "Time.h"
#include <imgui.h>
Time::Time() : timeScale(1.0), targetFPS(60), frameCount(0), currentFps(0.0), accumulator(0.0) {}

void Time::Update() {
    // 経過時間を取得
    deltaTime = clock.restart();

    // 異常な長時間フレームのキャップ (0.1秒)
    if (deltaTime.asSeconds() > 0.1f) {
        deltaTime = sf::seconds(0.1f);
    }

    // 総時間の加算
    double dtSec = deltaTime.asMicroseconds() / 1000000.0;

    // ここでエラーが出ていた箇所を修正
    totalTime += sf::microseconds(static_cast<std::int64_t>(dtSec * timeScale * 1000000.0));

    // アキュムレータへの加算 (double精度)
    accumulator += dtSec * timeScale;

    // FPS計測
    fpsTimer += deltaTime;
    frameCount++;
    if (fpsTimer.asSeconds() >= 1.0f) {
        currentFps = static_cast<double>(frameCount);
        frameCount = 0;
        fpsTimer -= sf::seconds(1.0f);
    }
}

double Time::GetDeltaTime() const {
    return (deltaTime.asMicroseconds() / 1000000.0) * timeScale;
}

double Time::GetUnscaledDeltaTime() const {
    return deltaTime.asMicroseconds() / 1000000.0;
}

double Time::GetTotalTime() const {
    return totalTime.asMicroseconds() / 1000000.0;
}

void Time::SetTimeScale(double scale) {
    if (scale >= 0.0)
    {
        timeScale = scale;
    }
}

double Time::GetTimeScale() const {
    return timeScale;
}

double Time::GetFPS() const {
    return currentFps;
}

void Time::SetTargetFPS(unsigned int fps) {
    targetFPS = fps;
}

double Time::GetTargetFPS() const {
    return targetFPS;
}

double Time::GetFixedDeltaTime() const {
    return FIXED_TIME_STEP;
}

double Time::GetAccumulator() const {
    return accumulator;
}

void Time::ConsumeAccumulator(double fixedStep) {
    accumulator -= fixedStep;
}

double Time::GetSmoothedAlpha() const {
    return accumulator / FIXED_TIME_STEP;
}

void Time::RenderImGui()
{
    ImGui::SeparatorText(" FPS ");
    static float fpsHistory[120] = {};
    static int index = 0;

    fpsHistory[index] = Time::Instance().GetFPS();
    index = (index + 1) % 120;

    ImGui::PlotLines("FPS Graph", fpsHistory, IM_ARRAYSIZE(fpsHistory));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 0.4f, 1.0f));  // 緑
    // FPS
    ImGui::Text("FPS: %.1f", GetFPS());
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));  // 薄い青
    // 目標FPS
    ImGui::Text("Target: %.1f", GetTargetFPS());
    ImGui::PopStyleColor();
    // ゲーム内のスピード倍率
    ImGui::Text("Time Scale: %.1f", GetTimeScale());
    ImGui::Separator();
}