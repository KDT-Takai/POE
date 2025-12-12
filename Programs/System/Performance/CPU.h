#pragma once
#include <windows.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")

#include "../Singleton/Singleton.h"

class CpuUsage : public Singleton<CpuUsage> {
    friend class Singleton<CpuUsage>;
public:
    CpuUsage();

    ~CpuUsage();

    float Get();

    void RenderImGui();

private:
    PDH_HQUERY query;
    PDH_HCOUNTER counter;
};
