#include "hardware_detector.h"

#include <windows.h>
#include <intrin.h>
#include <cstring>
#include <vector>
#include <dxgi.h>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <d3d12.h>



CPUInfo HardwareDetector::detectCPU() {
    CPUInfo info;

    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);

    info.logical_processors = systemInfo.dwNumberOfProcessors;

    info.physical_cores = detectPhysicalCores();

    int cpuInfo[4]{};

    __cpuid(cpuInfo, 0);

    int maxLeaf = cpuInfo[0];

    if (maxLeaf >=0 ){
        char brand[49]{};

        __cpuid(cpuInfo, 0x80000002);
        memcpy(brand, cpuInfo, sizeof(cpuInfo));

        __cpuid(cpuInfo, 0x80000003);
        memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));

        __cpuid(cpuInfo, 0x80000004);
        memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));

        info.name = brand;             
    }

    return info;
}

RAMInfo HardwareDetector::detectRAM() {
    RAMInfo info;

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);

    if (GlobalMemoryStatusEx(&memory)) {
        info.total_bytes = memory.ullTotalPhys;
        info.available_bytes = memory.ullAvailPhys;
    }

    return info;
}

StorageInfo HardwareDetector::detectStorage()
{
    StorageInfo info;

    char path[MAX_PATH]{};

    DWORD length = GetModuleFileNameA(
        nullptr,
        path,
        MAX_PATH
    );

    if (length == 0)
    {
        return info;
    }

    // Example: E:\AdaptiveAI\AdaptiveAI.exe
    // Extract E:
    info.drive = std::string(path, 2);

    // Convert E: -> E:
    std::wstring root(
        info.drive.begin(),
        info.drive.end()
    );

    root += L"\\";

    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFreeBytes{};

    if (GetDiskFreeSpaceExW(
        root.c_str(),
        &freeBytesAvailable,
        &totalBytes,
        &totalFreeBytes))
    {
        info.total_bytes = totalBytes.QuadPart;
        info.free_bytes = totalFreeBytes.QuadPart;
    }

    return info;
}




OSInfo HardwareDetector::detectOS() {
    OSInfo info;

    info.name = "Windows";

#ifdef _WIN64
    info.architecture = "x64";
#else
    info.architecture = "x86";
#endif

    return info;
}


std::vector<GPUInfo> HardwareDetector::detectGPUs()
{
    std::vector<GPUInfo> gpus;

    IDXGIFactory1* factory = nullptr;

    HRESULT result = CreateDXGIFactory1(
        __uuidof(IDXGIFactory1),
        reinterpret_cast<void**>(&factory)
    );

    if (FAILED(result))
    {
        return gpus;
    }

    IDXGIAdapter1* adapter = nullptr;

    for (UINT index = 0;
         factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND;
         index++)
    {
        DXGI_ADAPTER_DESC1 desc{};

        if (FAILED(adapter->GetDesc1(&desc)))
        {
            adapter->Release();
            adapter = nullptr;
            continue;
        }

        // Ignore software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapter->Release();
            adapter = nullptr;
            continue;
        }

        GPUInfo gpu;

        // Convert GPU name from WCHAR to UTF-8
        char name[256]{};

        WideCharToMultiByte(
            CP_UTF8,
            0,
            desc.Description,
            -1,
            name,
            sizeof(name),
            nullptr,
            nullptr
        );

        gpu.name = name;

        // Memory information
        gpu.dedicated_memory_bytes =
            desc.DedicatedVideoMemory;

        gpu.shared_memory_bytes =
            desc.SharedSystemMemory;

        // ------------------------------------------------
        // Determine Integrated vs Dedicated
        // ------------------------------------------------

        bool isIntegrated = false;
        bool architectureDetected = false;

        ID3D12Device* device = nullptr;

        HRESULT deviceResult = D3D12CreateDevice(
            adapter,
            D3D_FEATURE_LEVEL_11_0,
            __uuidof(ID3D12Device),
            reinterpret_cast<void**>(&device)
        );

        if (SUCCEEDED(deviceResult))
        {
            D3D12_FEATURE_DATA_ARCHITECTURE architecture{};

            architecture.NodeIndex = 0;

            if (SUCCEEDED(
                device->CheckFeatureSupport(
                    D3D12_FEATURE_ARCHITECTURE,
                    &architecture,
                    sizeof(architecture)
                )))
            {
                architectureDetected = true;

                // UMA = Unified Memory Architecture
                // Typical integrated GPU behavior
                isIntegrated = architecture.UMA;
            }

            device->Release();
            device = nullptr;
        }

        // ------------------------------------------------
        // Fallback
        // ------------------------------------------------

        if (!architectureDetected)
        {
            // Conservative fallback:
            // GPUs with no meaningful dedicated VRAM
            // are treated as integrated.
            isIntegrated =
                (desc.DedicatedVideoMemory <
                 1024ULL * 1024ULL * 1024ULL);
        }

        gpu.integrated = isIntegrated;
        gpu.dedicated = !isIntegrated;
        gpu.software = false;

        gpus.push_back(gpu);

        adapter->Release();
        adapter = nullptr;
    }

    factory->Release();

    return gpus;
}

HardwareProfile HardwareDetector::detect() {
    HardwareProfile profile;

    profile.cpu = detectCPU();
    profile.ram = detectRAM();
    profile.gpus = detectGPUs();
    profile.storage = detectStorage();
    profile.benchmark =runBenchmark();
    profile.os = detectOS();

    return profile;
}

uint32_t HardwareDetector::detectPhysicalCores()
{
    DWORD length = 0;

    GetLogicalProcessorInformationEx(
        RelationProcessorCore,
        nullptr,
        &length
    );

    if (length == 0)
    {
        return 0;
    }

    std::vector<BYTE> buffer(length);

    auto info =
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
            buffer.data()
        );

    if (!GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            info,
            &length))
    {
        return 0;
    }

    uint32_t cores = 0;

    DWORD offset = 0;

    while (offset < length)
    {
        auto current =
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
                buffer.data() + offset
            );

        if (current->Relationship == RelationProcessorCore)
        {
            cores++;
        }

        offset += current->Size;
    }

    return cores;
}

BenchmarkInfo HardwareDetector::runBenchmark()
{
    BenchmarkInfo result;

    // -------------------------
    // CPU benchmark
    // -------------------------

    constexpr uint64_t iterations = 50'000'000;

    volatile uint64_t value = 1;

    auto start =
        std::chrono::high_resolution_clock::now();

    for (uint64_t i = 0; i < iterations; ++i)
    {
        value = value * 1664525ULL + 1013904223ULL;
    }

    auto end =
        std::chrono::high_resolution_clock::now();

    double seconds =
        std::chrono::duration<double>(end - start).count();

    if (seconds > 0.0)
    {
        result.cpu_score =
            static_cast<double>(iterations) / seconds;
    }

    // -------------------------
    // RAM basic bandwidth test
    // -------------------------

    constexpr size_t bufferSize =
        64 * 1024 * 1024;

    std::vector<uint8_t> buffer(bufferSize);

    start =
        std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < bufferSize; ++i)
    {
        buffer[i] = static_cast<uint8_t>(i);
    }

    end =
        std::chrono::high_resolution_clock::now();

    seconds =
        std::chrono::duration<double>(end - start).count();

    if (seconds > 0.0)
    {
        result.ram_score =
            static_cast<double>(bufferSize) /
            seconds /
            (1024.0 * 1024.0);
    }

    // -------------------------
    // Simple combined score
    // -------------------------

    result.overall_score =
        (result.cpu_score + result.ram_score) / 2.0;

    return result;
}

