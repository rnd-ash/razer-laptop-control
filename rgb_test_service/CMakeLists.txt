# cmake_minimum_required(VERSION <specify CMake version here>)
project(rgb_test_service)
cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 17)

include_directories(src)

add_executable(rgb_test_service
        src/colours.hpp
        src/effects.cpp
        src/effects.h
        src/keyboard.cpp
        src/keyboard.hpp
        src/main.cpp
        src/power_fan_control.cpp
        src/power_fan_control.hpp
        CMakeLists.txt)
