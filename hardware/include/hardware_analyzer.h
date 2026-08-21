#pragma once

#include "hardware_detector.h"

#include <cstdint>
#include <string>

struct CPUBenchmarkResult
{
    double single_thread_score = 0.0;
    double multi_thread_score = 0.0;
    double stability_score = 0.0;
    double normalized_score = 0.0;

    double average_single_thread = 0.0;
    double average_multi_thread = 0.0;
};

struct CapabilityProfile
{
    // ---------------------------------
    // Component capability scores
    // ---------------------------------

    double cpu_score = 0.0;
    double ram_score = 0.0;
    double gpu_score = 0.0;
    double storage_score = 0.0;

    double overall_score = 0.0;

    // ---------------------------------
    // Classification
    // ---------------------------------

    std::string hardware_class;

    // ---------------------------------
    // CPU
    // ---------------------------------

    CPUBenchmarkResult cpu_benchmark;

    uint32_t recommended_cpu_threads = 1;

    // ---------------------------------
    // RAM
    // ---------------------------------

    uint64_t recommended_ai_ram_bytes = 0;

    // ---------------------------------
    // GPU
    // ---------------------------------

    bool use_gpu = false;
    bool use_integrated_gpu = false;
    bool use_dedicated_gpu = false;

    // ---------------------------------
    // Dynamic resource limits
    // ---------------------------------

    double max_cpu_usage_percent = 50.0;
    double max_ram_usage_percent = 50.0;

    // ---------------------------------
    // Information
    // ---------------------------------

    std::string explanation;
};

class HardwareAnalyzer
{
public:

    CapabilityProfile analyze(
        const HardwareProfile& hardware
    );

private:

    CPUBenchmarkResult runCPUBenchmark(
        const HardwareProfile& hardware
    );

    double calculateRAMScore(
        const HardwareProfile& hardware
    );

    double calculateGPUScore(
        const HardwareProfile& hardware
    );

    double calculateStorageScore(
        const HardwareProfile& hardware
    );

    double calculateOverallScore(
        double cpu,
        double ram,
        double gpu,
        double storage
    );

    std::string classifyHardware(
        double score,
        const HardwareProfile& hardware
    );
};