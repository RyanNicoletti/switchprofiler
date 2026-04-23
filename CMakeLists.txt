cmake_minimum_required(VERSION 3.16)
project(switchprofiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(switchprofiler src/main.cpp)

target_compile_options(switchprofiler PRIVATE -Wall -Wextra -Wpedantic)
