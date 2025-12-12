#pragma once
#include <imgui.h>
#include <windows.h>
#include <psapi.h>

class MemoryMonitor
{
public:
    void Update();

    void RenderImGui();

    void SetMaxDisplay(float mb) { maxDisplayMB = mb; }

    size_t GetUsedMemory() const { return usedMemory; }
    size_t GetPeakMemory() const { return peakMemory; }
    size_t GetPrivateMemory() const { return privateMemory; }

    float GetUsedMemoryMB() const { return usedMemory / (1024.0f * 1024.0f); }
    float GetPeakMemoryMB() const { return peakMemory / (1024.0f * 1024.0f); }
    float GetPrivateMemoryMB() const { return privateMemory / (1024.0f * 1024.0f); }

private:
    size_t usedMemory = 0;
    size_t peakMemory = 0;
    size_t privateMemory = 0;   // ← PrivateUsage 正常取得
    // プログレスバーの最大表示（2GB）
    float maxDisplayMB = 2048.0f;
};