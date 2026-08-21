#include "hardware_analyzer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <thread>
#include <vector>

namespace
{
    constexpr int BENCHMARK_RUNS = 3;

    constexpr double BENCHMARK_SECONDS = 0.50;

    /*
        These are capability thresholds, not claims that
        this CPU is X% of the world's fastest CPU.

        They represent useful AI-oriented performance
        ranges for this benchmark workload.
    */

    constexpr double SINGLE_REFERENCE = 100000000.0;
    constexpr double MULTI_REFERENCE = 500000000.0;
}

// =====================================================
// CPU BENCHMARK
// =====================================================

CPUBenchmarkResult HardwareAnalyzer::runCPUBenchmark(
    const HardwareProfile& hardware)
{
    CPUBenchmarkResult result;

    // -------------------------------------------------
    // Single-thread benchmark
    // -------------------------------------------------

    std::vector<double> singleResults;

    for (int run = 0; run < BENCHMARK_RUNS; ++run)
    {
        volatile uint64_t value = 0;

        uint64_t operations = 0;

        auto start =
            std::chrono::steady_clock::now();

        while (true)
        {
            value =
                value * 1664525ULL +
                1013904223ULL;

            operations += 4;

            auto now =
                std::chrono::steady_clock::now();

            double elapsed =
                std::chrono::duration<double>(
                    now - start
                ).count();

            if (elapsed >= BENCHMARK_SECONDS)
            {
                break;
            }
        }

        auto end =
            std::chrono::steady_clock::now();

        double seconds =
            std::chrono::duration<double>(
                end - start
            ).count();

        double score =
            static_cast<double>(operations) /
            std::max(seconds, 0.000001);

        singleResults.push_back(score);
    }

    // -------------------------------------------------
    // Multi-thread benchmark
    // -------------------------------------------------

    std::vector<double> multiResults;

    uint32_t threadCount =
        std::max(
            1u,
            hardware.cpu.logical_processors
        );

    for (int run = 0; run < BENCHMARK_RUNS; ++run)
    {
        std::atomic<bool> startFlag(false);
        std::atomic<bool> stopFlag(false);

        std::atomic<uint64_t> operations(0);

        std::vector<std::thread> workers;

        workers.reserve(threadCount);

        for (uint32_t i = 0;
             i < threadCount;
             ++i)
        {
            workers.emplace_back(
                [&]()
                {
                    volatile uint64_t value = 0;

                    while (!startFlag.load(
                        std::memory_order_acquire))
                    {
                        std::this_thread::yield();
                    }

                    while (!stopFlag.load(
                        std::memory_order_relaxed))
                    {
                        value =
                            value * 1664525ULL +
                            1013904223ULL;

                        operations.fetch_add(
                            4,
                            std::memory_order_relaxed
                        );
                    }
                }
            );
        }

        auto start =
            std::chrono::steady_clock::now();

        startFlag.store(
            true,
            std::memory_order_release
        );

        while (true)
        {
            auto now =
                std::chrono::steady_clock::now();

            double elapsed =
                std::chrono::duration<double>(
                    now - start
                ).count();

            if (elapsed >= BENCHMARK_SECONDS)
            {
                break;
            }

            std::this_thread::yield();
        }

        stopFlag.store(
            true,
            std::memory_order_release
        );

        for (auto& worker : workers)
        {
            worker.join();
        }

        auto end =
            std::chrono::steady_clock::now();

        double seconds =
            std::chrono::duration<double>(
                end - start
            ).count();

        double score =
            static_cast<double>(
                operations.load()
            ) /
            std::max(seconds, 0.000001);

        multiResults.push_back(score);
    }

    // -------------------------------------------------
    // Calculate averages
    // -------------------------------------------------

    auto average =
        [](const std::vector<double>& values)
        {
            if (values.empty())
                return 0.0;

            double total = 0.0;

            for (double value : values)
            {
                total += value;
            }

            return total /
                   static_cast<double>(
                       values.size()
                   );
        };

    result.average_single_thread =
        average(singleResults);

    result.average_multi_thread =
        average(multiResults);

    // -------------------------------------------------
    // Stability
    // -------------------------------------------------

    auto calculateStability =
        [](const std::vector<double>& values)
        {
            if (values.size() < 2)
                return 100.0;

            double avg = 0.0;

            for (double value : values)
            {
                avg += value;
            }

            avg /=
                static_cast<double>(
                    values.size()
                );

            if (avg <= 0.0)
                return 0.0;

            double deviation = 0.0;

            for (double value : values)
            {
                deviation +=
                    std::abs(value - avg) /
                    avg;
            }

            deviation /=
                static_cast<double>(
                    values.size()
                );

            double stability =
                100.0 -
                deviation * 100.0;

            return std::clamp(
                stability,
                0.0,
                100.0
            );
        };

    double singleStability =
        calculateStability(
            singleResults
        );

    double multiStability =
        calculateStability(
            multiResults
        );

    result.stability_score =
        (singleStability +
         multiStability) / 2.0;

    // -------------------------------------------------
    // Normalize single-thread capability
    // -------------------------------------------------

    double singleScore =
        std::log10(
            std::max(
                result.average_single_thread,
                1.0
            ) /
            SINGLE_REFERENCE
        );

    singleScore =
        std::clamp(
            singleScore * 50.0,
            0.0,
            100.0
        );

    // -------------------------------------------------
    // Normalize multi-thread capability
    // -------------------------------------------------

    double multiScore =
        std::log10(
            std::max(
                result.average_multi_thread,
                1.0
            ) /
            MULTI_REFERENCE
        );

    multiScore =
        std::clamp(
            multiScore * 50.0,
            0.0,
            100.0
        );

    // -------------------------------------------------
    // Final CPU capability
    // -------------------------------------------------

    result.single_thread_score =
        singleScore;

    result.multi_thread_score =
        multiScore;

    result.normalized_score =
        (singleScore * 0.35) +
        (multiScore * 0.45) +
        (result.stability_score * 0.20);

    result.normalized_score =
        std::clamp(
            result.normalized_score,
            0.0,
            100.0
        );

    return result;
}

