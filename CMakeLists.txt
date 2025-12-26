cmake_minimum_required(VERSION 3.15)
project(LoadBalancer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)

# Usage: cmake -DCMAKE_BUILD_TYPE=Debug ..
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build: Enabling AddressSanitizer and UBSan")
    add_compile_options(-g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
else()
    # In Release mode, we optimize for pure speed (P99 Latency).
    message(STATUS "Release build: Enabling O3 optimizations")
    add_compile_options(-O3)
endif()


find_package(Threads REQUIRED)

add_executable(LoadBalancer src/main.cpp)

target_link_libraries(LoadBalancer PRIVATE Threads::Threads)