//#pragma once
//#include <dxgi1_4.h>
//#pragma comment(lib, "dxgi.lib")
//
//#include "../Singleton/Singleton.h"
//
//float GetGpuUsage() {
//    IDXGIFactory4* factory = nullptr;
//    CreateDXGIFactory1(IID_PPV_ARGS(&factory));
//
//    IDXGIAdapter3* adapter = nullptr;
//    factory->EnumAdapters(0, (IDXGIAdapter**)&adapter);
//
//    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
//    adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
//
//    float used = (float)info.CurrentUsage;
//    float budget = (float)info.Budget;
//
//    adapter->Release();
//    factory->Release();
//
//    if (budget == 0) return 0.f;
//    return (used / budget) * 100.f; // Žg—p—¦ %
//}
