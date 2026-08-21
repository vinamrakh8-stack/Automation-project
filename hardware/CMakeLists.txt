cmake_minimum_required(VERSION 3.20)

project(AdaptiveAI_Hardware
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AdaptiveAI_Hardware
    src/main.cpp
    src/hardware_detector.cpp
    src/hardware_analyzer.cpp
)

target_include_directories(AdaptiveAI_Hardware
    PRIVATE
    include
)

target_link_libraries(AdaptiveAI_Hardware
    PRIVATE
    dxgi
    d3d12
)