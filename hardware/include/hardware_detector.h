#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct CPUInfo {
    std::string name;
    uint32_t physical_cores = 0;
    uint32_t logical_processors = 0;
};

struct RAMInfo {
    uint64_t total_bytes = 0;
    uint64_t available_bytes = 0;
};

struct GPUInfo {
    std::string name;

    bool integrated = false;
    bool dedicated = false;
    bool software = false;

    uint64_t dedicated_memory_bytes = 0;
    uint64_t shared_memory_bytes = 0;
};


struct DriveInfo {
    std::string letter;
    uint64_t total_bytes = 0;
    uint64_t free_bytes = 0;
};

struct StorageInfo {
    std::string drive;
    uint64_t total_bytes = 0;
    uint64_t free_bytes = 0;
};

struct BenchmarkInfo {
    double cpu_score = 0.0;
    double ram_score = 0.0;
    double overall_score = 0.0;
};

struct OSInfo {
    std::string name;
    std::string architecture;
};

struct HardwareProfile {
    CPUInfo cpu;
    RAMInfo ram;
    std::vector<GPUInfo> gpus;
    StorageInfo storage;
    BenchmarkInfo benchmark;
    OSInfo os;
};

class HardwareDetector {
public:
    HardwareProfile detect();

private:
    CPUInfo detectCPU();
    RAMInfo detectRAM();
    std::vector<GPUInfo> detectGPUs();
    StorageInfo detectStorage();
    BenchmarkInfo runBenchmark();
    OSInfo detectOS();

    uint32_t detectPhysicalCores();
};