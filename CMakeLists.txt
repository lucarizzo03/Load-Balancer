cmake_minimum_required(VERSION 3.12)
project(LoadBalancer VERSION 1.0 LANGUAGES CXX)

add_executable(LoadBalancer src/main.cpp)

target_compile_features(LoadBalancer PRIVATE cxx_std_20)
set_target_properties(LoadBalancer PROPERTIES CXX_STANDARD_REQUIRED ON)

target_compile_options(LoadBalancer PRIVATE -O3 -Wall -Wextra)

find_package(Threads REQUIRED)
target_link_libraries(LoadBalancer PRIVATE Threads::Threads)