#include "MemoryMonitor.h"

void MemoryMonitor::Update()
{
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
        (PROCESS_MEMORY_COUNTERS*)&pmc,
        sizeof(pmc)))
    {
        usedMemory = pmc.WorkingSetSize;         // 物理メモリ
        peakMemory = pmc.PeakWorkingSetSize;     // 最大物理メモリ
        privateMemory = pmc.PrivateUsage;        // プロセス専用メモリ
    }
}

void MemoryMonitor::RenderImGui()
{
    ImGui::SeparatorText(" Memory Usage ");

    ImGui::Text("Used Memory     : %.2f MB", usedMemory / (1024.0f * 1024.0f));
    ImGui::Text("Peak Memory     : %.2f MB", peakMemory / (1024.0f * 1024.0f));
    ImGui::Text("Private Memory  : %.2f MB", privateMemory / (1024.0f * 1024.0f));

    // 見た目を少し良くする Bar 表示
    float usedMB = usedMemory / (1024.0f * 1024.0f);
    ImGui::Separator();
    ImGui::Text("Used Memory Bar");
    ImGui::ProgressBar(usedMB / maxDisplayMB, ImVec2(200, 20));
    ImGui::Text("Max Display: %.0f MB", maxDisplayMB);
}
