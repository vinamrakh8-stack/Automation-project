#include <iostream>
#include <iomanip>

#include "hardware_detector.h"
#include "hardware_analyzer.h"

double toGB(uint64_t bytes) {
    return static_cast<double>(bytes) /
           (1024.0 * 1024.0 * 1024.0);
}

int main() {
    HardwareDetector detector;

    HardwareProfile hardware = detector.detect();
    HardwareAnalyzer analyzer;

    CapabilityProfile capability = analyzer.analyze(hardware);

    std::cout << "=============================\n";
    std::cout << "     AdaptiveAI Hardware\n";
    std::cout << "=============================\n\n";

    std::cout << "CPU: "
              << hardware.cpu.name << '\n';
    
    std::cout << "Physical cores: "
              << hardware.cpu.physical_cores
              << "\n";

    std::cout << "Logical processors: "
              << hardware.cpu.logical_processors << '\n';

    std::cout << "RAM: "
              << std::fixed << std::setprecision(2)
              << toGB(hardware.ram.total_bytes)
              << " GB\n";

    std::cout << "Available RAM: "
              << toGB(hardware.ram.available_bytes)
              << " GB\n";

    std::cout << "GPU(s): "
              << hardware.gpus.size() << '\n';
    for (size_t i=0;
        i<hardware.gpus.size();
        ++i)
    {
        const auto& gpu =
            hardware.gpus[i];
        std::cout
            << "\nGPU"
            << i
            << ":"
            << gpu.name << '\n';
        if (gpu.integrated){
            std::cout << "Type: Integrated\n";
        }
        else if (gpu.dedicated){
            std::cout << "Type: Dedicated\n";
        }
        std::cout << "Dedicated memory: "
                  << toGB(
                       gpu.dedicated_memory_bytes
                  )        
                  << "GB\n";
        std::cout
            << "Shared memory:"
            << toGB (
                gpu.shared_memory_bytes
            )
            << "GB\n";
    }

    std::cout << "AI Install Drive: "
              << hardware.storage.drive 
              << '\n';
              
    std::cout << "Storage: "
              << std::fixed 
              << std::setprecision(2)
              << toGB(hardware.storage.free_bytes)
              << " GB free / "
              << toGB(hardware.storage.total_bytes)
              << " GB\n";
    
    std::cout << "OS: "
              << hardware.os.name << " "
              << hardware.os.architecture << '\n';          

    std::cout
        << "\nBenchmark\n"; 

    std::cout
        << "CPU score:"
        << hardware.benchmark.cpu_score
        << '\n';
  

    std::cout
        << "RAM score:"
        << hardware.benchmark.ram_score
        << "MB/s\n";

    std::cout
        << "Overall score:"
        << hardware.benchmark.overall_score
        << '\n';        
    
    std::cout << "\n";
    std::cout << "=============================\n";
    std::cout << "     AI Capability Profile\n";
    std::cout << "=============================\n\n";
       
    std::cout
        << "CPU capability: "
        << capability.cpu_score
        << "/100\n";
    std::cout
        << "Single-thread score: "
        << capability.cpu_benchmark.single_thread_score
        << "/100\n";
    std::cout
        << "Multi-thread score: "
        << capability.cpu_benchmark.multi_thread_score
        << "/100\n";
    std::cout
        << "Stability score: "
        << capability.cpu_benchmark.stability_score
        << "/100\n";
    std::cout
        << "Normalized score: "
        << capability.cpu_benchmark.normalized_score
        << "/100\n";       

    std::cout
        << "RAM capability: "
        << capability.ram_score
        << "/100\n";

    std::cout
        << "GPU capability: "
        << capability.gpu_score
        << "/100\n";

    std::cout
        << "Storage capability: "
        << capability.storage_score
        << "/100\n";
 
    std::cout
        << "Overall capability: "
        << capability.overall_score
        << "/100\n\n";

    std::cout
       << "Hardware class: "
       << capability.hardware_class
       << '\n';

    std::cout
       << "Capability score: "
       << std::fixed
       << std::setprecision(2)
       << capability.overall_score
       << "/100\n";

    /*std::cout
       << "Recommended model tier: "
       << capability.recommended_model_tier
       << '\n';
*/
    std::cout
       << "Recommended CPU threads: "
       << capability.recommended_cpu_threads
       << '\n';

    std::cout
       << "Recommended AI RAM: "
       << toGB(
            capability.recommended_ai_ram_bytes
       )
       << " GB\n";

    std::cout
       << "GPU acceleration: "
       << (capability.use_gpu ? "YES" : "NO")
       << '\n';

    std::cout
       << "Integrated GPU: "
       << (capability.use_integrated_gpu ? "YES" : "NO")
       << '\n';

    std::cout
       << "Dedicated GPU: "
       << (capability.use_dedicated_gpu ? "YES" : "NO")
       << '\n';

    std::cout
       << "CPU limit: "
       << capability.max_cpu_usage_percent
       << "%\n";

    std::cout
        << "RAM limit: "
        << capability.max_ram_usage_percent
        << "%\n";
    return 0;
}