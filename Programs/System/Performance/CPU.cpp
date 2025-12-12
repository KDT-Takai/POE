#include <vector>
#include <imgui.h>
#include "CPU.h"

CpuUsage::CpuUsage()
{
    PdhOpenQuery(NULL, NULL, &query);
    PdhAddCounter(query, TEXT("\\Processor(_Total)\\% Processor Time"), NULL, &counter);
    PdhCollectQueryData(query);
}

CpuUsage::~CpuUsage()
{
    PdhCloseQuery(query);
}

float CpuUsage::Get()
{
    PDH_FMT_COUNTERVALUE value;
    PdhCollectQueryData(query);
    PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &value);
    return (float)value.doubleValue;
}

void CpuUsage::RenderImGui()
{
    static std::vector<float> history(200, 0.0f);
    static int index = 0;
    static float smoothed = 0.0f;
    float raw = Get();
    smoothed = smoothed * 0.92f + raw * 0.08f;
    history[index] = smoothed;
    index = (index + 1) % history.size();
    ImGui::Text("CPU Usage: %.1f %%", smoothed);
    ImGui::PlotLines("##cpu_graph", history.data(), history.size(), index, "", 0.0f, 100.0f, ImVec2(0, 80));
}