// =====================================================
// RAM
// =====================================================

double HardwareAnalyzer::calculateRAMScore(
    const HardwareProfile& hardware)
{
    double ramGB =
        static_cast<double>(
            hardware.ram.total_bytes
        ) /
        (1024.0 * 1024.0 * 1024.0);

    /*
        AI-oriented RAM capability.

        4 GB = minimum supported
        8 GB = usable
        16 GB = strong
        32 GB = very strong
        64 GB+ = maximum
    */

    if (ramGB <= 4.0)
        return 20.0;

    if (ramGB <= 8.0)
        return 40.0;

    if (ramGB <= 16.0)
        return 70.0;

    if (ramGB <= 32.0)
        return 85.0;

    return 100.0;
}

// =====================================================
// GPU
// =====================================================

double HardwareAnalyzer::calculateGPUScore(
    const HardwareProfile& hardware)
{
    double bestScore = 0.0;

    for (const auto& gpu : hardware.gpus)
    {
        if (gpu.dedicated)
        {
            double vramGB =
                static_cast<double>(
                    gpu.dedicated_memory_bytes
                ) /
                (1024.0 * 1024.0 * 1024.0);

            double score;

            if (vramGB < 2.0)
                score = 35.0;
            else if (vramGB < 4.0)
                score = 50.0;
            else if (vramGB < 8.0)
                score = 70.0;
            else if (vramGB < 12.0)
                score = 85.0;
            else
                score = 100.0;

            bestScore =
                std::max(
                    bestScore,
                    score
                );
        }
        else if (gpu.integrated)
        {
            double sharedGB =
                static_cast<double>(
                    gpu.shared_memory_bytes
                ) /
                (1024.0 * 1024.0 * 1024.0);

            double score =
                25.0 +
                std::min(
                    sharedGB * 3.0,
                    25.0
                );

            bestScore =
                std::max(
                    bestScore,
                    score
                );
        }
    }

    return std::clamp(
        bestScore,
        0.0,
        100.0
    );
}

// =====================================================
// STORAGE
// =====================================================

double HardwareAnalyzer::calculateStorageScore(
    const HardwareProfile& hardware)
{
    double freeGB =
        static_cast<double>(
            hardware.storage.free_bytes
        ) /
        (1024.0 * 1024.0 * 1024.0);

    if (freeGB < 5.0)
        return 10.0;

    if (freeGB < 10.0)
        return 30.0;

    if (freeGB < 20.0)
        return 50.0;

    if (freeGB < 50.0)
        return 70.0;

    if (freeGB < 100.0)
        return 85.0;

    return 100.0;
}

// =====================================================
// OVERALL
// =====================================================

double HardwareAnalyzer::calculateOverallScore(
    double cpu,
    double ram,
    double gpu,
    double storage)
{
    /*
        CPU and RAM are most important for CPU-first
        local AI.

        GPU is important but your minimum system
        cannot depend on a GPU.

        Storage affects model availability rather
        than inference speed.
    */

    double score =
        (cpu * 0.35) +
        (ram * 0.35) +
        (gpu * 0.20) +
        (storage * 0.10);

    return std::clamp(
        score,
        0.0,
        100.0
    );
}

// =====================================================
// CLASSIFICATION
// =====================================================

std::string HardwareAnalyzer::classifyHardware(
    double score,
    const HardwareProfile& hardware)
{
    double ramGB =
        static_cast<double>(
            hardware.ram.total_bytes
        ) /
        (1024.0 * 1024.0 * 1024.0);

    // Hard minimum class
    if (ramGB <= 4.0)
        return "ULTRA_LOW";

    if (score < 30.0)
        return "LOW";

    if (score < 50.0)
        return "MEDIUM";

    if (score < 70.0)
        return "HIGH";

    return "VERY_HIGH";
}

// =====================================================
// MAIN ANALYZER
// =====================================================

CapabilityProfile HardwareAnalyzer::analyze(
    const HardwareProfile& hardware)
{
    CapabilityProfile profile;

    // -----------------------------------------------
    // CPU
    // -----------------------------------------------

    profile.cpu_benchmark =
        runCPUBenchmark(
            hardware
        );

    profile.cpu_score =
        profile.cpu_benchmark.normalized_score;

    // -----------------------------------------------
    // RAM
    // -----------------------------------------------

    profile.ram_score =
        calculateRAMScore(
            hardware
        );

    // -----------------------------------------------
    // GPU
    // -----------------------------------------------

    profile.gpu_score =
        calculateGPUScore(
            hardware
        );

    // -----------------------------------------------
    // Storage
    // -----------------------------------------------

    profile.storage_score =
        calculateStorageScore(
            hardware
        );

    // -----------------------------------------------
    // Overall
    // -----------------------------------------------

    profile.overall_score =
        calculateOverallScore(
            profile.cpu_score,
            profile.ram_score,
            profile.gpu_score,
            profile.storage_score
        );

    // -----------------------------------------------
    // Hardware class
    // -----------------------------------------------

    profile.hardware_class =
        classifyHardware(
            profile.overall_score,
            hardware
        );

    // -----------------------------------------------
    // CPU threads
    // -----------------------------------------------

    profile.recommended_cpu_threads =
        std::max(
            1u,
            hardware.cpu.logical_processors / 2
        );

    // -----------------------------------------------
    // Safe RAM
    // -----------------------------------------------

    uint64_t availableRAM =
        hardware.ram.available_bytes;

    /*
        Never give the AI all available memory.

        Keep ~30% of currently available RAM as
        an immediate safety reserve.
    */

    uint64_t safeRAM =
        static_cast<uint64_t>(
            availableRAM * 0.70
        );

    const uint64_t minimumRAM =
        512ULL *
        1024ULL *
        1024ULL;

    profile.recommended_ai_ram_bytes =
        std::max(
            safeRAM,
            minimumRAM
        );

    // -----------------------------------------------
    // GPU availability
    // -----------------------------------------------

    for (const auto& gpu : hardware.gpus)
    {
        if (gpu.integrated)
        {
            profile.use_integrated_gpu = true;
        }

        if (gpu.dedicated)
        {
            profile.use_dedicated_gpu = true;
        }
    }

    profile.use_gpu =
        profile.use_integrated_gpu ||
        profile.use_dedicated_gpu;

    // -----------------------------------------------
    // CPU limit
    // -----------------------------------------------

    if (hardware.cpu.logical_processors <= 2)
    {
        profile.max_cpu_usage_percent = 40.0;
    }
    else if (hardware.cpu.logical_processors <= 4)
    {
        profile.max_cpu_usage_percent = 50.0;
    }
    else if (hardware.cpu.logical_processors <= 8)
    {
        profile.max_cpu_usage_percent = 65.0;
    }
    else
    {
        profile.max_cpu_usage_percent = 75.0;
    }

    // -----------------------------------------------
    // RAM limit
    // -----------------------------------------------

    double availablePercent =
        (
            static_cast<double>(
                hardware.ram.available_bytes
            )
            /
            static_cast<double>(
                hardware.ram.total_bytes
            )
        ) * 100.0;

    profile.max_ram_usage_percent =
        std::clamp(
            availablePercent * 0.70,
            20.0,
            70.0
        );

    profile.explanation =
        "Hardware capability analyzed successfully.";

    return profile;
}